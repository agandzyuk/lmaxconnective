#include "sslclient.h"
#include "syserrorinfo.h"

#include <QtWidgets/QMessageBox>
#include <QtNetwork/QSslCipher>
#include <QSslConfiguration>

#include <windows.h>
#include <string>



#define SOCKET_BUFSIZE 0x2000

/////////////////
QString SslClient::infoEstablished  = tr("Established"); // success connection
QString SslClient::infoConnecting   = tr("Connecting");  // connection in progress
QString SslClient::infoReconnect    = tr("Reconnect"); // reconnect in progress
QString SslClient::infoError        = tr("Error"); // error was during a last operation
QString SslClient::infoWarning      = tr("Warning"); // warning and connection alive
QString SslClient::infoClosing      = tr("Closing"); // closing in progress
QString SslClient::infoUnconnected  = tr("Unconnected"); // closed

/////////////////
SslClient::SslClient(RequestHandler* handler, QSsl::SslProtocol protocol)
    : handler_(handler),
    messagesLog_(NULL)
{
    connect(this, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(this, SIGNAL(encrypted()), this, SLOT(socketEncrypted()));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(this, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(this, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(this, SIGNAL(readChannelFinished()), this, SLOT(socketReadyRead()));
    connect(this, SIGNAL(asyncSending(QByteArray)), this, SLOT(socketSendMessage(QByteArray)));

    ConfigureForLMAX(protocol);
}

SslClient::~SslClient()
{
    if(messagesLog_) {
        fprintf(messagesLog_, "\nClosing time %s\n\n\n", Global::timestamp().c_str());
        fclose(messagesLog_);
    }
}

void SslClient::ConfigureForLMAX(QSsl::SslProtocol proto)
{
    // configure required session protocol
    QSslConfiguration config = sslConfiguration();
    QSsl::SslProtocol p = config.sessionProtocol();
    if( p != proto )
        config.setProtocol(proto);

    config.setPeerVerifyMode(VerifyNone);
    config.setSslOption(QSsl::SslOptionDisableServerNameIndication, true);
    setSslConfiguration(config);
}

void SslClient::establish(const QString& host, quint16 port)
{
    handler_->onStateChanged(ProgressState);
    connectToHostEncrypted(host, port);
}


void SslClient::socketSendMessage(const QByteArray& message)
{
    qint64 written = QSslSocket::write( message );
    if( written <= 0 ) 
    {
        QList<QSslError> errLst = QSslSocket::sslErrors();
        if( !errLst.empty() ) {
            QString msg("SSL errors:\n");
            for(QList<QSslError>::const_iterator It = errLst.begin(); It != errLst.end(); ++It)
                msg += It->errorString() + "\n";
            setIOError( msg );
            handler_->onStateChanged(WarningState);
        }
    }
    else {
        if( NULL == messagesLog_ ) {
            if( messagesLog_ = fopen(MESSAGES_LOGGING_FILE, "a+") ) 
            {
                fprintf(messagesLog_, "///////////////////////////////////////////////////////////\n");
                fprintf(messagesLog_, "// Log session created. Start time %s //\n", Global::timestamp().c_str());
                fprintf(messagesLog_, "///////////////////////////////////////////////////////////\n\n");
            }
        }
        if( messagesLog_ ) {
            fprintf(messagesLog_, "[%s]  >> %s\n", Global::timestamp().c_str(), message.data());
            fflush(messagesLog_);
        }
    }
}

void SslClient::socketReadyRead()
{
    if( !bytesAvailable() )
        return;
    
    QByteArray message = QSslSocket::read(SOCKET_BUFSIZE);
    if( 0 == message.size() ) 
    {
        QList<QSslError> errLst = QSslSocket::sslErrors();
        if( !errLst.empty() ) {
            QString msg("SSL errors:\n");
            for(QList<QSslError>::const_iterator It = errLst.begin(); It != errLst.end(); ++It)
                msg += It->errorString() + "\n";
            setIOError( msg );
            handler_->onStateChanged(WarningState);
        }
    }
    else {
        if( NULL == messagesLog_ ) {
            if( messagesLog_ = fopen(MESSAGES_LOGGING_FILE, "a+") ) 
            {
                fprintf(messagesLog_, "///////////////////////////////////////////////////////////\n");
                fprintf(messagesLog_, "// Log session created. Start time %s //\n", Global::timestamp().c_str());
                fprintf(messagesLog_, "///////////////////////////////////////////////////////////\n\n");
            }
        }
        if( messagesLog_ ) {
            fprintf(messagesLog_, "[%s] <<  %s\n", Global::timestamp().c_str(), message.data());
            fflush(messagesLog_);
        }
        handler_->onMessageReceived(message);
    }
}

void SslClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    CDebug() << "SslClient::socketStateChanged " << state;

    RequestHandler::ConnectionState uiState = InitialState;
    switch(state)
    {
    case UnconnectedState:
        uiState = UnconnectState;
        break;
    case ClosingState:
        uiState = CloseWaitState;
        break;
    case HostLookupState:
    case ConnectingState:
        uiState = ProgressState;
        break;
    case ConnectedState:
        handler_->onHaveToLogin();
    case BoundState:
    case ListeningState:
        uiState = EstablishState;
        break;
    }
    handler_->onStateChanged(uiState);
}

void SslClient::socketEncrypted()
{
    QSslCipher ciph = sessionCipher();
    QString cipher = QString("%1, %2 (%3/%4)").arg(ciph.authenticationMethod())
                     .arg(ciph.name()).arg(ciph.usedBits()).arg(ciph.supportedBits());
    CDebug() << "SSL cipher " << cipher;
}

void SslClient::socketError(QAbstractSocket::SocketError err)
{
    if( lastError().isEmpty() ) {
        QString output;
        QDebug dbg(&output);
        dbg << err;
        setIOError(output);
    }
    handleDisconnectError(err);
}

void SslClient::sslErrors(const QList<QSslError>& errors)
{
    Q_UNUSED(errors);

    ignoreSslErrors();
    QAbstractSocket::SocketState st = state();
    if( st != QAbstractSocket::ConnectedState )
    {
        socketStateChanged(state());
        if( st == QAbstractSocket::ClosingState )
            handler_->onStateChanged(WarningState);
    }
}

void SslClient::handleDisconnectError(QAbstractSocket::SocketError sockError)
{
    short disconnectStatus = ClientDisconnect;
    switch( sockError )
    {
    case RemoteHostClosedError:
    case SocketTimeoutError:
    case SocketResourceError:
    case UnfinishedSocketOperationError:
    case SslHandshakeFailedError:
    case UnsupportedSocketOperationError:
    case ProxyConnectionClosedError:
    case ProxyConnectionTimeoutError:
    case OperationError:
        disconnectStatus = RemoteDisconnect;
        break;
    case HostNotFoundError:
    case SocketAccessError:
    case AddressInUseError:
    case DatagramTooLargeError:
    case SocketAddressNotAvailableError:
    case ProxyAuthenticationRequiredError:
    case ProxyNotFoundError:
    case ProxyProtocolError:
        disconnectStatus = ForcedDisconnect;
        break;
    }
    handler_->onStateChanged(ErrorState, disconnectStatus);
}
