#ifndef __mqlbridge_h__
#define __mqlbridge_h__

#include "external.h"

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 1
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 1
#endif

#include <QScopedPointer>
#include <QAtomicPointer>
#include <QReadWriteLock>
#include <QThread>

#include <set>
#include <map>

////////////////////////////////////////////////////////////////////////////////
// Macros for Windows API Event

#define InitEvent() ((quintptr)CreateEvent(NULL,FALSE,FALSE,NULL))
#define DeleteEvent(waiter) (CloseHandle((HANDLE)waiter))
#define SignalEvent(waiter) (SetEvent((HANDLE)waiter))
#define WaitForEvent(waiter) (WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)waiter,INFINITE))
#define TimedWaitForEvent(waiter,tm) (WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)waiter,tm))

////////////////////////////////////////////////////////////////////////////////
struct MqlQuote {
    double ask_;
    double bid_;
    MqlQuote(double ask, double bid) 
        : ask_(ask), bid_(bid)
    {}
};

////////////////////////////////////////////////////////////////////////////////
class MqlProxyClient;
struct MqlProxySymbols;

class MqlBridge : private QThread
{
    Q_OBJECT
public:
    MqlBridge();
    ~MqlBridge();

    void attach();
    void detach();
    bool isAttached();

    inline double getBid(const char* symbol)
    { return getQuote(symbol, true); }

    inline double getAsk(const char* symbol)
    { return getQuote(symbol, false); }

    void setBid(const char* symbol, double bid);
    void setAsk(const char* symbol, double ask);

protected slots:
    void onTransaction(const char* buffer, qint32 size, qint32* remainder);
    void onNewConnection();
    void onDisconnect();

protected:
    MqlQuote* MqlBridge::addQuote(const char* sym, QScopedPointer<QWriteLocker>& autolock);
    MqlQuote* MqlBridge::findQuote(const char* sym, QScopedPointer<QReadLocker>& autolock);
    bool proxyStart(QScopedPointer<QMutexLocker>& autolock);
    void proxyConnect(QScopedPointer<QMutexLocker>& autolock);
    void run();

private:
    double getQuote(const char* symbol, bool bid);
    bool   setQuote(const char* symbol, double value, bool bid, QScopedPointer<QReadLocker>& readlock);

    // creates transaction message about new instrument request
    void sendSingleTransaction(const char* sym);

    // returned buffer must be freed
    char* createMultipleTransaction(const std::set<std::string>& symbols);

    static void apiShowError(const std::string& info);

private:
    typedef std::map<std::string,MqlQuote> QuotesT;
    QuotesT         quotes_;
    QReadWriteLock* quotesLock_;
    quintptr        proxyWaiter_;
    QMutex          proxyLock_;

    QAtomicPointer<MqlProxyClient> connection_;
    QAtomicInt proxyConnected_;
    QAtomicInt attached_;

    std::set<std::string> incomingQuotes_;
};

Q_GLOBAL_STATIC(MqlBridge, spMqlBridge)

#endif __mqlbridge_h__
