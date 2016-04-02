#ifndef __sslclient_h__
#define __sslclient_h__

#include "defines.h"
#include "requesthandler.h"

#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslConfiguration>

// Aliases for global using
#define InfoEstablished (SslClient::infoEstablished)
#define InfoConnecting  (SslClient::infoConnecting)
#define InfoReconnect   (SslClient::infoReconnect)
#define InfoClosing     (SslClient::infoClosing)
#define InfoUnconnected (SslClient::infoUnconnected)
#define InfoError       (SslClient::infoError)
#define InfoWarning     (SslClient::infoWarning)

//////////////////////////////////////////////////////////////
class SslClient : public QSslSocket
{
    Q_OBJECT

    friend class QSharedPointer<SslClient>;
public:
    SslClient(RequestHandler* handler, QSsl::SslProtocol proto);
    virtual ~SslClient();

    void establish(const QString& host, quint16 port);

    inline void setIOError(const QString& error) const { 
        QString serr = errorString();
        if( !serr.isEmpty() ) {
            CDebug() << "SslClient::setIOError " << error;
            const_cast<SslClient*>(this)->setErrorString(error);
        }
    }

    inline QString lastError() const 
    { return errorString(); }

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
    void handleDisconnectError(QAbstractSocket::SocketError sockError);
    void ConfigureForLMAX(QSsl::SslProtocol proto);

    RequestHandler* handler_;

public:
    static QString infoEstablished;
    static QString infoConnecting;
    static QString infoReconnect;
    static QString infoClosing;
    static QString infoUnconnected;
    static QString infoError;
    static QString infoWarning;
    FILE* messagesLog_;
};


#endif /* __sslclient_h__ */
