#ifndef __fixdispatcher_h__
#define __fixdispatcher_h__

#include "responsehandler.h"
#include "fix.h"

#include <QReadWriteLock>

class QScheduler;

////////////////////////////////////
class FIXDispatcher : public ResponseHandler, public FIX
{
public:
    FIXDispatcher(QIni& ini_, QScheduler* sheduler);
    ~FIXDispatcher();

    // must returns n > 0 when have outgoing messages after processing
    // -1 then processed logoout message from server
    int process(const QByteArray& message);

    // create the new request
    QByteArray MakeNewRequest(const QString& symbol, const QString& code);

    void beforeLogout();

protected:
    // quick: for table data fetching
    const MarketReport* snapshot(const QString& symbol, const QString& code) const;

protected:
    void onLogon(const QByteArray& message);
    void onLogout(const QByteArray& message);
    void onHeartbeat(const QByteArray& message);
    void onTestRequest(const QByteArray& message);
    void onResendRequest(const QByteArray& message);
    void onSequenceReset(const QByteArray& message);
    void onMarketData(const QByteArray& message);
    void onMarketDataReject(const QByteArray& message);
    void onSessionReject(const QByteArray& message);

private:
    // request for all viewed instruments
    int enqueueAll();
    // remove from cache
    void removeCached(const QString& symbol, const QString& code);
    // snapshots with item forwarding in the list
    MarketReport* snapshotDelegate(const QString& symbol, const QString& code);
    // this snapshot also prevents two spare QString copying
    MarketReport* snapshotDelegate(const char* symbol, const char* code); 

private:
    QScheduler* scheduler_;
    QReadWriteLock cacheLock_;
};

#endif // __fixdispatcher_h__
