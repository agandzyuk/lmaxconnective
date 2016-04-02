#ifndef __netmanager_h__
#define __netmanager_h__

#include "requesthandler.h"
#include <QDialog>
#include <QMutex>

class QIni;
class ConnectDlg;
class SslClient;
class FIXDispatcher;
class QScheduler;

/////////////////////////////////////
class NetworkManager : public RequestHandler
{
    friend class QScheduler;

public:
    NetworkManager(QIni& ini_);
    ~NetworkManager();

    void onMessageReceived(const QByteArray& message) const;

protected:
    void setParent(QWidget* parent);
    void asyncStart(bool reconnect);
    void asyncStop();

    void onStateChanged(ConnectionState state, 
                        short disconnectStatus = 0);
    void onHaveToLogin();
    void onHaveToLogout();
    void onHaveToTestRequest();
    void onHaveToHeartbeat() const;
    void onHaveToMarketRequest(const QString& symbol, const QString& code);
    void onHaveToSymbolsUpdate();

    QString errorString() const;
    short disconnectStatus() const;
    short disconnectFlags_;

protected:
    QSharedPointer<FIXDispatcher> dispatcher_;
    QSharedPointer<QScheduler>    scheduler_;
    ConnectDlg* connectInfoDlg_;

private:
    QIni& ini_;
    QSharedPointer<SslClient> connection_;

    QWidget* parent_;

    mutable QMutex flagMutex_;
    ConnectionState state_;
};

#endif // __netmanager_h__
