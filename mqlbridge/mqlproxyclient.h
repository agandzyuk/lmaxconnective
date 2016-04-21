#ifndef __mqlproxyclient_h__
#define __mqlproxyclient_h__

#include <QObject>
#include <qringbuffer_p.h>

QT_BEGIN_NAMESPACE;
class QLocalSocket;
QT_END_NAMESPACE;

///////////////////////////////////////////////////////
class MqlProxyClient: public QObject
{
    Q_OBJECT
    friend class MqlBridge;
public:
    MqlProxyClient(QObject* parent);
    ~MqlProxyClient();

    bool    connectToServer();
    void    sendMessage(const char* transaction);
    void    readSync();
    qint32  checkPipeState(); // returns the buffer size if pipe is not broken

    inline const std::string& errorString() const
    { return errorString_; }

    inline bool hasError() const 
    { return !errorString_.empty(); }

    void dbgInfo(const std::string& info);

Q_SIGNALS:
    void notifyReadyRead(const char*, qint32, qint32*);
    void notifyNewConnection();
    void notifyDisconnect();

protected slots:
    void onDisconnect();
    void onNewConnection();

private:
    void logInternalError(const std::string& desc);
    void destroyPipe();
    bool createWriter();

private:
    void* reader_;
    QRingBuffer readBuffer_;
    std::string errorString_;
    bool pipeBroken_;

    QMutex writerLock_;
    QLocalSocket* writer_;
};

#endif // __mqlproxyclient_h__
