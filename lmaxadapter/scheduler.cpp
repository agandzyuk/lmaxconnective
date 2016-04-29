#include "globals.h"

#include "scheduler.h"
#include "netmanager.h"
#include "quotestablemodel.h"

#include <QtCore>

////////////////////////////////////////////////////////////////////////////////////
class RequestTimer: public QTimer {
public:
    RequestTimer(Scheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((Scheduler*)parent())->exec(Scheduler::RequestTimerID); 
    }
};

class ResponseTimer: public QTimer {
public:
    ResponseTimer(Scheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((Scheduler*)parent())->exec(Scheduler::ResponseTimerID); 
    }
};

class ReconnectTimer: public QTimer {
public:
    ReconnectTimer(Scheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((Scheduler*)parent())->exec(Scheduler::ReconnectTimerID); 
    }
};

class HeartbeatTimer: public QTimer {
public:
    HeartbeatTimer(Scheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) {
        stop(); ((Scheduler*)parent())->exec(Scheduler::HeartbeatTimerID); 
    }

};

class TestrequestTimer: public QTimer {
public:
    TestrequestTimer(Scheduler* parent) : QTimer(parent) {}
    void timerEvent(QTimerEvent* e) { 
        stop(); ((Scheduler*)parent())->exec(Scheduler::TestrequestTimerID); 
    }
};

////////////////////////////////////////////////////////////////////////////////////
Scheduler::Scheduler(QObject* parent)
    : QObject(parent),
    requeste_(new RequestTimer(this) ),
    response_(new ResponseTimer(this) ),
    reconnect_(new ReconnectTimer(this) ),
    heartbeat_(new HeartbeatTimer(this) ),
    testrequest_(new TestrequestTimer(this) ),
    reconnectInterval_(0),
    allowReconnect_(true)
{}

Scheduler::~Scheduler()
{
    requeste_->stop();
    response_->stop();
    reconnect_->stop();
    heartbeat_->stop();
    testrequest_->stop();
}

bool Scheduler::activateSSLReconnect()
{
    if( !allowReconnect_ )
        return false;

    if( manager()->getState() == ClosedRemoteState )
        reconnectInterval_ = 0;
    else
        reconnectInterval_ = 2000;

    reconnect_->setSingleShot(true);
    reconnect_->start(reconnectInterval_);
    return true;
}

void Scheduler::activateHeartbeat(qint32 ms)
{
    heartbeat_->setSingleShot(true);
    heartbeat_->start(ms);
}

void Scheduler::activateTestrequest(qint32 ms)
{
    testrequest_->setSingleShot(true);
    testrequest_->start(ms);
}

void Scheduler::activateLogout()
{
    {
        QMutexLocker g(&guardReqQueue_);
        reqQueue_.clear();
    }
    requeste_->setSingleShot(true);
    requeste_->start();
}

void Scheduler::activateRequest(const Instrument& inst)
{
    QMutexLocker g(&guardReqQueue_);

    // remove all with similar symbols OR symb codes, 
    // insert ordinal last request instead
    SPairQueue::iterator It;
    do {
        It = qFind(reqQueue_.begin(), reqQueue_.end(), inst);
        if( It == reqQueue_.end() )
            break;
        It = reqQueue_.erase(It);
    }
    while(It != reqQueue_.end());
    reqQueue_.push_back(inst);

    requeste_->setSingleShot(true);
    requeste_->start();
}

void Scheduler::activateResponse(const Instrument& inst)
{
    QMutexLocker g(&guardRespQueue_);
    
    // remove all with similar symbols OR symb codes, 
    // insert ordinal last response instead
    SPairQueue::iterator It;
    do {
        It = qFind(respQueue_.begin(), respQueue_.end(), inst);
        if( It == respQueue_.end() )
            break;
        It = respQueue_.erase(It);
    }
    while(It != respQueue_.end());
    respQueue_.push_back(inst);

    response_->setSingleShot(true);
    response_->start();
}

void Scheduler::setReconnectEnabled(int on)
{
    allowReconnect_ = on;
}

void Scheduler::exec(TimerID id)
{
    if( id == RequestTimerID)
        onRequestEvent();
    if( id == ResponseTimerID)
        onResponseEvent();
    else if( id == ReconnectTimerID)
        onReconnectEvent();
    else if( id == HeartbeatTimerID)
        onHeartbeatEvent();
    else if( id == TestrequestTimerID)
        onTestrequestEvent();
}

void Scheduler::onRequestEvent()
{
    NetworkManager* mgr = manager();
    if(mgr == NULL)
        return;

    Instrument inst;
    {
        QMutexLocker g(&guardReqQueue_);
        if( reqQueue_.isEmpty() )
            return;

        inst = *reqQueue_.begin();
        reqQueue_.pop_front();
    }

    if( inst.first.substr(0,8) == "Disable_")
        mgr->onHaveToUnSubscribe(Instrument(inst.first.c_str()+8,inst.second));
    else
        mgr->onHaveToSubscribe(inst);

    QMutexLocker g(&guardReqQueue_);
    if(!requeste_->isActive() && !reqQueue_.isEmpty()) {
        requeste_->setSingleShot(true);
        requeste_->start();
    }
}

void Scheduler::onResponseEvent()
{
    NetworkManager* mgr = manager();
    if(mgr == NULL)
        return;

    Instrument inst;
    {
        QMutexLocker g(&guardRespQueue_);
        if( respQueue_.isEmpty() )
            return;

        inst = *respQueue_.begin();
        respQueue_.pop_front();
    }

    if( inst.first == "FullUpdate" )
        emit model()->onInstrumentUpdate();
    else
        emit model()->onInstrumentUpdate(inst);

    QMutexLocker g(&guardRespQueue_);
    if(!response_->isActive() && !respQueue_.isEmpty()) {
        response_->setSingleShot(true);
        response_->start();
    }
}

void Scheduler::onReconnectEvent()
{
    NetworkManager* mgr = manager();
    if(NULL == mgr || !allowReconnect_)
        return;

    if( mgr->getState() != ForcedClosingState ) {
        CDebug() << "Reconnect: passed " << reconnectInterval_ << " msecs";
        QTimer::singleShot(0, mgr->parent(), SLOT(asyncStart()) );
    }
}

void Scheduler::onHeartbeatEvent()
{
    NetworkManager* mgr = manager();
    if( mgr )
        mgr->onHaveToHeartbeat();
}

void Scheduler::onTestrequestEvent()
{
    NetworkManager* mgr = manager();
    if( mgr )
        mgr->onHaveToTestRequest();
}

NetworkManager* Scheduler::manager() const
{
    NetworkManager* netman = qobject_cast<NetworkManager*>(parent());
    if( NULL == netman ) {
        CDebug() << "Scheduler: Error of NetworkManager qobject_cast";
        return NULL;
    }
    return netman;
}

MarketAbstractModel* Scheduler::model() const
{
    NetworkManager* netman = manager();
    if( netman )
        return netman->model();
    return NULL;
}
