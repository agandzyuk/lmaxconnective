#include "external.h"
#include "mqlproxyclient.h"
#include "syserrorinfo.h"

#include <QtCore>
#include <QLocalSocket>
#include <windows.h>

using namespace std;
static FILE* localsocklog = NULL;


/////////////////////////////////////////////////////////////////
MqlProxyClient::MqlProxyClient(QObject* parent)
    : reader_(INVALID_HANDLE_VALUE), 
    writer_(NULL), 
    pipeBroken_(false)
{
//    if( localsocklog == NULL)
//        localsocklog = fopen("lmax_cnt.log", "wc+");
}
MqlProxyClient::~MqlProxyClient()
{
    destroyPipe();
}

bool MqlProxyClient::connectToServer()
{
//    dbgInfo("MqlProxyClient::connectToServer...");

    // set starting state
    errorString_.clear();
    pipeBroken_ = false;

    string pipename = string("\\\\.\\pipe\\") + MQL_PROXY_PIPE;
    HANDLE localSocket;
    while(1) 
    {
        localSocket = CreateFileA( pipename.c_str(), GENERIC_READ,
                                   0, NULL, OPEN_EXISTING, 0, NULL);
        if( localSocket != INVALID_HANDLE_VALUE )
            break;

        if( ERROR_PIPE_BUSY != GetLastError() ) 
            break;
        // 5 secs waiting to be sure
        if( !WaitNamedPipeA(pipename.c_str(),5000) )
            break;
    }
    if( localSocket == INVALID_HANDLE_VALUE ) {
//        logInternalError( SysErrorInfo("Failed to create the pipe client "
//                                       "(MqlProxyClient::connectToServer)\n",
//                                       (qint32)GetLastError()).get());
        return false;
    }

    reader_ = localSocket;

    DWORD dwMode = 0;
    DWORD instancesCount = 0;
    DWORD flags = 0;
    DWORD outBufferSize = 0;
    DWORD inBufferSize = 0;
    char username[50] = {0};

    //dbgInfo("MqlProxyClient::connectToServer connected!");
    return true;
}

void MqlProxyClient::readSync()
{
//    dbgInfo("MqlProxyClient::readSync...");

    DWORD available = 0;
    qint32 curBufferSize = 0;
    bool startedWithCheckbyte = false;
    do 
    {
        const qint32 minReadBufferSize = 1;
        
        DWORD bytesToRead = qMax(checkPipeState(), minReadBufferSize);
        if( pipeBroken_ ) {
            return;
        }

        qint32 maxReadBufferSize = MqlProxyQuotes::maxProxyServerBufferSize();
        if ( maxReadBufferSize && (qint32)bytesToRead > (maxReadBufferSize - curBufferSize)) 
        {
            bytesToRead = maxReadBufferSize - curBufferSize;
            if (bytesToRead == 0) {
                // Buffer is full. It can happense only when invalid data recevied.
                logInternalError("An invalid data received from pipe (reading buffer is full)");
                onDisconnect();
                return;
            }
        }

        if( !writer_ )
            onNewConnection();

        char* ptr = readBuffer_.reserve(bytesToRead);
        if( !ReadFile(reader_, ptr, bytesToRead, &available, NULL)) 
        {
            startedWithCheckbyte = false;

            const qint32 errcode = (qint32)GetLastError();
            switch (errcode) {
            case ERROR_IO_PENDING:
                break;
            case ERROR_MORE_DATA:
                curBufferSize += available;
                break;
            case ERROR_BROKEN_PIPE:
            case ERROR_PIPE_NOT_CONNECTED:
                break;
            default:
                logInternalError(SysErrorInfo("MqlProxyClient::readSync", errcode).get());
                break;
            }
        }
        else if(available == 1) // next ReadFile must obtain rest data, PeekNamedPipe receives num of bytes before
        {
            curBufferSize = 1;
            // EOF received: exit
            if(*ptr == -1)
                break;
            startedWithCheckbyte = true;
        }
        else 
        {
            curBufferSize += available;
            
            //fprintf(localsocklog, "MqlProxyClient::readSync: notifyReadyRead %d bytes\n", curBufferSize); fflush(localsocklog);
            qint32 remainder = 0;
            emit notifyReadyRead(ptr - qint32(startedWithCheckbyte), curBufferSize, &remainder);

            if( remainder )
                readBuffer_.truncate(curBufferSize-remainder);
            else
                readBuffer_.clear();
            curBufferSize = remainder;
            startedWithCheckbyte = false;
        }
    } 
    while(available);

    onDisconnect();
//    dbgInfo("MqlProxyClient::readSync ends with success");
}

void MqlProxyClient::sendMessage(const char* message)
{
//    dbgInfo("MqlProxyServer::sendMessage...");
    if( pipeBroken_ || reader_ == INVALID_HANDLE_VALUE ) {
//        dbgInfo("MqlProxyServer::sendMessage pipe broken"); 
        return;
    }

    if(writer_ == NULL)
        if(!createWriter())
            return;

    MqlProxySymbols* trans = (MqlProxySymbols*)message;

    /*
    string names;
    for(int i = 0; i < trans->numOfSymbols_; i++ ) 
        names += "\"" + string(trans->symbols_[i]) + "\"" + string(i == trans->numOfSymbols_-1 ? "" : ", ");
    fprintf(localsocklog, "... to LMAX adapter: %s\n", names.c_str());  fflush(localsocklog);
    */
    qint32 transSize = 2 + trans->numOfSymbols_*sizeof(trans->symbols_[0]);

    qint32 written = (qint32)writer_->write(message, transSize);
    if(written != transSize) {
        logInternalError("MqlProxyClient::sendMessage " + writer_->errorString().toStdString());
        writer_->close();
        writer_->deleteLater();
        writer_ = NULL;
    }
    //dbgInfo("MqlProxyServer::sendMessage end"); 
}

qint32 MqlProxyClient::checkPipeState()
{
    DWORD bytes;
    if( PeekNamedPipe(reader_, NULL, 0, NULL, &bytes, NULL) ) {
        return bytes;
    } else {
        if( !pipeBroken_ )
            onDisconnect();
    }
    return 0;
}

bool MqlProxyClient::createWriter()
{
    QMutexLocker guard(&writerLock_);

//    dbgInfo("MqlProxyServer::createWriter"); 
    if( pipeBroken_ || reader_ == INVALID_HANDLE_VALUE ) {
//        dbgInfo("MqlProxyServer::createWriter pipe broken"); 
        return false;
    }
    if( writer_ )
        return true;

    writer_ = new QLocalSocket();
    writer_->connectToServer(MQL_PROXY_PIPE, QIODevice::WriteOnly);
    return writer_->waitForConnected(1000);
}

void MqlProxyClient::destroyPipe()
{
    if( reader_ != INVALID_HANDLE_VALUE ) 
    {
        FlushFileBuffers(reader_);
        DisconnectNamedPipe(reader_);
        CloseHandle(reader_);
        reader_ = INVALID_HANDLE_VALUE;

        if( writer_ != NULL ) {
            writer_->disconnectFromServer();
            writer_->deleteLater();
            writer_ = NULL;
        }
    }
}

void MqlProxyClient::onNewConnection()
{
//    dbgInfo("MqlProxyClient::onNewConnection...");
    if( createWriter() )
        emit notifyNewConnection();
}

void MqlProxyClient::onDisconnect()
{
//    dbgInfo("MqlProxyClient::onDisconnect...");

/*    if( !errorString_.empty() )
        fprintf(localsocklog, "Error: %s\n", errorString_.c_str());
        */
    pipeBroken_ = true;
    destroyPipe();
    emit notifyDisconnect();
}

void MqlProxyClient::logInternalError(const std::string& desc)
{
    errorString_ = desc;
//    fprintf(localsocklog, "Error: %s\n", desc.c_str()); fflush(localsocklog);
}

void MqlProxyClient::dbgInfo(const std::string& info)
{
//    fprintf(localsocklog, "%s\n", info.c_str()); fflush(localsocklog);
}
