#ifndef __scheduler_h__
#define __scheduler_h__

#include <QTimer>
#include <QSharedPointer>
#include <QMutex>
#include <QVector>

class NetworkManager;
class FIXDispatcher;


////////////////////////////////////
class QScheduler : public QObject
{
    Q_OBJECT

    enum TimerID {
        RequestTimerID = 0,
        ResponseTimerID,
        UpdaterTimerID,
        ReconnectTimerID,
        HeartbeatTimerID,
        TestrequestTimerID,
    };

    friend class RequestTimer;
    friend class ResponseTimer;
    friend class UpdaterTimer;
    friend class ReconnectTimer;
    friend class HeartbeatTimer;
    friend class TestrequestTimer;
public:
    QScheduler(QObject* parent);
    ~QScheduler();

    bool activateSSLReconnect();
    void activateLogout();
    void activateStartDialog();
    void activateHeartbeat(qint32 ms);
    void activateTestrequest(qint32 ms);
    void activateMarketRequest(const QString& symbol, const QString& code);
    void activateMarketResponse(const QString& symbol, const QString& code);
    void activateSymbolsUpdater(qint32 msPeriod);

    bool reconnectEnabled() const
    { return allowReconnect_; }

protected slots:
    void setReconnectEnabled(int on);
    void exec(TimerID id);

protected:
    void onMarketRequestEvent();
    void onMarketResponseEvent();
    void onUpdaterEvent();
    void onReconnectEvent();
    void onHeartbeatEvent();
    void onTestrequestEvent();
    
private:
    FIXDispatcher* fixdispatcher() const;
    NetworkManager* manager() const;

private:
    bool allowReconnect_;
    int  reconnectInterval_;
    
    class SPair : public QPair<QString,QString> {
    public:
        SPair() {}
        SPair(const QPair& other) : QPair(other) {}
        SPair(const QString& first, const QString& second) : QPair(first, second) {}
        bool operator==(const SPair& other) const 
        { return (first == other.first); } // we insterested only in symbols dublication
    };

    typedef QVector<SPair> SPairsT;

    SPairsT reqQueue_;
    QMutex  quardReqQueue_;

    SPairsT respQueue_;
    QMutex  quardRespQueue_;

    QSharedPointer<QTimer> requeste_;
    QSharedPointer<QTimer> response_;
    QSharedPointer<QTimer> updater_;
    QSharedPointer<QTimer> reconnect_;
    QSharedPointer<QTimer> heartbeat_;
    QSharedPointer<QTimer> testrequest_;
};

#endif // __scheduler_h__
