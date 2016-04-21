#include "globals.h"
#include "netmanager.h"

#include "quotestablemodel.h"
#include "connectdialog.h"
#include "scheduler.h"
#include "sslclient.h"
#include "fixlogger.h"
#include "mqlproxyserver.h"

#include <QMutex>
#include <QtWidgets>
#include <QSslSocket>

#include <vector>
#include <algorithm>

using namespace std;

////////////////////////////////////////////////////////
NetworkManager::NetworkManager(QWidget* parent) 
    : QObject(parent),
    state_(Initial),
    disconnectFlags_(None),
    connectDialog_(NULL),
    flagLock_(new QMutex())
{
    if( !QSslSocket::supportsSsl() ) {
        QMessageBox::information(NULL, "SSL", "OpenSSL libraries not found!");
        throw std::exception();
    }

    mqlProxy_.reset(new MqlProxyServer(parent));
    mqlProxy_->start();
    QObject::connect(mqlProxy_.data(), SIGNAL(notifyNewConnection(QLocalSocket*)), this, SLOT(onMqlConnected(QLocalSocket*)));
    QObject::connect(mqlProxy_.data(), SIGNAL(notifyReadyRead(QLocalSocket*)), this, SLOT(onMqlReadyRead(QLocalSocket*)));

    model_.reset(new QuotesTableModel(mqlProxy_, parent));
    scheduler_.reset( new Scheduler(this) );
    connectDialog_.reset(new ConnectDialog(*model(), parent));

    QObject::connect( this, SIGNAL(notifyDlgSetReconnect(bool)), 
                      this, SLOT(onDlgSetReconnect(bool)) );
    QObject::connect( this, SIGNAL(notifyDlgUpdateStatus(QString)), 
                      this, SLOT(onDlgUpdateStatus(QString)) );

    QObject::connect( model(), SIGNAL(activateRequest(Instrument)), 
                      scheduler(), SLOT(activateRequest(Instrument)) );
    QObject::connect( model(), SIGNAL(unsubscribeImmediate(Instrument)),
                      this, SLOT(onHaveToUnSubscribe(Instrument)) );
    
    QObject::connect( model(), SIGNAL(activateResponse(Instrument)), 
                      scheduler(), SLOT(activateResponse(Instrument)) );
    
    QObject::connect( model(), SIGNAL(notifyReconnectSetCheck(bool)), 
                      parent, SLOT(onReconnectSetCheck(bool)) );
}

NetworkManager::~NetworkManager()
{
    if(model_->loggedIn())
        stop();

    { QMutexLocker g(flagLock_); }
    delete flagLock_;
    flagLock_ = NULL;
}

void NetworkManager::start(bool reconnect)
{
    {
        QMutexLocker g(flagLock_);
        disconnectFlags_ = None;
        connectDialog_->setReconnect(reconnect);
    }

    QSsl::SslProtocol  ssnproto = QSsl::AnyProtocol;
    QString proto = model_->value(ProtocolParam);

    if( proto == ProtoSSLv2 )
        ssnproto = QSsl::SslV2; 
    else if( proto == ProtoSSLv3 )
        ssnproto = QSsl::SslV3; 
    else if( proto == ProtoTLSv1_0 )
        ssnproto = QSsl::TlsV1_0; 
    else if( proto == ProtoTLSv1_x )
        ssnproto = QSsl::TlsV1_0OrLater; 
    else if( proto == ProtoTLSv1_1 )
        ssnproto = QSsl::TlsV1_1; 
    else if( proto == ProtoTLSv1_1x )
        ssnproto = QSsl::TlsV1_1OrLater; 
    else if( proto == ProtoTLSv1_2 )
        ssnproto = QSsl::TlsV1_2; 
    else if( proto == ProtoTLSv1_2x )
        ssnproto = QSsl::TlsV1_2OrLater; 
    else if( proto == ProtoTLSv1_SSLv3 )
        ssnproto = QSsl::TlsV1SslV3; 

    connection_.reset(new SslClient(ssnproto, this));
    connection_->establish(model_->value(ServerParam), 443);

    QObject::connect( model(), SIGNAL(notifySendingManual(QByteArray)), 
                      this, SLOT(onHaveToSendMessage(QByteArray)) );
    QObject::connect( connection_.data(), SIGNAL(notifyStateChanged(int, short)), 
                      parent(), SLOT(onStateChanged(int, short)) );
}

void NetworkManager::stop()
{
    {
        QMutexLocker g(flagLock_);
        disconnectFlags_ = Forced;
        connectDialog_->setReconnect(false);
        if( connectDialog_->isVisible() )
            connectDialog_->hide();
    }
    onHaveToLogout();
    connection_.reset();
}

QString NetworkManager::errorString() const
{
    return connection_.isNull() ? "" : connection_->lastError();
}

short NetworkManager::disconnectStatus() const
{
    QMutexLocker g(flagLock_);
    return disconnectFlags_;
}

void NetworkManager::onDlgUpdateStatus(const QString& statusInfo)
{
    connectDialog_->updateStatus(statusInfo);
}

void NetworkManager::onDlgSetReconnect(bool on)
{
    connectDialog_->setReconnect(on);
}

void NetworkManager::onStateChanged(int state, short disconnectStatus)
{
    if( state == (int)state_ )
        return;
    state_ = (RequestHandler::ConnectionState)state;

    QString txtState;
    switch( state_ )
    {
        case Initial:
            txtState = "Initial"; break;
        case Progress:
            txtState = "Progress"; break;
        case Establish:
            txtState = "Establish"; break;
        case Closing:
            txtState = "Closing"; break;
        case Unconnect:
            txtState = "Unconnect"; break;
        case HasError:
            txtState = "HasError"; break;
        case HasWarning:
            txtState = "HasWarning"; break;
    }

    QString disconnectInfo;
    if( state_ == HasError || state_ == Closing )
    {
        // restore fix flags 
        if( state_ != Closing ) {
            model_->setLoggedIn(false);
            model_->setTestRequestSent(false);
        }

        if( disconnectStatus == None )
            disconnectInfo = "NoDisconnect";
        else if( disconnectStatus == Client )
            disconnectInfo = "ClientDisconnect";
        else if( disconnectStatus == Forced )
            disconnectInfo = "ForcedDisconnect";
        else if( disconnectStatus == Remote )
            disconnectInfo = "RemoteDisconnect";

        if( disconnectFlags_ != Forced )
        {
            QMutexLocker g(flagLock_);
            disconnectFlags_ = disconnectStatus;
        }
        else
            return;
    }

    CDebug() << "NetworkManager:onStateChanged " << txtState << 
            (disconnectInfo.isEmpty() ? "" : (", status "+disconnectInfo));

    if( disconnectFlags_ == Forced )
    {
        if( state_ == Unconnect && 
            scheduler_->reconnectEnabled() && 
            connectDialog_->getReconnect() ) 
        {
            QString arg = InfoWarning + ": reconnecting is disabled by reason of network error on client side.";
            emit notifyDlgUpdateStatus(arg);
            emit notifyDlgSetReconnect(false);
        }
    }
    else if( state_ == Unconnect  && scheduler_->reconnectEnabled() )
        emit notifyDlgSetReconnect(true);

    
    if( state_ == Unconnect )
        emit notifyDlgUpdateStatus(InfoUnconnected);
    else if( state_ == Progress )
        emit notifyDlgUpdateStatus(InfoConnecting);
    else if( state_ == HasError )
        emit notifyDlgUpdateStatus(InfoError + ": " + errorString());
    else if( state_ == HasWarning )
        emit notifyDlgUpdateStatus(InfoWarning + ": " + errorString());
    else if( state_ == Closing )
        emit notifyDlgUpdateStatus(InfoClosing + ": " + (errorString().isEmpty() ? "" : errorString()) );
    else if( state_ == Establish ) {
        emit notifyDlgUpdateStatus(InfoEstablished);
        onHaveToLogin();
    }
}

void NetworkManager::onMqlConnected(QLocalSocket* cnt)
{
    QVector<string> allMonitored;
    model_->getSymbolsUnderMonitoring(allMonitored);

    qint16 countOf = allMonitored.size();

    // create transaction even with 0 monitoring instruments (mql client does a same)
    // this is well to test the first connection and structures parsing
    MqlProxyQuotes* transaction = (MqlProxyQuotes*)malloc(2 + countOf * sizeof(MqlProxyQuotes::Quote));
    transaction->numOfQuotes_ = countOf;

    // retrive all snapshots which been subscribed before
    QSharedPointer<QReadLocker> autolock; // autolock aquired inside getSnapshot only once 
    for(qint16 i = 0; i < countOf; ++i) {
        strcpy_s(transaction->quotes_[i].symbol_, MAX_SYMBOL_LENGTH,allMonitored[i].c_str());
        Snapshot* snap = model_->getSnapshot(allMonitored[i].c_str(), autolock);
        if(snap) {
            // copy quotes from snapshot into transaction structure
            Global::reinterpretDouble(snap->ask_.toStdString().c_str(), &transaction->quotes_[i].ask_);
            Global::reinterpretDouble(snap->bid_.toStdString().c_str(), &transaction->quotes_[i].bid_);
        }
        else {
            transaction->quotes_[i].ask_ = 0;
            transaction->quotes_[i].bid_ = 0;
        }
    }
    autolock.reset();

    mqlProxy_->sendMessage((const char*)transaction, cnt);
    free(transaction); // don't need
}

void NetworkManager::onMqlReadyRead(QLocalSocket* cnt)
{
//    CDebug() << "NetworkManager::onMqlReadyRead";
    qint64 bytes = cnt->bytesAvailable();
    if( bytes < 2 ) {
        CDebug() << "Error: onMqlReadyRead received an empty data";
        return;
    }

    char* buffer = (char*)malloc(bytes);
    const char* ptr = buffer;
    cnt->read((char*)buffer, bytes);

    qint32 parsed = 0;
    while( parsed < bytes )
    {
        const MqlProxySymbols* mqlPtr = (MqlProxySymbols*)ptr;
        if(mqlPtr->numOfSymbols_ >= MAX_SYMBOLS ) 
        {
            CDebug() << "Error: onMqlReadyRead received an unexpected amount of symbols (" << mqlPtr->numOfSymbols_ << ")";
            return;
        }

        for(int i = 0; i < mqlPtr->numOfSymbols_; i++)
        {
            const char* sym = mqlPtr->symbols_[i];
            qint32 code = model_->getCode(sym);
            if( code == -1) {
                CDebug() << "Error: onMqlReadyRead received an unsupported symbol \"" << sym << "\"";
                continue;
            }

            Instrument inst(sym,code);
            if( model_->isMonitored(inst) ) {
                //CDebug() << "onMqlReadyRead skips symbol \"" << sym << "\" adding because it already monitored";
                continue;
            }

            model_->setMonitoring(inst, true, false);
            CDebug(false) << "symbol \"" << sym << "\" added to monitoring";
        }
        parsed += (2 + mqlPtr->numOfSymbols_*sizeof(mqlPtr->symbols_[0]));
        ptr += (2 + mqlPtr->numOfSymbols_*sizeof(mqlPtr->symbols_[0]));
    }
    free(buffer);
}

void NetworkManager::onHaveToLogin()
{
    if( model_->loggedIn() )
        return;
    onHaveToSendMessage( model_->makeLogon() );
}

void NetworkManager::onHaveToLogout()
{
    model_->beforeLogout();

    if( !model_->loggedIn() )
        return;

    onHaveToSendMessage( model_->makeLogout() );
    model_->setLoggedIn(false);
}

void NetworkManager::onHaveToTestRequest()
{
    if( !model_->loggedIn() )
        return;

    int hbi = model_->getHeartbeatInterval() + 1000;

    int delta = Global::time() - model_->getLastIncoming();
    if( delta >= hbi )
    {
        if( !model_->testRequestSent() )
        {
            onHaveToSendMessage( model_->makeTestRequest() );
        }
        else
        {
            // next time it will work
            model_->setTestRequestSent(false);
        }
        delta = hbi;
    }
    else 
        delta = hbi - delta;

    scheduler_->activateTestrequest(delta);
}

void NetworkManager::onHaveToHeartbeat()
{
    if( !model_->loggedIn() )
        return;

    int hbi = model_->getHeartbeatInterval();

    int delta = Global::time() - model_->getLastOutgoing();
    if( delta >= hbi )
    {
        onHaveToSendMessage( model_->makeHeartBeat() );
        delta = hbi;
    }
    else
        delta = hbi-delta;

    scheduler_->activateHeartbeat(delta);
}

void NetworkManager::onHaveToSubscribe(const Instrument& inst)
{
    QByteArray message = model_->makeSubscribe(inst);
    if( !message.isNull() )
        onHaveToSendMessage(message);
    model()->activateResponse(Instrument("FullUpdate",-1));
}

void NetworkManager::onHaveToUnSubscribe(const Instrument& inst)
{
    QByteArray message = model_->makeUnSubscribe(inst);
    if( !message.isNull() )
        onHaveToSendMessage(message);
    model()->activateResponse(Instrument("FullUpdate",-1));
}

void NetworkManager::onMessageReceived(const QByteArray& message)
{
    int ret = model_->process(message);
    if( ret > 0 ) 
    {
        QByteArray message;
        do {
            message.swap( model_->takeOutgoing() );
            if( message.isEmpty() )
                break;
            onHaveToSendMessage(message);
            message.clear();
        }
        while( model_->loggedIn() );
    }
    else if( ret == -1 ) // Server Login
    {
        scheduler_->activateHeartbeat(model_->getHeartbeatInterval());
        scheduler_->activateTestrequest(model_->getHeartbeatInterval());
    }
    else if( ret == -2 ) // Server Logout
    {
        scheduler_->activateLogout();
    }
}

bool NetworkManager::onHaveToSendMessage(const QByteArray& message)
{
    if( message.isEmpty() )
        return false;

    string type = FIX::getField(message,"35");
    if(type.empty())
        return false;

    string info;
    string f262sym;
    qint32 f48code;
    switch( type[0] )
    {
    case '0': { 
            string f112 = FIX::getField(message,"112");
            info = f112.empty() ? ("skip") : ("Test Request Response type=\"0\" on TestReqID=\"" + f112 + "\" is sent");
        }
        break;
    case 'V': { 
            f262sym = FIX::getField(message,"262");
            f48code = model_->getCode( f262sym.c_str() );
            char buf[10];
            string insname = f262sym + ":" + string(ltoa(f48code, buf, 10));
            info = "Market Request type=\"V\" for \"" + insname + "\" is sent";
            model_->storeRequestSeqnum(message, f262sym.c_str() );
        }
        break;
    case 'A':
        info = "Logon type=\"A\" is sent";
        break;
    case '5':
        info = "Logout type=\"5\" is sent";
        break;
    case '1':
        info = "TestRequest type=\"1\" is sent";
        break;
    default:
        return false;
    }

    emit connection_->asyncSending(message);
    if( info != "skip" ) {
        CDebug() << QString::fromStdString(info);
        CDebug(false) << ">> " << message;
        if( f262sym.empty() )
            model()->msglog().outmsg(message);
        else
            model()->msglog().outmsg(message, f262sym.c_str(), f48code);
        if( type[0] == 'V')
            model()->activateResponse(Instrument("FullUpdate",-1));
    }

    return true;
}
