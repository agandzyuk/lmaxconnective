#include "netmanager.h"
#include "ini.h"
#include "connectdlg.h"
#include "scheduler.h"
#include "sslclient.h"
#include "model.h"

#include <QtWidgets>
#include <QtCore>

#include <vector>
#include <algorithm>

using namespace std;

extern bool outerSymbolsUpdate(std::vector<std::string>& symbols);
extern void outerClearPrices(const QIni& ini);

////////////////////////////////////////////////////////
NetworkManager::NetworkManager(QIni& ini) 
    : ini_(ini),
    state_(Initial),
    disconnectFlags_(None),
    connectInfoDlg_(NULL)
{
    if( !QSslSocket::supportsSsl() ) {
        QMessageBox::information(NULL, "SSL", "OpenSSL libraries not found!");
        throw std::exception();
    }
}

NetworkManager::~NetworkManager()
{
    if(dispatcher_->LoggedIn())
        asyncStop();
}

void NetworkManager::setParent(QWidget* parent) 
{ 
    if( NULL == scheduler_ ) {
        scheduler_.reset( new QScheduler(parent) );
        scheduler_->activateSymbolsUpdater(SYMBOLS_UPDATE_PROVIDER_PERIOD);
    }
    if( NULL == dispatcher_ )
        dispatcher_.reset( new LMXModel(ini_ ,scheduler_.data()) );
    if( NULL == connectInfoDlg_ )
        connectInfoDlg_ = new ConnectDlg(ini_, parent);
    parent_ = parent; 
}

void NetworkManager::asyncStart(bool reconnect)
{
    {
        QMutexLocker safe(&flagMutex_);
        disconnectFlags_ = None;
        connectInfoDlg_->setReconnect(reconnect);
    }

    if( !connection_.isNull() )
        connection_->close();
    
    QSsl::SslProtocol  ssnproto = QSsl::AnyProtocol;
    QString proto = ini_.value(ProtocolParam);

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

    connection_.reset(new SslClient(this, ssnproto));
    connection_->establish(ini_.value(ServerParam), 443);
}

void NetworkManager::asyncStop()
{
    {
        QMutexLocker safe(&flagMutex_);
        disconnectFlags_ = Forced;
        connectInfoDlg_->setReconnect(false);
        if( connectInfoDlg_->isVisible() )
            connectInfoDlg_->hide();
    }
    onHaveToLogout();

    if( !connection_.isNull() ) {
        connection_->close();
        connection_.reset();
    }
}

QString NetworkManager::errorString() const
{
    return connection_.isNull() ? "" : connection_->lastError();
}

short NetworkManager::disconnectStatus() const
{
    QMutexLocker safe(&flagMutex_);
    return disconnectFlags_;
}

void NetworkManager::onStateChanged(ConnectionState state, 
                                    short disconnectStatus)
{
    if( state_ == state )
        return;
    state_ = state;

    QString txtState;
    switch( state )
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
    if( state == HasError || state == Closing )
    {
        // restore dispatcher flags 
        if( state != Closing ) {
            dispatcher_->SetLoggedIn(false);
            dispatcher_->SetTestRequestSent(false);
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
            QMutexLocker safe(&flagMutex_);
            disconnectFlags_ = disconnectStatus;
        }
        else
            return;
    }


    CDebug() << "NetworkManager:onStateChanged " << txtState << 
            (disconnectInfo.isEmpty() ? "" : (", status "+disconnectInfo));

    if( disconnectFlags_ == Forced )
    {
        if( state == Unconnect && 
            scheduler_->reconnectEnabled() && 
            connectInfoDlg_->getReconnect() ) 
        {
            QString arg = InfoWarning + ": reconnecting is disabled by reason of network error on client side.";
            connectInfoDlg_->updateStatus(arg);
            connectInfoDlg_->setReconnect(false);
        }
    }
    else if( state == Unconnect  && scheduler_->reconnectEnabled() )
        connectInfoDlg_->setReconnect(true);
    
    if( state == Establish ) 
        connectInfoDlg_->updateStatus(InfoEstablished);
    else if( state == Unconnect )
        connectInfoDlg_->updateStatus(InfoUnconnected);
    else if( state == Progress )
        connectInfoDlg_->updateStatus(InfoConnecting);
    else if( state == HasError )
        connectInfoDlg_->updateStatus(InfoError + ": " + errorString());
    else if( state == HasWarning )
        connectInfoDlg_->updateStatus(InfoWarning + ": " + errorString());
    else if( state == Closing )
        connectInfoDlg_->updateStatus(InfoClosing + ": " + (errorString().isEmpty() ? "" : errorString()) );
}

void NetworkManager::onHaveToLogin()
{
    if( dispatcher_->LoggedIn() )
        return;

    QByteArray message = dispatcher_->MakeLogon();

    // send nulls instead prices
    outerClearPrices(ini_);
    emit connection_->asyncSending(message);

    CDebug() << "Logon type=\"A\" sent:";
    CDebug(false) << ">> " << message;
}

void NetworkManager::onHaveToLogout()
{
    if( !dispatcher_->LoggedIn() )
        return;

    dispatcher_->beforeLogout();
    outerClearPrices(ini_);
    if( dispatcher_->LoggedIn() )
    {
        QByteArray message = dispatcher_->MakeLogout();
        emit connection_->asyncSending(message);

        CDebug() << "Logout type=\"5\" sent:";
        CDebug(false) << ">> " << message;

        dispatcher_->SetLoggedIn(false);
    }
}

void NetworkManager::onHaveToTestRequest()
{
    if( !dispatcher_->LoggedIn() )
        return;

    int hbi = dispatcher_->heartbeatInterval() + 1000;

    int delta = Global::time() - dispatcher_->lastIncoming();
    if( delta >= hbi )
    {
        if( !dispatcher_->TestRequestSent() )
        {
            QByteArray message = dispatcher_->MakeTestRequest();
            emit connection_->asyncSending(message);

            CDebug() << ">> TestRequest type=\"1\" sent";
        }
        else
        {
            // next time it will work
            dispatcher_->SetTestRequestSent(false);
        }
        delta = hbi;
    }
    else 
        delta = hbi - delta;

    scheduler_->activateTestrequest(delta);
}

void NetworkManager::onHaveToHeartbeat() const
{
    if( !dispatcher_->LoggedIn() )
        return;

    int hbi = dispatcher_->heartbeatInterval();

    int delta = Global::time() - dispatcher_->lastOutgoing();
    if( delta >= hbi )
    {
        QByteArray message = dispatcher_->MakeHeartBeat();
        emit connection_->asyncSending(message);

        delta = hbi;
    }
    else
        delta = hbi-delta;

    scheduler_->activateHeartbeat(delta);
}

void NetworkManager::onHaveToMarketRequest(const QString& symbol, const QString& code)
{
    if( !dispatcher_->LoggedIn() )
        return;

    QByteArray message = dispatcher_->MakeNewRequest(symbol, code);
    emit connection_->asyncSending(message);

    // check is a new request from mql client
    if( !ini_.isMounted(symbol) )
        ini_.mountSymbol(symbol);

    CDebug() << "Market Request type=\"V\" sent:";
    CDebug(false) << ">> " << message;
}

void NetworkManager::onHaveToSymbolsUpdate()
{
    vector<string> symbols;
    ini_.mountedSymbolsStd(symbols);

    vector<string> copy = symbols;
    // !outer dll call to check MT provider symbols
    if( !outerSymbolsUpdate(symbols) )
        return;

    if( symbols.size() == copy.size()  )
        return;
    
    for(vector<string>::iterator s_It = symbols.begin(); s_It != symbols.end(); ++s_It)
    {
        vector<string>::iterator copy_It = std::find(copy.begin(), copy.end(), *s_It);
        if( copy_It != copy.end() ) {
            copy.erase(copy_It);
            continue;
        }

        QString qSymbol = s_It->c_str();
        QString code = ini_.originCodeBySymbol(qSymbol);
        if( !ini_.isMounted(qSymbol) && !code.isEmpty() ) 
        {
            qDebug() << "Update: New Request from MQL client for \"" + qSymbol + ":" + code + "\"";
            ini_.mountSymbol(qSymbol);
            scheduler_->activateMarketRequest(qSymbol, code);
        }
    }

    if(!dispatcher_->LoggedIn())
        outerClearPrices(ini_);
}

void NetworkManager::onMessageReceived(const QByteArray& message) const
{
    int ret = dispatcher_->process(message);
    if( ret > 0 ) 
    {
        QByteArray message;
        do {
            message.swap( dispatcher_->TakeOutgoing() );
            if( !message.isEmpty() ) {
                emit connection_->asyncSending(message);
                std::string value = FIX::getField(message,"35");
                if( value[0] == 'V' ) {
                    CDebug() << "Market Request type=\"V\" sent:";
                    CDebug(false) << ">> " << message;
                }
                else if( value[0] == '0' && !FIX::getField(message,"112").empty() ) {
                    CDebug() << "Test Request Response type=\"0\" sent:";
                    CDebug(false) << ">> " << message;
                }
                message.clear();
            }
            else
                break;
        }
        while( dispatcher_->LoggedIn() );
    }
    else if( ret == -1 ) // Server Logout
    {
        scheduler_->activateLogout();
    }
}
