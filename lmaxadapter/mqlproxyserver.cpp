#include "logger.h"
#include "mqlproxyserver.h"

using namespace std;
//static FILE* localsocklog = NULL;

/////////////////////////////////////////////////////////////////
MqlProxyServer::MqlProxyServer(QObject* parent) 
    : QLocalServer(parent)
{
/*    if( localsocklog == NULL)
        localsocklog = fopen("lmax_srv.log", "wc+");
*/    
    QObject::connect(this, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    start();
}

MqlProxyServer::~MqlProxyServer()
{
    dbgInfo("MqlProxyServer: destroying...");
//    fflush(localsocklog);
    stop();
//    dbgInfo("MqlProxyServer: destroying end");
}

void MqlProxyServer::start()
{
    if( isListening() ) return;

    dbgInfo("MqlProxyServer::start");
    if( !listen(MQL_PROXY_PIPE) )
        logSocketError(serverError());
}

void MqlProxyServer::stop()
{
    dbgInfo("MqlProxyServer::stop...");
    { QMutexLocker g(&clientsLock_); }
    close();
//    dbgInfo("MqlProxyServer::stop end");
}

void MqlProxyServer::sendMessageBroadcast(const char* message)
{
    //dbgInfo("MqlProxyServer::sendMessageBroadcast...");

    QMutexLocker g(&clientsLock_);
    if( clients_.empty() ) {
        dbgInfo("MqlProxyServer::sendMessageBroadcast no clients");
        return;
    }

    MqlProxyQuotes* transaction = (MqlProxyQuotes*)message;

/*    if( transaction->numOfQuotes_ > 1 )
    {
        string names;
        for(int i = 0; i < transaction->numOfQuotes_; i++ ) 
            names += "\"" + string(transaction->quotes_[i].symbol_) + "\"" + string(i == transaction->numOfQuotes_-1 ? "" : ", ");
        CDebug(false) << QString("to %1 MQL clients: ").arg(clients_.size()) << names.c_str(); 
    }
*/
    vector<QLocalSocket*> writers;
    ChannelsT::iterator It = clients_.begin();
    qint32 transSize = 2 + transaction->numOfQuotes_*sizeof(transaction->quotes_[0]);
    for(; It != clients_.end(); ++It)
        writers.push_back(It.key());
    g.unlock();

    for(quint32 i = 0; i < writers.size(); i++) {
        qint32 written = static_cast<qint32>( writers[i]->write(message, transSize) );
        if( written != transSize ) {
            logSocketError((QAbstractSocket::SocketError)(*It)->error());
            continue;
        }
//        CDebug(false) << "a broadcast sent sucessfully";
    }
//    dbgInfo("MqlProxyServer::sendMessageBroadcast end");
}

void MqlProxyServer::sendMessage(const char* message, QLocalSocket* cnt)
{
//    dbgInfo("MqlProxyServer::sendMessage...");

    QLocalSocket* writer = NULL;
    QMutexLocker g(&clientsLock_);
    ChannelsT::Iterator It = clients_.begin();
    for(; It != clients_.end(); ++It)
        if( It.key() == cnt || It.value() == cnt ) {
            writer = It.key();
            break;
        }
    if(writer == NULL) {
        dbgInfo("MqlProxyServer::sendMessage client pipe writer not found");
        return;
    }
    g.unlock();

    MqlProxyQuotes* transaction = (MqlProxyQuotes*)message;

/*    string names;
    for(int i = 0; i < transaction->numOfQuotes_; i++ ) 
        names += "\"" + string(transaction->quotes_[i].symbol_) + "\"" + string(i == transaction->numOfQuotes_-1 ? "" : ", ");
    CDebug(false) << "to MQL client: %s" << names.c_str(); 
*/
    qint32 transSize = 2 + transaction->numOfQuotes_*sizeof(transaction->quotes_[0]);
    qint32 written = static_cast<qint32>(writer->write(message, transSize));
    if( written != transSize )
        logSocketError((QAbstractSocket::SocketError)writer->error());

//    dbgInfo("MqlProxyServer::sendMessage end");
}

void MqlProxyServer::onNewConnection()
{
    dbgInfo("MqlProxyServer::onNewConnection...");

    while( hasPendingConnections() )
    {
        QLocalSocket* cnt = nextPendingConnection();
        if( cnt )
        {
            QMutexLocker g(&clientsLock_);
            ChannelsT::iterator It = clients_.begin();
            // searching for incoming client's reader channel
            for(; It != clients_.end(); ++It) 
                if( It.value() == NULL)
                    break;

            // assign it as server pipe writer
            if( It == clients_.end() )
            {
                dbgInfo("New MQL client is connected");
                QObject::connect(cnt, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
                clients_.insert(cnt, NULL);
                g.unlock();

                // we notice NetworkManager immediatedly about writer because adapter should send syncronization transaction to MQL
                emit notifyNewConnection(cnt);
            }
            else
            {
                dbgInfo("MQL client's pipe writer obtained and will be used as server's reader");

                // assign it as server pipe reader
                QObject::connect(cnt, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
                QObject::connect(cnt, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
                clients_[It.key()] = cnt;
                cnt->setReadBufferSize(MqlProxySymbols::maxProxyClientBufferSize());
            }
        }
    }
}

void MqlProxyServer::onDisconnected()
{
    dbgInfo("MqlProxyServer::onDisconnected...");

    QLocalSocket* cnt = qobject_cast<QLocalSocket*>(sender());
    if( cnt == NULL ) {
        Q_ASSERT_X(cnt, "MqlProxyServer::onDisconnected", "Error qobject_cast to QLocalSocket*");
        dbgInfo("MqlProxyServer::onDisconnected Error qobject_cast to QLocalSocket*");
        return;
    }

    QMutexLocker g(&clientsLock_); 
    ChannelsT::iterator It = clients_.begin();
    for(; It != clients_.end(); ++It )
        if( It.key() == cnt) {
            dbgInfo("MQL pipe reader is disconnected");
            clients_.erase(It);
            break;
        }
        else if( It.value() == cnt )
        {
            dbgInfo("MQL pipe writer is disconnected");
            clients_.erase(It);
            break;
        }
}

void MqlProxyServer::onReadyRead()
{
//    dbgInfo("MqlProxyServer::onReadyRead...");

    QLocalSocket* cnt = qobject_cast<QLocalSocket*>(sender());
    if( cnt == NULL ) {
        Q_ASSERT_X(cnt, "MqlProxyServer::onReadyRead", "Error qobject_cast to QLocalSocket*");
        dbgInfo("MqlProxyServer::onReadyRead Error qobject_cast to QLocalSocket*");
        return;
    }
    if( 0 == cnt->bytesAvailable() ) {
        Q_ASSERT_X(cnt, "MqlProxyServer::onReadyRead", "Null data received");
        dbgInfo("MqlProxyServer::onReadyRead Null data received");
        return;
    }

    // problem that server use one pipe objec read write, but client use separated for reading and writing
    // so we may receive onReadyRead() twice for one message
    // solve it by skipping server writer notification

    bool found = false;
    QMutexLocker g(&clientsLock_);
    ChannelsT::Iterator It = clients_.begin();
    for(; It != clients_.end(); ++It) 
        if( It.value() == cnt && (found = true) ) {
//            dbgInfo("Pipe reader found for client");
            break;
        }
    g.unlock();
    if( found )
        emit notifyReadyRead(cnt);
//    dbgInfo("MqlProxyServer::onReadyRead end");
}

qint8 MqlProxyServer::numberOfConnected()
{
    QMutexLocker g(&clientsLock_);
    return clients_.size();
}

void MqlProxyServer::logSocketError(QAbstractSocket::SocketError code)
{
    QString out;
    QDebug errdbg(&out);
    errdbg << code;
//    fprintf(localsocklog, "Error of MqlProxyServer: %s\n", out.toStdString().c_str()); 
//    fflush(localsocklog);
    CDebug() << QString("Error of MqlProxyServer: %1").arg(out.toStdString().c_str());
}

void MqlProxyServer::dbgInfo(const std::string& info)
{
//    fprintf(localsocklog, "%s\n", info.c_str()); 
//    fflush(localsocklog);
    CDebug() << info.c_str();
}
