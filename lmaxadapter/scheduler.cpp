#include "defines.h"

#include "scheduler.h"
#include "maindialog.h"
#include "fixdispatcher.h"

#include <QtCore>

////////////////////////////////////////////////////////////////////////////////////
class RequestTimer: public QTimer {
public:
    RequestTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((QScheduler*)parent())->exec(QScheduler::RequestTimerID); 
    }
};

class ResponseTimer: public QTimer {
public:
    ResponseTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((QScheduler*)parent())->exec(QScheduler::ResponseTimerID); 
    }
};

class UpdaterTimer: public QTimer {
public:
    UpdaterTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        ((QScheduler*)parent())->exec(QScheduler::UpdaterTimerID); 
    }
};

class ReconnectTimer: public QTimer {
public:
    ReconnectTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((QScheduler*)parent())->exec(QScheduler::ReconnectTimerID); 
    }
};

class HeartbeatTimer: public QTimer {
public:
    HeartbeatTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) {
        stop(); ((QScheduler*)parent())->exec(QScheduler::HeartbeatTimerID); 
    }

};

class TestrequestTimer: public QTimer {
public:
    TestrequestTimer(QScheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((QScheduler*)parent())->exec(QScheduler::TestrequestTimerID); 
    }
};

////////////////////////////////////////////////////////////////////////////////////
QScheduler::QScheduler(QObject* parent)
    : QObject(parent),
    requeste_(new RequestTimer(this) ),
    response_(new ResponseTimer(this) ),
    updater_(new UpdaterTimer(this) ),
    reconnect_(new ReconnectTimer(this) ),
    heartbeat_(new HeartbeatTimer(this) ),
    testrequest_(new TestrequestTimer(this) ),
    reconnectInterval_(0),
    allowReconnect_(true)
{}

QScheduler::~QScheduler()
{
    requeste_->stop();
    response_->stop();
    updater_->stop();
    reconnect_->stop();
    heartbeat_->stop();
    testrequest_->stop();
}

bool QScheduler::activateSSLReconnect()
{
    if( !allowReconnect_ )
        return false;

    if( manager() ) {
        if( manager()->disconnectStatus() == RemoteDisconnect )
            reconnectInterval_ = 0;
        else
            reconnectInterval_ = 2000;
    }

    reconnect_->setSingleShot(true);
    reconnect_->start(reconnectInterval_);
    return true;
}

void QScheduler::activateHeartbeat(qint32 ms)
{
    heartbeat_->setSingleShot(true);
    heartbeat_->start(ms);
}

void QScheduler::activateTestrequest(qint32 ms)
{
    testrequest_->setSingleShot(true);
    testrequest_->start(ms);
}

void QScheduler::activateLogout()
{
    {
        QMutexLocker g(&quardReqQueue_);
        reqQueue_.clear();
    }
    {
        QMutexLocker g(&quardRespQueue_);
        respQueue_.clear();
    }
    requeste_->setSingleShot(true);
    requeste_->start();
}

void QScheduler::activateMarketRequest(const QString& symbol, const QString& code)
{
    QMutexLocker g(&quardReqQueue_);

    // remove all with similar symbols OR symb codes, 
    // insert ordinal last request instead
    SPairsT::iterator It = reqQueue_.begin();
    do {
        It = qFind(It, reqQueue_.end(), SPair(symbol,code));
        if( It == reqQueue_.end() )
            break;
        It = reqQueue_.erase(It);
    }
    while(It != reqQueue_.end());
    reqQueue_.push_back(SPair(symbol,code));

    if( !requeste_->isActive() ) {
        requeste_->setSingleShot(true);
        requeste_->start();
    }
}

void QScheduler::activateMarketResponse(const QString& symbol, const QString& code)
{
    QMutexLocker g(&quardRespQueue_);

    // remove all with similar symbols OR symb codes, 
    // insert ordinal last response instead
    SPairsT::iterator It = respQueue_.begin();
    do {
        It = qFind(It, respQueue_.end(), SPair(symbol,code));
        if( It == respQueue_.end() )
            break;
        It = respQueue_.erase(It);
    }
    while(It != respQueue_.end());
    respQueue_.push_back(SPair(symbol,code));

    if(!response_->isActive()) {
        response_->setSingleShot(true);
        response_->start();
    }
}

void QScheduler::activateSymbolsUpdater(qint32 msPeriod)
{
    updater_->setSingleShot(false);
    updater_->start(msPeriod);
}

void QScheduler::setReconnectEnabled(int on)
{
    allowReconnect_ = on;
}

void QScheduler::exec(TimerID id)
{
    if( id == RequestTimerID)
        onMarketRequestEvent();
    if( id == ResponseTimerID)
        onMarketResponseEvent();
    else if( id == UpdaterTimerID)
        onUpdaterEvent();
    else if( id == ReconnectTimerID)
        onReconnectEvent();
    else if( id == HeartbeatTimerID)
        onHeartbeatEvent();
    else if( id == TestrequestTimerID)
        onTestrequestEvent();
}

void QScheduler::onMarketRequestEvent()
{
    NetworkManager* mgr = manager();
    if(mgr == NULL)
        return;

    QString sym, code;
    {
        QMutexLocker g(&quardReqQueue_);
        if( reqQueue_.isEmpty() )
            return;

        sym  = reqQueue_.begin()->first;
        code = reqQueue_.begin()->second;
        reqQueue_.pop_front();
    }

    mgr->onHaveToMarketRequest(sym,code);

    QMutexLocker g(&quardReqQueue_);
    if(!requeste_->isActive() && !reqQueue_.isEmpty()) {
        requeste_->setSingleShot(true);
        requeste_->start();
    }
}

void QScheduler::onMarketResponseEvent()
{
    FIXDispatcher* disp = fixdispatcher();
    if(disp == NULL)
        return;

    QString sym;
    {
        QMutexLocker g(&quardRespQueue_);
        if( respQueue_.isEmpty() )
            return;

        sym  = respQueue_.begin()->first;
        respQueue_.pop_front();
    }

    disp->onResponseReceived(sym);

    QMutexLocker g(&quardRespQueue_);
    if(!response_->isActive() && !respQueue_.isEmpty()) {
        response_->setSingleShot(true);
        response_->start();
    }
}

void QScheduler::onUpdaterEvent()
{
    NetworkManager* mgr = manager();
    if(mgr)
        mgr->onHaveToSymbolsUpdate();
}

void QScheduler::onReconnectEvent()
{
    NetworkManager* mgr = manager();
    if(NULL == mgr || !allowReconnect_)
        return;

    short status = mgr->disconnectStatus();
    if( status  != ForcedDisconnect ) {
        CDebug() << "Reconnect: passed " << reconnectInterval_ << " msecs";
        mgr->asyncStart(true);
    }
}

void QScheduler::onHeartbeatEvent()
{
    NetworkManager* mgr = manager();
    if( mgr )
        mgr->onHaveToHeartbeat();
}

void QScheduler::onTestrequestEvent()
{
    NetworkManager* mgr = manager();
    if( mgr )
        mgr->onHaveToTestRequest();
}

NetworkManager* QScheduler::manager() const
{
    MainDialog* netman = qobject_cast<MainDialog*>(parent());
    if( NULL == netman ) {
        CDebug() << "QScheduler: Error of NetworkManager qobject_cast";
        return NULL;
    }
    return netman;
}

FIXDispatcher* QScheduler::fixdispatcher() const
{
    NetworkManager* mgr = manager();
    if( mgr )
        return mgr->dispatcher_.data();
    return NULL;
}
