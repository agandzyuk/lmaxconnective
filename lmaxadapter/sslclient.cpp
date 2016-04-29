#include "sslclient.h"

#include "globals.h"
#include "syserrorinfo.h"
#include "fix.h"


#include <QtCore>
#include <QtNetwork>
#include <QSslConfiguration>
#include <QWaitCondition>

#include <Windows.h>
#include <string>

#define SOCKET_BUFSIZE 0x2000

//FILE *fcheck = NULL;

/////////////////
SslClient::SslClient(QSsl::SslProtocol protocol, RequestHandler* handler)
    : handler_(handler),
    running_(false),
    proto_(protocol),
    ssl_(NULL),
    host_("not_configured"),
    port_(443)
{
    connect(this, SIGNAL(asyncSending(QByteArray)), this, SLOT(socketSendMessage(QByteArray)));
}

SslClient::~SslClient()
{
    stop();
    delete threadEvent_;
    threadEvent_ = NULL;
    delete threadLock_;
    threadLock_ = NULL;
}

void SslClient::establish(const QString& host, quint16 port)
{
    threadLock_  = new QMutex();
    threadEvent_ = new QWaitCondition();

    QMutexLocker blocked(threadLock_);
    host_ = host;
    port_ = port;

    start();
    threadEvent_->wait(threadLock_);

    if(ssl_.isNull() || running_ == false)
        Q_ASSERT_X(ssl_.isNull() || running_ == false, "SslClient::establish", "Cannot start SslClient's thread");

    connect(ssl_.data(), SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)), Qt::DirectConnection);
    connect(ssl_.data(), SIGNAL(encrypted()), 
            this, SLOT(socketEncrypted()), Qt::DirectConnection);
    connect(ssl_.data(), SIGNAL(error(QAbstractSocket::SocketError)), 
            this, SLOT(socketError(QAbstractSocket::SocketError)), Qt::DirectConnection);
    connect(ssl_.data(), SIGNAL(sslErrors(QList<QSslError>)), 
            this, SLOT(sslErrors(QList<QSslError>)), Qt::DirectConnection);
    connect(ssl_.data(), SIGNAL(readyRead()), 
            this, SLOT(socketReadyRead()), Qt::DirectConnection);

    ConfigureForLMAX();
}

void SslClient::run()
{
    Q_ASSERT_X(running_==0,"SslClient::run", "Thread object reuse");
    Q_ASSERT_X(ssl_.isNull(),"SslClient::run", "Socket object reuse");

    ssl_.reset(new QSslSocket(this));
    ssl_->connectToHostEncrypted(host_,port_);

    running_ = true;
    threadEvent_->wakeOne();

    { QMutexLocker blocked(threadLock_); }
    QThread::exec();

    QMutexLocker blocked(threadLock_);
    ssl_->close();
    ssl_.reset();
}

void SslClient::stop()
{
    running_ = false;
    {
        QMutexLocker blocked(threadLock_);
        exit();
    }
    wait();
}

void SslClient::ConfigureForLMAX()
{
    // configure required session protocol
    QSslConfiguration config = ssl_->sslConfiguration();
    QSsl::SslProtocol p = config.sessionProtocol();
    if( p != proto_ )
        config.setProtocol(proto_);

    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setSslOption(QSsl::SslOptionDisableServerNameIndication, true);
    ssl_->setSslConfiguration(config);
}

void SslClient::socketSendMessage(const QByteArray& message)
{
    if( !running_ )
        return;

    qint64 written = ssl_->write( message );
    if( written <= 0 ) 
    {
        QList<QSslError> errLst = ssl_->sslErrors();
        if( !errLst.empty() ) {
            QString msg("SSL errors:\n");
            for(QList<QSslError>::const_iterator It = errLst.begin(); It != errLst.end(); ++It)
                msg += It->errorString() + "\n";
            setIOError(msg);
            handler_->onStateChanged(EstablishWarnState);
        }
    }
}

void SslClient::socketReadyRead()
{
    if( !ssl_->bytesAvailable() )
        return;
    
    QByteArray message = ssl_->read(SOCKET_BUFSIZE);
    if( 0 == message.size() ) 
    {
        QList<QSslError> errLst = ssl_->sslErrors();
        if( !errLst.empty() ) {
            QString msg("SSL errors:\n");
            for(QList<QSslError>::const_iterator It = errLst.begin(); It != errLst.end(); ++It)
                msg += It->errorString() + "\n";
            setIOError( msg );
            handler_->onStateChanged(EstablishWarnState);
        }
    }
    else
    {
/*        std::string type = FIX::getField(message,"35");
        if(!type.empty() && type[0] == 'W') {
            std::string sym = FIX::getField(message,"262");
            if( !sym.empty() )
            {
                std::string ask, bid;
                int entries = atol(FIX::getField(message,"268").c_str());
                for(int n = 0; n < entries; n++)
                {
                    type = FIX::getField(message,"269",n);
                    if( !type.empty() && type[0] == '0' )
                        bid = FIX::getField(message,"270",n);
                    else if( !type.empty() && type[0] == '1' )
                        ask = FIX::getField(message,"270",n);
                }

                if( !bid.empty() || !ask.empty() )
                {
                    if(fcheck == NULL)
                        fcheck = fopen("onsocket.txt", "w+c");

                    fprintf(fcheck, "%s: \"%s\"\task=%s, bid=%s\n", 
                                    Global::timestamp().c_str(), 
                                    sym.c_str(), 
                                    ask.c_str(), 
                                    bid.c_str());
                    fflush(fcheck);
                }
            }
        }*/
        handler_->onMessageReceived(message);
    }
}

void SslClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    CDebug() << "SslClient::socketStateChanged " << state;

    RequestHandler::ConnectionState uiState = InitialState;
    switch(state)
    {
    case QSslSocket::UnconnectedState:
        uiState = UnconnectState;
        break;
    case QSslSocket::ClosingState:
        uiState = EngineClosingState;
        break;
    case QSslSocket::HostLookupState:
    case QSslSocket::ConnectingState:
        ioError_.clear();
        uiState = ProgressState;
        break;
    case QSslSocket::ConnectedState:
    case QSslSocket::BoundState:
    case QSslSocket::ListeningState:
        ioError_.clear();
        uiState = EstablishState;
        break;
    }

    // ignore engine closing
    if( uiState != EngineClosingState )
        handler_->onStateChanged(uiState);
}

void SslClient::socketEncrypted()
{
    QSslCipher ciph = ssl_->sessionCipher();
    QString cipher = QString("%1, %2 (%3/%4)").arg(ciph.authenticationMethod())
                     .arg(ciph.name()).arg(ciph.usedBits()).arg(ciph.supportedBits());
    CDebug() << "SSL cipher " << cipher;
}


void SslClient::socketError(QAbstractSocket::SocketError err)
{
    if( lastError().isEmpty() ) {
        QString output;
        QDebug errinfo(&output);
        errinfo << err;
        setIOError(output);
    }
    handleDisconnectError(err);
}

void SslClient::sslErrors(const QList<QSslError>& errors)
{
    Q_UNUSED(errors);

    ssl_->ignoreSslErrors();
    QAbstractSocket::SocketState st = ssl_->state();
    if( st != QAbstractSocket::ConnectedState )
    {
        socketStateChanged(ssl_->state());
        if( st == QAbstractSocket::ClosingState )
            handler_->onStateChanged(EstablishWarnState);
    }
}

QString SslClient::lastError() const
{ 
    QString tmp;
    if( !ssl_.isNull() )
        tmp = ssl_->errorString().isEmpty() ? ioError_ : ssl_->errorString(); 
    ioError_.clear();
    return tmp;
}

void SslClient::handleDisconnectError(QAbstractSocket::SocketError sockError)
{
    RequestHandler::ConnectionState st = ClosedFailureState;
    switch( sockError )
    {
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::SocketTimeoutError:
    case QAbstractSocket::SocketResourceError:
    case QAbstractSocket::UnfinishedSocketOperationError:
    case QAbstractSocket::SslHandshakeFailedError:
    case QAbstractSocket::UnsupportedSocketOperationError:
    case QAbstractSocket::ProxyConnectionClosedError:
    case QAbstractSocket::ProxyConnectionTimeoutError:
    case QAbstractSocket::OperationError:
        st = ClosedRemoteState;
        break;
    case QAbstractSocket::HostNotFoundError:
    case QAbstractSocket::SocketAccessError:
    case QAbstractSocket::AddressInUseError:
    case QAbstractSocket::DatagramTooLargeError:
    case QAbstractSocket::SocketAddressNotAvailableError:
    case QAbstractSocket::ProxyAuthenticationRequiredError:
    case QAbstractSocket::ProxyNotFoundError:
    case QAbstractSocket::ProxyProtocolError:
        st = ForcedClosingState;
        break;
    }
    handler_->onStateChanged(st);
}

void SslClient::setIOError(const QString& error) const
{ 
    if( ssl_.isNull() )
        return;

    QString serr = ssl_->errorString();
    if( !serr.isEmpty() ) {
        CDebug() << "SslClient::setIOError " << error;
        ioError_ = error;
    }
}
