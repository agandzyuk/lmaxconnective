#ifndef __scheduler_h__
#define __scheduler_h__

#include "marketabstractmodel.h"

#include <QTimer>
#include <QSharedPointer>
#include <QMutex>

class NetworkManager;

////////////////////////////////////
class Scheduler : public QObject
{
    Q_OBJECT

    enum TimerID {
        RequestTimerID = 0,
        ResponseTimerID,
        ReconnectTimerID,
        HeartbeatTimerID,
        TestrequestTimerID,
    };

    friend class RequestTimer;
    friend class ResponseTimer;
    friend class ReconnectTimer;
    friend class HeartbeatTimer;
    friend class TestrequestTimer;
public:
    Scheduler(QObject* parent);
    ~Scheduler();

    bool activateSSLReconnect();
    void activateLogout();
    void activateStartDialog();
    void activateHeartbeat(qint32 ms);
    void activateTestrequest(qint32 ms);

    void setReconnectEnabled(int on);
    bool reconnectEnabled() const
    { return allowReconnect_; }

protected slots:
    void activateRequest(const Instrument& inst);
    void activateResponse(const Instrument& inst);
    void exec(TimerID id);

protected:
    void onRequestEvent();
    void onResponseEvent();
    void onReconnectEvent();
    void onHeartbeatEvent();
    void onTestrequestEvent();
    
private:
    NetworkManager* manager() const;
    MarketAbstractModel* model() const;

private:
    bool allowReconnect_;
    int  reconnectInterval_;
    
    class SPair : public Instrument 
    {
    public:
        SPair(){}
        SPair(const Instrument& r): Instrument(r) {}
        SPair(const std::string& first, qint32& second) : Instrument(first, second) {}
        bool operator==(const Instrument& other) const 
        { return (first == other.first); } // we insterested only in symbols dublication
    };
    typedef QVector<SPair> SPairQueue;

    SPairQueue reqQueue_;
    QMutex guardReqQueue_;

    SPairQueue respQueue_;
    QMutex guardRespQueue_;

    QSharedPointer<QTimer> requeste_;
    QSharedPointer<QTimer> response_;
    QSharedPointer<QTimer> reconnect_;
    QSharedPointer<QTimer> heartbeat_;
    QSharedPointer<QTimer> testrequest_;
};

#endif // __scheduler_h__
