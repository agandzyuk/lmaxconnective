#include "globals.h"
#include "fixdatamodel.h"
#include "baseini.h"
#include "scheduler.h"
#include "fixlogger.h"
#include "mqlproxyserver.h"

#include <QReadWriteLock>

using namespace std;

//////////////////////////////////////////////////////////////
FixDataModel::FixDataModel(QSharedPointer<MqlProxyServer>& mqlProxy, QWidget* parent) 
    : SymbolsModel(parent),
    cacheLock_(new QReadWriteLock()),
    mqlProxy_(mqlProxy)
{
    setIniModel(dynamic_cast<const BaseIni*>(this));
    fixlog_.reset(new FixLog(this));
}

FixDataModel::~FixDataModel()
{
    { QWriteLocker g(cacheLock_); }
    delete cacheLock_;
    cacheLock_ = NULL;
}

int FixDataModel::process(const QByteArray& message)
{
    lastIncomingTime_ = Global::time();

    string value = getField(message, "35");
    switch(value[0])
    {
    case 'A':
        onLogon(message);
        break;
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
        setLoggedIn(false);
        return -1;
    case 'W':
        onMarketData(message);
        break;
    case 'Y':
        onMarketDataReject(message);
        break;
    default:
        CDebug() << "Warning: unexpected message type=\"" << value.c_str() << "\" received";
        CDebug(false) << "<< " << message;
        break;
    }
    return 0;
}

void FixDataModel::beforeLogout()
{
    clearCache();
    mqlClearPrices();
}

void FixDataModel::onLogon(const QByteArray& message)
{
    CDebug() << "Logon type=\"A\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"49");
    if( !tag.empty() )
        CDebug(false) << "tag \"SenderCompID\":49=" << tag.c_str();

    tag = getField(message,"56");
    if( !tag.empty() )
        CDebug(false) << "tag \"TargetCompID\":56=" << tag.c_str();

    msglog().inmsg(message);
    setLoggedIn(true);
    activateMonitoring();
}

void FixDataModel::onLogout(const QByteArray& message)
{
    CDebug() << "Logout type=\"5\" received:";
    CDebug(false) << "<< " << message;

    string reason = getField(message,"58");
    if( !reason.empty() )
        CDebug(false) << "tag \"Reason\":58=" << reason.c_str();

    msglog().inmsg(message);

    if( !reason.empty() )
        serverLogoutError(reason);
    else
        beforeLogout();
}

void FixDataModel::onHeartbeat(const QByteArray& message)
{
    string tag = getField(message,"112");
    if( !tag.empty() ) {
        CDebug() << "Heartbeat type=\"0\" on TestReqID=\"" << tag.c_str() << "\" received:";
        CDebug(false) << "<< " << message;
    }
}

void FixDataModel::onTestRequest(const QByteArray& message)
{
    CDebug() << "TestRequest type=\"1\" received:";
    CDebug(false) << "<< " << message;

    string reqId = getField(message,"112");
    msglog().inmsg(message);
    if( !reqId.empty() ) {
        CDebug(false) << "tag \"TestReqID\":112=" << reqId.c_str();
        outgoing_.push_back( makeOnTestRequest(reqId.c_str()) );
    }
    else
        CDebug(false) << "Error corrupted message: tag \"TestReqID\":112 is empty";
}

void FixDataModel::onResendRequest(const QByteArray& message)
{
    CDebug() << "ResendRequest type=\"2\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"7");
    if( tag.empty() )
        CDebug(false) << "Error corrupted message: tag \"BeginSeqNo\":7 is empty";
    else
        CDebug(false) << "tag \"BeginSeqNo\":7=" << tag.c_str();

    tag = getField(message,"16");
    if( tag.empty() )
        CDebug(false) << "Error corrupted message: tag \"EndSeqNo\":16 is empty";
    else
        CDebug(false) << "tag \"EndSeqNo\":16=" << tag.c_str();

    msglog().inmsg(message);
    CDebug() << "Warning: behaviour on ResendRequest is not implemented!";
}

void FixDataModel::onSequenceReset(const QByteArray& message)
{
    CDebug() << "SequenceReset type=\"4\" received:";
    CDebug(false) << "<< " << message;

    string tag = getField(message,"36");
    msglog().inmsg(message);
    if( tag.empty() ) {
        CDebug(false) << "Error corrupted message: tag \"NewSeqNo\":36 is empty";
        return;
    }

    int newSeq = atol(tag.c_str());

    tag = getField(message,"123");
    if( !tag.empty() )
        CDebug(false) << "tag \"GapFillFlag\":123=" << tag.c_str();

    if( "tag" == "N" ) {
        resetMsgSeqNum(0);
        CDebug(false) << "Reset \"MsgSeqNum\"";
    }
    else {
        resetMsgSeqNum(newSeq-1);
        CDebug(false) << "Change \"MsgSeqNum\", this GapFill message";
    }
}

void FixDataModel::onMarketData(const QByteArray& message)
{
    CDebug() << "MarketDataSnapshotFullRefresh type=\"W\" received";
    string sym = getField(message,"262");
    if( sym.empty() ) {
        CDebug(false) << "Error corrupted message: tag \"MDReqID\":262 is empty";
        return;
    }

    qint32 code  = atol(getField(message,"48").c_str());
    if( code == 0 ) {
        CDebug(false) << "Error corrupted message: tag \"SecurityID\":48 is empty";
        return;
    }

    msglog().inmsg(message, sym.c_str(), code);

    if( !isMonitoringEnabled() )
        return;

    // Case when we changed symbol name from GUI
    Instrument instrument(sym, code);
    string symToBeRenew;
    if( !isMonitored(instrument) ) 
    {
        string mayRenew = getSymbol(code);
        if( !mayRenew.empty() ) 
            sym = mayRenew;
        else {
            CDebug(false) << "Monitoring is disabled for \"" << sym.c_str() << "\": message ignored.";
            return;
        }
    }

    CDebug(false) << "<< " << message;

    string entries = getField(message,"268").c_str();
    if( entries.empty() ) {
        CDebug(false) << "Error corrupted message: tag \"NoMDEntries\":268 is empty";
        return;
    }

    bool response_noinfo = true;
    string bid, ask;
    int num = atol(entries.c_str());
    for(int n = 0; n < num; n++)
    {
        string type = getField(message,"269",n);
        if( type.empty() ) {
            CDebug(false) << "Error corrupted message: tag \"MDEntryType\":269[" << n << "] is empty";
            return;
        }
        string price = getField(message,"270",n);
        if( price.empty() ) {
            CDebug(false) << "\"MDEntryPx\":270[" << n << "] is empty";
            continue;
        }

        response_noinfo = false;

        if(type[0] == char('0'))
            bid = price;
        else if(type[0] == char('1'))
            ask = price;
        else {
            CDebug(false) << "Error corrupted message: tag \"MDEntryType\":269[" << n << "] has value '" << type.c_str() << "', not '0':Bid or '1':Ask";
            return;
        }
    }
    response_noinfo |= (bid.empty() && ask.empty());

    QSharedPointer<QWriteLocker> autolock;
    Snapshot* dest = snapshotDelegate(sym.c_str(), code, autolock);
    if( NULL == dest ) {
        CDebug(false) << "Warning: request for \"" << sym.c_str() << "\" not found in cache.";
        return;
    }

    // set request time to delta msecs only after requested source
    dest->responseTime_ = Global::time();
    if( dest->statuscode_ == Snapshot::StatSubscribe )
        dest->requestTime_ = dest->responseTime_ - dest->requestTime_;

    if(dest->statuscode_ == Snapshot::StatUnSubscribed || response_noinfo) 
    {
        dest->description_ = "Inactive on Exchange";
        dest->statuscode_  = Snapshot::StatUnSubscribed;
        autolock.reset();
        emit activateResponse(instrument);
        return;
    }

    dest->statuscode_ = Snapshot::StatNoChange;
    if(!ask.empty()) {
        if( !dest->ask_.isEmpty() && dest->ask_ != ask.c_str() )
            dest->statuscode_ = Snapshot::StatAskChange;
        dest->ask_ = ask.c_str();
    }

    if(!bid.empty()) {
        if( !dest->bid_.isEmpty() && dest->bid_ != bid.c_str() )
            dest->statuscode_ = (Snapshot::Status)(dest->statuscode_ | Snapshot::StatBidChange);
        dest->bid_ = bid.c_str();
    }
    switch(dest->statuscode_) {
    case Snapshot::StatNoChange:
        dest->description_ = "OK no changes"; break;
    case Snapshot::StatAskChange:
        dest->description_ = "Ask changed"; break;
    case Snapshot::StatBidChange:
        dest->description_ = "Bid changed"; break;
    case Snapshot::StatBidAndAskChange:
        dest->description_ = "Bid&Ask changed"; break;
    }

    // unlock region
    string copysym  = sym, copybid = bid, copyask = ask;
    autolock.reset();

    mqlSendQuotes(copysym, copybid, copyask);
    emit activateResponse(instrument);
}

void FixDataModel::onMarketDataReject(const QByteArray& message)
{
    CDebug() << "MarketDataRequestReject type  type=\"Y\" received:";

    string sym = getField(message,"262");
    if( sym.empty() ) {
        CDebug(false) << "Error corrupted message: tag tag \"MDReqID\":262 is empty";
        return;
    }

    qint32 code = getCode(sym.c_str());
    msglog().inmsg(message, sym.c_str(), code);

    if( !isMonitoringEnabled() )
        return;

    Instrument instrument(sym,code);
    if( !isMonitored(instrument) ) {
        CDebug(false) << "Monitoring is disabled for instrument \"" << sym.c_str() << "\": message ignored.";
        return;
    }
    CDebug(false) << "<< " << message;

    QString reason = getField(message,"58").c_str();
    const char* stder = getField(message,"281").c_str();
    QString qErrcod = stder;
    if( !qErrcod.isEmpty() ) 
    {
        switch(stder[0]) {
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

    QSharedPointer<QWriteLocker> autolock;
    Snapshot* dest = snapshotDelegate(sym.c_str(), code, autolock);
    if( NULL == dest ) {
        CDebug(false) << "Warning: request for \"" << sym.c_str() << "\" not found in cache.";
        return;
    }

    // set request time to delta msecs only after requested source
    dest->responseTime_ = Global::time();
    if( dest->statuscode_ == Snapshot::StatSubscribe )
        dest->requestTime_ = dest->responseTime_ - dest->requestTime_;

    dest->statuscode_ = Snapshot::StatBusinessReject;
    if( reason.isEmpty() )
        dest->description_ = "Reject by \"Y\", 281=\"" + qErrcod + "\"";
    else
        dest->description_ = "Reject by \"Y\", \"" + reason + "\"";

    autolock.reset();
    emit activateResponse(instrument);
}

void FixDataModel::onSessionReject(const QByteArray& message)
{
    CDebug() << "SessionReject type=\"3\" received:";
    if( !isMonitoringEnabled() )
        return;

    CDebug(false) << "<< " << message;

    QString reason = getField(message,"58").c_str();
    QString rejCode = getField(message,"373").c_str();
    qint32 seqnum = atol( getField(message,"45").c_str() );
    if( seqnum <= 0 ) {
        CDebug(false) << "Error corrupted message: tag \"MsgSeqNum\":45 is invalid";
        return;
    }

    QSharedPointer<QWriteLocker> autolock;
    Snapshot* dest = snapshotDelegate(seqnum, autolock);
    if( NULL == dest ) {
        CDebug(false) << "Error corrupted message: request with \"MsgSeqNum\"=" << seqnum << " not found in cache";
        return;
    }

    // set request time to delta msecs only after requested source
    dest->responseTime_ = Global::time();
    if( dest->statuscode_ == Snapshot::StatSubscribe )
        dest->requestTime_ = dest->responseTime_ - dest->requestTime_;

    dest->statuscode_ = Snapshot::StatSessionReject;
    if( reason.isEmpty() )
        dest->description_ = "Reject by \"3\", 373=\"" + rejCode + "\"";
    else
        dest->description_ = "Reject by \"3\", \"" + reason + "\"";

    Instrument instrument = dest->instrument_;
    msglog().inmsg(message, instrument.first.c_str(), instrument.second);

    autolock.reset();
    
    if( !isMonitored(instrument) )
        CDebug(false) << "Monitoring is disabled for instrument \"" << instrument.first.c_str() << "\": message ignored.";
    else
        emit activateResponse(instrument);
}

Snapshot* FixDataModel::snapshotDelegate(const char* symbol, qint32 code, QSharedPointer<QWriteLocker>& autolock)
{
    // on first ReadLock simply to find in the cache
    Snapshot* snap = NULL;
    {
        QReadLocker g(cacheLock_);
        snap = cache_[code];
        if( snap == NULL )
            return snap;
    }
    
    // create the write locker what should be destroyed above of the method
    autolock.reset(new QWriteLocker(cacheLock_));
    return snap;
}

Snapshot* FixDataModel::snapshotDelegate(qint32 msgSeqNum, QSharedPointer<QWriteLocker>& autolock)
{
    // on first ReadLock simply to find in the cache
    Snapshot* snap = NULL;
    {
        QReadLocker g(cacheLock_);
        qint32 code;
        SeqnumToSymT::const_iterator seq_It = seqnumMap_.find(msgSeqNum);
        if( seq_It != seqnumMap_.end() && -1 != (code = getCode(seq_It.value().c_str())) )
        {
            SnapshotSet& cache = *const_cast<SnapshotSet*>(&cache_);
            snap = cache[code];
        }
    }
    
    // create the write locker what should be destroyed above of the method
    if( snap )
        autolock.reset(new QWriteLocker(cacheLock_));
    return snap;
}

Snapshot* FixDataModel::getSnapshot(const char* sym, QSharedPointer<QReadLocker>& autolock) const
{
    qint32 code = getCode(sym);
    if( code == -1 ) // is compatible?
        return NULL;

    if( autolock.isNull() )
        autolock.reset(new QReadLocker(cacheLock_));

    SnapshotSet& cache = *const_cast<SnapshotSet*>(&cache_);
    return cache[code];
}

void FixDataModel::activateMonitoring()
{
    qint16 countOf = monitoredCount();
    for(qint16 row = 0; row < countOf; ++row) 
    {
        Instrument inst = getByOrderRow(row);
        QSharedPointer<QReadLocker> autolock;
        Snapshot* sp = getSnapshot(inst.first.c_str(), autolock);
        if( sp && sp->statuscode_ & Snapshot::StatUnSubscribed )
            continue;
        autolock.reset();
        emit activateRequest(inst);
    }
}

QByteArray FixDataModel::makeSubscribe(const Instrument& inst)
{
    const char* symbol = inst.first.c_str();
    qint32 code = inst.second;
    CDebug() << "makeSubscribe: \"" << symbol << ":" << code << "\"";

    QSharedPointer<QWriteLocker> autolock;
    Snapshot* exsp = snapshotDelegate(symbol, code, autolock);
    if( exsp && loggedIn() ) {
        exsp->statuscode_   = Snapshot::StatSubscribe;
        exsp->requestTime_  = Global::time();
        exsp->responseTime_ = 0;
        exsp->description_  = "Subscribing";
        return makeMarketSubscribe(symbol, code);
    }
    else if( exsp && !loggedIn() )
    {
        exsp->statuscode_   = Snapshot::StatNoChange;
        exsp->responseTime_ = exsp->requestTime_ = 0;
        exsp->description_  = "Subscribed";
        return QByteArray();
    }

    QByteArray message;
    Snapshot snap;
    snap.instrument_.first  = symbol;
    snap.instrument_.second = code;
    snap.responseTime_ = snap.requestTime_  = 0;

    if( loggedIn() ) {
        snap.requestTime_  =  Global::time();
        snap.statuscode_   = Snapshot::StatSubscribe;
        snap.description_  = "Subscribing";
        cache_.insert(snap);
        setMonitoring(inst, true, false);
        message = makeMarketSubscribe(symbol, code);
    }
    else {
        snap.statuscode_   = Snapshot::StatNoChange;
        snap.description_  = "Subscribed";
        cache_.insert(snap);
    }
    return message;
}

QByteArray FixDataModel::makeUnSubscribe(const Instrument& inst)
{
    const char* symbol = inst.first.c_str();
    qint32 code = inst.second;
    CDebug() << "makeUnSubscribe: \"" << symbol << ":" << code << "\"";

    QSharedPointer<QWriteLocker> autolock;
    Snapshot* exsp = snapshotDelegate(symbol, code, autolock);
    if( exsp)
    {
        exsp->statuscode_ = Snapshot::StatUnSubscribed;
        exsp->description_  = "UnSubscribed";
        exsp->requestTime_  = exsp->responseTime_ = 0;
    }
    else 
    {
        Snapshot snap;
        snap.instrument_.first  = symbol;
        snap.instrument_.second = code;
        snap.statuscode_   = Snapshot::StatUnSubscribed;
        snap.description_  = "UnSubscribed";
        snap.responseTime_ = snap.requestTime_  = 0;
        cache_.insert(snap);
    }
    return loggedIn() ? makeMarketUnSubscribe(symbol, code) : QByteArray();
}

void FixDataModel::clearCache()
{
    QWriteLocker g(cacheLock_);
    QSet<Snapshot>::iterator It = cache_.begin();
    while( It != cache_.end() ) {
        Instrument copy = It->instrument_;
        It = cache_.erase(It->instrument_.second);
    }
    seqnumMap_.clear();
    g.unlock();

    emit activateResponse(Instrument("FullUpdate",-1));
}

void FixDataModel::serverLogoutError(const std::string& reason)
{
    QVector<string> syms;
    getSymbolsUnderMonitoring(syms);
    
    for(QVector<string>::iterator It = syms.begin(); 
        It != syms.end(); ++It)
    {
        Instrument inst(*It,getCode(It->c_str()));

        QSharedPointer<QWriteLocker> autolock;
        Snapshot* dest = snapshotDelegate(inst.first.c_str(), inst.second, autolock);
        if( dest == NULL ) {
            Snapshot snap;
            snap.instrument_= inst;
            snap.description_ = reason.c_str();
            snap.statuscode_ = Snapshot::StatSessionReject;
            snap.requestTime_ = snap.responseTime_ = 0;
            cache_.insert(snap);
        }
        else {
            dest->bid_.clear(); 
            dest->ask_.clear();
            dest->description_ = reason.c_str();
            dest->statuscode_ = Snapshot::StatSessionReject;
            dest->requestTime_ = dest->responseTime_ = 0;
        }
        autolock.reset();
    }
    emit notifyReconnectSetCheck(Qt::Unchecked);
    emit activateResponse(Instrument("FullUpdate",-1));
}

void FixDataModel::storeRequestSeqnum(const QByteArray& requestMessage, const char* symbol)
{
    qint32 seqNum = atol(getField(requestMessage,"34").c_str());
    if( seqNum < 1 ) 
        return;

    if( symbol && strlen(symbol) ) {
        QWriteLocker g(cacheLock_);
        seqnumMap_.insert(seqNum,symbol);
        return;
    }

    string sym = getField(requestMessage,"262");
    if( !sym.empty() )  {
        QWriteLocker g(cacheLock_);
        seqnumMap_.insert(seqNum,symbol);
    }
}

void FixDataModel::removeCached(qint32 byCode)
{
    {
        QReadLocker g(cacheLock_);
        Snapshot* snap = cache_[byCode];
        if( snap ) 
        {
            std::string sym = snap->instrument_.first;
            for(SeqnumToSymT::iterator It = seqnumMap_.begin(); It != seqnumMap_.end(); ++It )
                if( It.value() == sym ) {
                    seqnumMap_.erase(It);
                    break;
                }
            mqlSendQuotes(sym, "0", "0");
        }
        else
            return;
    }

    QWriteLocker g(cacheLock_);
    cache_.erase(byCode);
}

void FixDataModel::mqlClearPrices()
{
    // send nulls instead prices
    QVector<string> monitored;
    getSymbolsUnderMonitoring(monitored);
    MqlProxyQuotes* transaction = (MqlProxyQuotes*)malloc(2+monitored.size()*sizeof(MqlProxyQuotes::Quote));
    transaction->numOfQuotes_ = monitored.size();
    for(int i = 0; i < monitored.size(); ++i) {
        transaction->quotes_[i].ask_ = 0;
        transaction->quotes_[i].bid_ = 0;
        strcpy_s(transaction->quotes_[i].symbol_, MAX_SYMBOL_LENGTH, monitored[i].c_str());
    }
    mqlProxy_->sendMessageBroadcast((const char*)transaction);

    free(transaction);
}

void FixDataModel::mqlSendQuotes(const string& sym, const string& bid, const string& ask)
{
    //QThread::msleep(65);

/*    CDebug() << "FixDataModel::mqlSendQuotes \"" << sym.c_str() 
             << "\": ask=" << ask.c_str() << ", bid=" << bid.c_str();
*/
    MqlProxyQuotes transaction;
    transaction.numOfQuotes_ = 1;
    strcpy_s(transaction.quotes_[0].symbol_, MAX_SYMBOL_LENGTH, sym.c_str());
    if( ask.empty() )
        transaction.quotes_[0].ask_ = -1;
    else
        Global::reinterpretDouble(ask.c_str(), &transaction.quotes_[0].ask_);
    if( bid.empty() )
        transaction.quotes_[0].bid_ = -1;
    else
        Global::reinterpretDouble(bid.c_str(), &transaction.quotes_[0].bid_);
    mqlProxy_->sendMessageBroadcast((const char*)&transaction);
}
