#include "globals.h"
#include "baseini.h"
#include "fix.h"

#include <QtCore>
#include <windows.h>
#include <time.h>

using namespace std;

FIX::FIX() 
    : ini_( NULL ),
    flagLock_(new QReadWriteLock()),
    msgSeqNum_(0),
    loggedIn_(),
    lastIncomingTime_(0),
    lastOutgoingTime_(0),
    testRequestSent_(false),
    hbi_(0)
{}

FIX::~FIX()
{
    { QWriteLocker g(flagLock_); }
    delete flagLock_;
    flagLock_ = NULL;
}

string FIX::getField(const QByteArray& message, const char* field, unsigned char reqEntryNum)
{
    QByteArray templ(field);
    templ.append('=');
    int valPos, start = 0;
    int sohPos = message.indexOf(SOH);
    if( sohPos == 0 )
        sohPos = message.indexOf(SOH, ++start);

    // we met field from begin, SOH may be on start, value may be empty
    if( 0 == strncmp(message.data()+start, templ.data(), templ.size()) ) {
        valPos = start+templ.size();
        if( valPos <= sohPos )
            return "";
        return string(message.data() + valPos, sohPos-valPos);
    }

    // template should be more exact
    templ.prepend(SOH);

    unsigned short repeating = 0;
    do {
        // searching again
        if( -1 == (start = message.indexOf(templ, sohPos)) )
            return ""; // not found

        // next go looking for SOH
        valPos = start + templ.size();
        sohPos = message.indexOf(SOH, valPos);
        if( valPos == sohPos )
            return ""; // empty value
    }
    while( repeating++ < reqEntryNum ); // 1 and 2 bytes type comparsion make infinite loop impossible in case of trash

    return string(message.data()+valPos, sohPos-valPos);
}    


std::string FIX::makeTime()
{
	struct tm *area;
	time_t t = time(NULL);
	area = gmtime(&t);

	SYSTEMTIME st;
	GetLocalTime(&st);

    char buf[25];
	sprintf(buf,"%4d%02d%02d-%02d:%02d:%02d.%03d",
			area->tm_year + 1900, area->tm_mon + 1, area->tm_mday,
			area->tm_hour, area->tm_min, area->tm_sec, st.wMilliseconds);

	return string(buf);
}

QByteArray FIX::makeHeader(const char* msgType) const
{
	char buff[256];
	sprintf(buff, "35=%s%c49=%s%c56=%s%c34=%d%c52=%s%c", 
        msgType, SOH,
        ini_->value(SenderCompParam).toStdString().c_str(), SOH,
        ini_->value(TargetCompParam).toStdString().c_str(), SOH,
        ++msgSeqNum_, SOH,
        makeTime().c_str(), SOH );
	return buff;
}

void FIX::completeMessage(QByteArray& message) const
{
    char tmp[24];
    sprintf(tmp, "8=FIX.4.4%c9=%d%c", SOH, message.size(), SOH);
    message.prepend(tmp);

    sprintf(tmp, "10=%03d%c", getChecksum(message.data(), message.size()), SOH);
    message.append(tmp);
}

QByteArray FIX::makeLogon()
{
	msgSeqNum_ = 0;
    hbi_ = ini_->value(HeartbeatParam).toInt();

    char buf[128];
	sprintf_s(buf, 128, "98=0%c108=%d%c141=Y%c553=%s%c554=%s%c", 
        SOH, hbi_, SOH, SOH, 
        ini_->value(SenderCompParam).toStdString().c_str(), SOH,
        ini_->value(PasswordParam).toStdString().c_str(), SOH);

    hbi_ *= 1000;

    QByteArray logon = makeHeader("A").append(buf);
    completeMessage(logon);
    return logon;
}

QByteArray FIX::makeLogout() const
{
    QByteArray logout = makeHeader("5");
	completeMessage( logout );
    return logout;
}

QByteArray FIX::makeHeartBeat() const
{
    QByteArray hearbeat = makeHeader("0");
	completeMessage( hearbeat );
	return hearbeat;
}

QByteArray FIX::makeTestRequest()
{
    {
        QWriteLocker guard(flagLock_);
        testRequestSent_ = true;
    }

    QByteArray test = makeHeader("1").append("112=TSTTST").append(SOH);
    completeMessage( test );
    return test;
}

QByteArray FIX::makeOnTestRequest(const char* TestReqID) const
{
    QByteArray ontest = makeHeader("0").append("112=").append(TestReqID).append(SOH);
	completeMessage(ontest);
    return ontest;
}

QByteArray FIX::makeMarketSubscribe(const char* symbol, qint32 code) const
{
    char buf[100];
	sprintf(buf, "262=%s%c263=1%c264=1%c267=2%c269=0%c269=1%c146=1%c48=%d%c22=8%c", 
            symbol, SOH, SOH, SOH, SOH, SOH, SOH, SOH, code, SOH, SOH);

    QByteArray request = makeHeader("V").append(buf);
    completeMessage( request );

    return request;
};

QByteArray FIX::makeMarketUnSubscribe(const char* symbol, qint32 code) const
{
    if( !loggedIn() )
        return QByteArray();

    char buf[100];
	sprintf(buf, "262=%s%c263=2%c264=1%c267=2%c269=0%c269=1%c146=1%c48=%d%c22=8%c", 
            symbol, SOH, SOH, SOH, SOH, SOH, SOH, SOH, code, SOH, SOH);

    QByteArray request = makeHeader("V").append(buf);
    completeMessage( request );

    return request;
};

bool FIX::normalize(QByteArray& rawFix)
{
    string f35 = getField(rawFix,"35");
    if(f35.empty()) return false;

    string hdrTail = getField(rawFix,"52");
    if(hdrTail.empty()) return false;

    // exeract the message body
    char buf[10]; sprintf_s(buf,10,"%c10=",SOH);
    qint16 endpos = rawFix.indexOf(buf)+1;
    rawFix.truncate(endpos);

    qint16 firstpos = rawFix.indexOf(hdrTail.c_str()) + hdrTail.length() + 1;
    rawFix.remove(0,firstpos);

    // normalize
    rawFix.prepend( makeHeader(f35.c_str()) );
    completeMessage(rawFix);
    return true;
}

QByteArray FIX::takeOutgoing()
{
    if( outgoing_.empty() )
        return QByteArray();

    QByteArray nocopyObj;
    nocopyObj.swap(outgoing_.front());
    outgoing_.pop_front();

    lastOutgoingTime_ = Global::time();
    return nocopyObj;
}

bool FIX::loggedIn() const
{
    QReadLocker g(flagLock_);
    return loggedIn_;
}

void FIX::setLoggedIn(bool on)
{
    QWriteLocker guard(flagLock_);
    loggedIn_ = on;
}

bool FIX::testRequestSent() const
{
    QReadLocker guard(flagLock_);
    return testRequestSent_;
}

void FIX::setTestRequestSent(bool on)
{
    QWriteLocker guard(flagLock_);
    testRequestSent_ = on;
}

void FIX::resetMsgSeqNum(quint32 newMsgSeqNum)
{
    QReadLocker g(flagLock_);
    msgSeqNum_ = newMsgSeqNum;
}
