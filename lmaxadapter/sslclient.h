#ifndef __ssl_client_h__
#define __ssl_client_h__

#include "requesthandler.h"

#include <QThread>
#include <QSsl>
#include <QSslError>
#include <QAbstractSocket>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE;
class QSslSocket;
class QWaitCondition;
class QMutex;
QT_END_NAMESPACE;

//////////////////////////////////////////////////////////////
class SslClient : public QThread
{
    Q_OBJECT

public:
    SslClient(QSsl::SslProtocol proto = QSsl::TlsV1_1OrLater, RequestHandler* handler = NULL);
    ~SslClient();

    void establish(const QString& host, quint16 port);
    QString lastError() const; 

Q_SIGNALS:
    void asyncSending(const QByteArray& message);

private Q_SLOTS:
    void socketStateChanged(QAbstractSocket::SocketState state);
    void socketEncrypted();
    void socketReadyRead();
    void socketError(QAbstractSocket::SocketError error);
    void sslErrors(const QList<QSslError> &errors);
    void socketSendMessage(const QByteArray& message);

private:
    void setIOError(const QString& error) const;
    void handleDisconnectError(QAbstractSocket::SocketError sockError);
    void ConfigureForLMAX();
    void run();
    void stop();

private:
    QSharedPointer<QSslSocket> ssl_;
    RequestHandler*     handler_;
    QWaitCondition*     threadEvent_;
    QMutex*             threadLock_;
    QAtomicInt          running_;
    QSsl::SslProtocol   proto_;
    QString             host_;
    quint16             port_;
    mutable QString     ioError_;
};

#endif // __ssl_client_h__
