#include "defines.h"
#include "ini.h"
#include "fix.h"

#include <QtCore>
#include <windows.h>
#include <time.h>

using namespace std;

FIX::FIX(QIni& ini) 
    : ini_( ini ),
    msgSeqNum_(0),
    loggedIn_(),
    lastIncomingTime_(0),
    lastOutgoingTime_(0),
    testRequestSent_(false),
    hbi_(0)
{}

FIX::~FIX() 
{}

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

quint16 FIX::GetChecksum(const char* buf, int buflen)
{
	quint32 cks = 0;
	for(int i = 0; i < buflen; ++i)
		cks += (unsigned char)buf[i];
	return cks % 256;
}

std::string FIX::MakeTime()
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

QByteArray FIX::MakeHeader(char msgType) const
{
	char buff[256];
	sprintf(buff, "35=%c%c49=%s%c56=%s%c34=%d%c52=%s%c", 
        msgType, SOH,
        ini_.value(SenderCompParam).toStdString().c_str(), SOH,
        ini_.value(TargetCompParam).toStdString().c_str(), SOH,
        ++msgSeqNum_, SOH,
        MakeTime().c_str(), SOH );
	return buff;
}

void FIX::ResetMsgSeqNum(quint32 newMsgSeqNum)
{
    QReadLocker guard(&flagMutex_);
    msgSeqNum_ = newMsgSeqNum;
}

void FIX::CompleteMessage(QByteArray& message) const
{
    char tmp[24];
    sprintf(tmp, "8=FIX.4.4%c9=%d%c", SOH, message.size(), SOH);
    message.prepend(tmp);

    sprintf(tmp, "10=%03d%c", GetChecksum(message.data(), message.size()), SOH);
    message.append(tmp);
}

QByteArray FIX::MakeLogon()
{
	msgSeqNum_ = 0;
    hbi_ = ini_.value(HeartbeatParam).toInt();

    char buf[128];
	sprintf_s(buf, 128, "98=0%c108=%d%c141=Y%c553=%s%c554=%s%c", 
        SOH, hbi_, SOH, SOH, 
        ini_.value(SenderCompParam).toStdString().c_str(), SOH,
        ini_.value(PasswordParam).toStdString().c_str(), SOH);

    hbi_ *= 1000;

    QByteArray logon = MakeHeader('A').append(buf);
    CompleteMessage(logon);
    return logon;
}

QByteArray FIX::MakeLogout() const
{
    QByteArray logout = MakeHeader('5');
	CompleteMessage( logout );
    return logout;
}

QByteArray FIX::MakeHeartBeat() const
{
    QByteArray hearbeat = MakeHeader('0');
	CompleteMessage( hearbeat );
	return hearbeat;
}

QByteArray FIX::MakeTestRequest()
{
    {
        QWriteLocker guard(&flagMutex_);
        testRequestSent_ = true;
    }

    QByteArray test = MakeHeader('1').append("112=TSTTST").append(SOH);
    CompleteMessage( test );
    return test;
}

QByteArray FIX::MakeOnTestRequest(const char* TestReqID) const
{
    QByteArray ontest = MakeHeader('0').append("112=").append(TestReqID).append(SOH);
	CompleteMessage(ontest);
    return ontest;
}

QByteArray FIX::MakeMarketDataRequest(const char* symbol, const char* code) const
{
    char buf[100];
	sprintf(buf, "262=%s%c263=1%c264=1%c267=2%c269=0%c269=1%c146=1%c48=%s%c22=8%c", 
            symbol, SOH, SOH, SOH, SOH, SOH, SOH, SOH, code, SOH, SOH);

    QByteArray request = MakeHeader('V').append(buf);
    CompleteMessage( request );

    return request;
};

QByteArray FIX::TakeOutgoing()
{
    if( outgoing_.empty() )
        return QByteArray();

    QByteArray nocopyObj;
    nocopyObj.swap(outgoing_.front());
    outgoing_.pop_front();

    lastOutgoingTime_ = Global::time();
    return nocopyObj;
}

bool FIX::LoggedIn() const
{
    QReadLocker guard(const_cast<QReadWriteLock*>(&flagMutex_));
    return loggedIn_;
}

void FIX::SetLoggedIn(bool on)
{
    QWriteLocker guard(&flagMutex_);
    loggedIn_ = on;
}

bool FIX::TestRequestSent() const
{
    QReadLocker guard(const_cast<QReadWriteLock*>(&flagMutex_));
    return testRequestSent_;
}

void FIX::SetTestRequestSent(bool on)
{
    QWriteLocker guard(&flagMutex_);
    testRequestSent_ = on;
}