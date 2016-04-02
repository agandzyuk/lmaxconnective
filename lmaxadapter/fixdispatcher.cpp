#include "defines.h"
#include "fixdispatcher.h"
#include "ini.h"
#include "scheduler.h"

using namespace std;

//////////////////////////////////////////////////////////////
extern void outerAddQuoteBid(const QString& symbol, double bid);
extern void outerAddQuoteAsk(const QString& symbol, double ask);

//////////////////////////////////////////////////////////////
FIXDispatcher::FIXDispatcher(QIni& ini, QScheduler* scheduler) 
    : FIX(ini),
    scheduler_(scheduler)
{}

FIXDispatcher::~FIXDispatcher()
{}

int FIXDispatcher::process(const QByteArray& message)
{
    lastIncomingTime_ = Global::time();

    string value = getField(message, "35");
    switch(value[0])
    {
    case 'A':
        onLogon(message);
        SetLoggedIn(true);
        return enqueueAll();
    case '0':
        onHeartbeat(message);
        break;
    case '1':
        onTestRequest(message);
        return 1;
    case '2':
        onResendRequest(message);
        break;
    case '3':
        onSessionReject(message);
        break;
    case '4':
        onSequenceReset(message);
        break;
    case '5':
        onLogout(message);
        SetLoggedIn(false);
        return -1;
    case 'W':
        onMarketData(message);
        break;
    case 'Y':
        onMarketDataReject(message);
        break;
    default:
        CDebug() << "Warning: Unexpected Message Type \"" << value.c_str() << "\" received:";
        CDebug(false) << "<< " << message;
        break;
    }
    return 0;
}

void FIXDispatcher::beforeLogout()
{
    QWriteLocker gw(&cacheLock_);
    while( !empty() ) 
    {
        QString sym = back().symbol_;
        QString code = back().code_;
        // removing should be first than table update
        pop_back();
        scheduler_->activateMarketResponse(sym, code);
    }
}

void FIXDispatcher::onLogon(const QByteArray& message)
{
    CDebug() << "Logon type=\"A\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"49");
    if( !tag.empty() )
        CDebug(false) << "tag \"SenderCompID\":49=" << tag.c_str();

    tag = getField(message,"56");
    if( !tag.empty() )
        CDebug(false) << "tag \"TargetCompID\":56=" << tag.c_str();

    scheduler_->activateHeartbeat(hbi_);
    scheduler_->activateTestrequest(hbi_);
}

void FIXDispatcher::onLogout(const QByteArray& message)
{
    CDebug() << "Logout type=\"5\" received:";
    CDebug(false) << "<< " << message;

    string reason = getField(message,"58");
    if( !reason.empty() )
        CDebug(false) << "tag \"Reason\":58=" << reason.c_str();

    beforeLogout();
}

void FIXDispatcher::onHeartbeat(const QByteArray& message)
{
    string tag = getField(message,"112");
    if( !tag.empty() ) {
        CDebug() << "Heartbeat type=\"0\" on TestRequest \"" << tag.c_str() << "\" received:";
        CDebug(false) << "<< " << message;
    }
}

void FIXDispatcher::onTestRequest(const QByteArray& message)
{
    CDebug() << "TestRequest type=\"1\" received:";
    CDebug(false) << "<< " << message;

    string reqId = getField(message,"112");
    if( !reqId.empty() ) {
        CDebug(false) << "tag \"TestReqID\":112=" << reqId.c_str();
        outgoing_.push_back( MakeOnTestRequest(reqId.c_str()) );
    }
    else
        CDebug(false) << "error: tag \"TestReqID\":112 is empty";
}

void FIXDispatcher::onResendRequest(const QByteArray& message)
{
    CDebug() << "ResendRequest type=\"2\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"7");
    if( tag.empty() )
        CDebug(false) << "error: tag \"BeginSeqNo\":7 is empty";
    else
        CDebug(false) << "tag \"BeginSeqNo\":7=" << tag.c_str();

    tag = getField(message,"16");
    if( tag.empty() )
        CDebug(false) << "error: tag \"EndSeqNo\":16 is empty";
    else
        CDebug(false) << "tag \"EndSeqNo\":16=" << tag.c_str();

    CDebug() << "Warning: behaviour on ResendRequest is not implemented!";
}

void FIXDispatcher::onSequenceReset(const QByteArray& message)
{
    CDebug() << "SequenceReset type=\"4\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"36");
    if( tag.empty() )
        CDebug(false) << "error: tag \"NewSeqNo\":36 is empty";
    else
        CDebug(false) << "tag \"NewSeqNo\":36=" << tag.c_str();

    int newSeq = atol(tag.c_str());

    tag = getField(message,"123");
    if( !tag.empty() )
        CDebug(false) << "tag \"GapFillFlag\":123=" << tag.c_str();

    if( "tag" == "N" ) {
        ResetMsgSeqNum(0);
        CDebug(false) << "Reset \"MsgSeqNum\"";
    }
    else {
        ResetMsgSeqNum(newSeq-1);
        CDebug(false) << "Change \"MsgSeqNum\", this GapFill message";
    }
}

void FIXDispatcher::onMarketData(const QByteArray& message)
{
    CDebug() << "MarketDataSnapshotFullRefresh type=\"W\" received:";
    CDebug(false) << "<< " << message;

    string symb = getField(message,"262");
    if( symb.empty() )
        CDebug(false) << "error: tag \"MDReqID\":262 is empty";

    string code = getField(message,"48");
    if( code.empty() )
        CDebug(false) << "error: tag \"	SecurityID\":44 is empty";

    string entries = getField(message,"268").c_str();
    if( entries.empty() )
        CDebug(false) << "error: tag \"NoMDEntries\":268 is empty";

    QString bid, ask;
    int num = atol(entries.c_str());
    for(int n = 0; n < num; n++)
    {
        string type = getField(message,"269",n);
        if( type.empty() ) {
            CDebug(false) << "error: tag \"MDEntryType\":269[" << n << "] is empty";
            continue;
        }

        string price = getField(message,"270",n);
        if( price.empty() )
            CDebug(false) << "warning: tag \"MDEntryPx\":270[" << n << "] is empty";
        if(type == "0")
            bid = price.c_str();
        else if(type == "1")
            ask = price.c_str();
        else {
            CDebug(false) << "error: tag \"MDEntryType\":269[" << n << "] has value '" << type.c_str() << "', not '0':Bid or '1':Ask";
            break;
        }
    }

    if( bid.isEmpty() && ask.isEmpty() ) {
        CDebug(false) << "warning: new market response has no info.";
        return;
    }


    QWriteLocker g(&cacheLock_);
    MarketReport* dest = snapshotDelegate(symb.c_str(), code.c_str());
    if( NULL == dest ) {
        CDebug(false) << "warning: request for \"" << symb.c_str() << "\" not found in history: create new entry.";
        return;
    }

    dest->status_.clear();

    int i = 0;
    if(!ask.isEmpty()) {
        if( !dest->ask_.isEmpty() && dest->ask_ != ask )
            i = 1;
        dest->ask_ = ask;
    }

    if(!bid.isEmpty()) {
        if( !dest->bid_.isEmpty() && dest->bid_ != bid )
            i |= 2;
        dest->bid_ = bid;
    }

    switch(i) {
    case 0:
        dest->status_ = "OK"; break;
    case 1:
        dest->status_ = "Ask"; break;
    case 2:
        dest->status_ = "Bid"; break;
    case 3:
        dest->status_ = "Bid and Ask"; break;
    }

    // update response time only for requested sources
    if( dest->requestTime_ && dest->responseTime_ == 0 )
        dest->responseTime_ = Global::time();

    if( i & 1 )
        outerAddQuoteAsk(symb.c_str(), ask.toDouble());
    if( i & 2 )
        outerAddQuoteBid(symb.c_str(), bid.toDouble());

    scheduler_->activateMarketResponse(dest->symbol_, dest->code_);
}

void FIXDispatcher::onMarketDataReject(const QByteArray& message)
{
    CDebug() << "MarketDataRequestReject type  type=\"Y\" received:";
    CDebug(false) << "<< " << message;

    QString symbol = getField(message,"262").c_str();
    if( symbol.isEmpty() )
        CDebug(false) << "error: tag tag \"MDReqID\":262 is empty";

    QString reason = getField(message,"58").c_str();
    if( !reason.isEmpty() )
        CDebug(false) << "tag 58=" << reason;

    string errcode = getField(message,"281");
    QString qErrcod( errcode.c_str() );
    if( !errcode.empty() ) 
    {
        switch(errcode.c_str()[0]) {
            case '0':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Unknown symbol\""; break;
            case '1':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Duplicate MDReqID\""; break;
            case '4':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Unsupported SubscriptionRequestType\""; break;
            case '5':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Unsupported MarketDepth\""; break;
            case '6':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Unsupported MDUpdateType\""; break;
            case '8':
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " \"Unsupported MDEntryType\""; break;
            default:
                CDebug() << "tag \"MDReqRejReason\":281=" << qErrcod << " (Other)"; break;
        }
    }
    
    if( symbol.isEmpty() )
        return;

    QString code = ini_.originCodeBySymbol(symbol);
    if( code.isEmpty() )
        CDebug() << "error: instrument \"" << symbol << "\" not found in INI configuration. Check the settings please!";

    QWriteLocker g(&cacheLock_);
    MarketReport* report = snapshotDelegate(symbol, code);
    if( NULL == report ) {
        CDebug() << "error: request on \"" << symbol << "\":" << code << " not found in history!";
        return;
    }

    // update response time only for requested sources
    if( report->requestTime_ && report->responseTime_ == 0 )
        report->responseTime_ = Global::time();

    if( reason.isEmpty() )
        report->status_   = "Rej_Y 281=" + qErrcod;
    else
        report->status_   = "Rej_Y " + reason;

    scheduler_->activateMarketResponse(symbol, code);
}

void FIXDispatcher::onSessionReject(const QByteArray& message)
{
    CDebug() << "SessionReject type=\"3\" received:";
    CDebug(false) << "<< " << message;

    QString reason = getField(message,"58").c_str();
    if( !reason.isEmpty() )
        CDebug(false) << "tag \"Text\":58=" << reason;

    QString rejCode = getField(message,"373").c_str();
    if( !rejCode.isEmpty() )
        CDebug(false) << "tag \"SessionRejectReason\":373=" << rejCode;

    int seqnum = atol( getField(message,"45").c_str() );
    if( seqnum <= 0 ) {
        CDebug(false) << "error: tag \"MsgSeqNum\":45 is empty";
        return;
    }

    QWriteLocker g(&cacheLock_);
    iterator It = begin();

    for(; It != end(); ++It)
        if( It->msgSeqNum_ == seqnum )
            break;
    if( It == end() ) {
        CDebug(false) << "error: request with \"MsgSeqNum\"=" << seqnum << " not found in history";
        return;
    }

    MarketReport& r = *It;
    // update response time only for requested sources
    if( r.requestTime_ && r.responseTime_ == 0 )
        r.responseTime_ = Global::time();

    if( reason.isEmpty() )
        r.status_ = "Rej_3 373=" + rejCode;
    else
        r.status_ = "Rej_3 " + reason;
        
    scheduler_->activateMarketResponse(r.symbol_, r.code_);
}

MarketReport* FIXDispatcher::snapshotDelegate(const QString& symbol, const QString& code)
{
    MarketReport report;
    report.code_ = code;
    report.symbol_ = symbol;

    int idx = indexOf(report);
    if( idx == -1 )
        return NULL;
    
    move(idx, 0);
    return &front();
}

MarketReport* FIXDispatcher::snapshotDelegate(const char* symbol, const char* code)
{
    MarketReport report;
    report.code_   = code;
    report.symbol_ = symbol;

    int idx = indexOf(report);
    if( idx == -1 )
        return NULL;

    move(idx, 0);
    return &front();
}

const MarketReport* FIXDispatcher::snapshot(const QString& symbol, const QString& code) const
{
    MarketReport report;
    report.code_ = code;
    report.symbol_ = symbol;

    QReadLocker g(const_cast<QReadWriteLock*>(&cacheLock_));
    int idx = indexOf(report);
    if(idx == -1)
    {
        // drop old references
        if( LoggedIn() )
        {
            g.unlock();

            const_cast<FIXDispatcher*>(this)->removeCached(symbol, code);
            scheduler_->activateMarketRequest(symbol, code);
        }
        return NULL;
    }
    return &at(idx);
}

int FIXDispatcher::enqueueAll()
{
    int countOf = ini_.mountedCount();
    for(int orderNum = 0; orderNum < countOf; ++orderNum)
    {
        QString symb = ini_.mountedBySortingOrder(orderNum);
        QString code = ini_.mountedCodeBySymbol(symb);

        QByteArray message = MakeNewRequest(symb, code);
        if( !message.isEmpty() )
            outgoing_.push_back(message);
        else
            --countOf;
    }
    return countOf;
}

QByteArray FIXDispatcher::MakeNewRequest(const QString& symbol, const QString& code)
{
    CDebug() << "MakeNewRequest: \"" << symbol << ":" << code + "\"";
    QByteArray message = MakeMarketDataRequest(symbol.toStdString().c_str(),code.toStdString().c_str());
    if( message.isEmpty() ) {
        CDebug() << "MakeNewRequest error: created <null> message";
        return NULL;
    }

    int seqNum = atol(getField(message,"34").c_str());

    QWriteLocker g(&cacheLock_);
    MarketReport* newr = snapshotDelegate(symbol.toStdString().c_str(), code.toStdString().c_str());
    if( newr ) {
        newr->status_ = "Request";
        newr->requestTime_  = Global::time();
        newr->responseTime_ = 0;
        newr->msgSeqNum_ = seqNum;
        return message;
    }

    MarketReport t;
    t.symbol_ = symbol;
    t.code_ = code;
    t.status_ = "Request";
    t.responseTime_ = 0;    // must be 0, because each MarketDataSnapshot will be checked 
                            // is this message real response or update on subscription
    t.requestTime_ = Global::time();
    t.msgSeqNum_ = seqNum;
    push_front(t);
    return message;
}

void FIXDispatcher::removeCached(const QString& symbol, const QString& code)
{
    QWriteLocker gw(&cacheLock_);

    ReportListT::iterator It = begin();
    while( It != end() )
    {
        if(It->symbol_ == symbol || It->code_ == code)
        {
            It = erase(It);
            continue;
        }
        ++It;
    }
}
