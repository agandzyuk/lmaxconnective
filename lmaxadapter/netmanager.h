#ifndef __netmanager_h__
#define __netmanager_h__

#include "requesthandler.h"
#include "marketabstractmodel.h"

#include <QDialog>
#include <QMutex>

class QuotesTableModel;
class ConnectDialog;
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

    void start(bool reconnect);
    void stop();

    inline Scheduler* scheduler() {
        return scheduler_.data();
    }
    inline ConnectDialog* connectDialog() {
        return connectDialog_.data();
    }
    inline QuotesTableModel* model() {
        return model_.data();
    }
    short disconnectStatus() const;

Q_SIGNALS:
    void notifyDlgUpdateStatus(const QString& info);
    void notifyDlgSetReconnect(bool on);

protected slots:
    void onStateChanged(int state, short disconnectStatus);
    bool onHaveToSendMessage(const QByteArray& message);
    void onHaveToSubscribe(const Instrument& inst);
    void onHaveToUnSubscribe(const Instrument& inst);
    void onMessageReceived(const QByteArray& message);
    void onMqlConnected(QLocalSocket* cnt);
    void onMqlReadyRead(QLocalSocket* cnt);
    void onDlgUpdateStatus(const QString& statusInfo);
    void onDlgSetReconnect(bool on);

protected:
    void onHaveToLogin();
    void onHaveToLogout();
    void onHaveToTestRequest();
    void onHaveToHeartbeat();
    QString errorString() const;

protected:
    QScopedPointer<Scheduler> scheduler_;
    QScopedPointer<ConnectDialog> connectDialog_;
    QScopedPointer<QuotesTableModel> model_;
    QScopedPointer<SslClient>  connection_;
    QSharedPointer<MqlProxyServer> mqlProxy_;

private:
    ConnectionState state_;
    QMutex* flagLock_;
    short disconnectFlags_;
};

#endif // __netmanager_h__
