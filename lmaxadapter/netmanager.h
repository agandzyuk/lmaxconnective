#ifndef __netmanager_h__
#define __netmanager_h__

#include "requesthandler.h"
#include "marketabstractmodel.h"

#include <QDialog>
#include <QMutex>

class QuotesTableModel;
class SslClient;
class Scheduler;
class MqlProxyServer;

QT_BEGIN_NAMESPACE;
class QMutex;
class QLocalSocket;
QT_END_NAMESPACE;

/////////////////////////////////////
class NetworkManager : public QObject, public RequestHandler
{
    Q_OBJECT

    friend class Scheduler;
public:
    NetworkManager(QWidget* parent);
    ~NetworkManager();

    void start();
    void stop();
    void reconnect();
    void onStateChanged(ConnectionState state);
    ConnectionState getState() const;

    inline QuotesTableModel* model() 
    { return model_.data(); }

    inline Scheduler* scheduler() 
    { return scheduler_.data(); }

Q_SIGNALS:
    void notifyStateChanged(quint8 state, const QString& reason);

protected slots:
    void onServerLogout(const QString& reason);
    bool onHaveToSendMessage(const QByteArray& message);
    void onHaveToSubscribe(const Instrument& inst);
    void onHaveToUnSubscribe(const Instrument& inst);
    void onMessageReceived(const QByteArray& message);
    void onMqlConnected(QLocalSocket* cnt);
    void onMqlReadyRead(QLocalSocket* cnt);

protected:
    void onHaveToLogin();
    void onHaveToLogout();
    void onHaveToTestRequest();
    void onHaveToHeartbeat();

protected:
    QScopedPointer<Scheduler> scheduler_;
    QScopedPointer<QuotesTableModel> model_;
    QScopedPointer<SslClient>  connection_;
    QSharedPointer<MqlProxyServer> mqlProxy_;

private:
    QMutex* stateLock_;
    ConnectionState state_;
};

#endif // __netmanager_h__
