#ifndef __mqlproxyserver_h__
#define __mqlproxyserver_h__

#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qlocalserver.h>
#include <QMap>

/////////////////////////////////////////////////////////////////
class MqlProxyServer: public QLocalServer
{
    Q_OBJECT
public:
    MqlProxyServer(QObject* parent);
    ~MqlProxyServer();

    void  start();
    void  sendMessageBroadcast(const char* transaction);
    void  sendMessage(const char* transaction, QLocalSocket* cnt);
    qint8 numberOfConnected();

Q_SIGNALS:
    void notifyNewConnection(QLocalSocket* cnt);
    void notifyReadyRead(QLocalSocket* cnt);

protected slots:
    void onNewConnection();
    void onDisconnected();
    void onReadyRead();

protected:
    void stop();

private:
    void dbgInfo(const std::string& info);
    void logSocketError(QAbstractSocket::SocketError code);

    // Each client using channels pair: first - reading, second - writing
    // Client's reading channel is connective so channel for server writing used as a key
    typedef QMap<QLocalSocket*,QLocalSocket*> ChannelsT;
    ChannelsT clients_;
    QMutex clientsLock_;
};

#endif // __mqlproxyserver_h__
