#ifndef __fixdatamodel_h__
#define __fixdatamodel_h__

#include "responsehandler.h"
#include "fix.h"
#include "symbolsmodel.h"

#include <QSharedPointer>

class Scheduler;
class QWriteLocker;
class FixLog;
class MqlProxyServer;

////////////////////////////////////
class FixDataModel : public SymbolsModel, public FIX
{
public:
    FixDataModel(QSharedPointer<MqlProxyServer>& mqlProxy, QWidget* parent);
    ~FixDataModel();

    // Returned > 0 when outgoing messages has to be sent
    // 0 when doesn't need sending after process
    // 1 when processed login message from server and heartbeat timer should be activated
    // -1 when processed logout message from server and logging out should be activated
    int process(const QByteArray& message);

    // Make FIX message type "35=V" - request subscribe market data by instrument 
    QByteArray makeSubscribe(const Instrument& inst);

    // Make FIX message type "35=V" - request unsubscribe ("263=2") market data by instrument 
    QByteArray makeUnSubscribe(const Instrument& inst);

    // Gets snapshot by the table data fetching (readonly)
    // check autolock after calling - it must be not empty when getSnapshot has owned the section
    Snapshot* getSnapshot(const char* symbol, QSharedPointer<QReadLocker>& autolock) const;

    // Store MsgSeqNum of requesting message (symbols by msgSeqNums association)
    void storeRequestSeqnum(const QByteArray& requestMessage, const char* symbol = NULL);

    // Actions before logout
    void beforeLogout();

    FixLog& msglog() const 
    { return *fixlog_.data(); }

protected:
    // Remove from cache by code
    void removeCached(qint32 byCode);

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
    // Activate requests for all viewed instruments
    void activateMonitoring();

    // Cache not cleared but logout reason must be set for info
    void serverLogoutError(const std::string& reason);

    // Remove from cache all records
    void clearCache();

    // Send out zero quotes to Mql client(s)
    void mqlClearPrices();

    // Send out quotes with ask/bid to Mql client(s)
    void mqlSendQuotes(const std::string& sym, const std::string& bid, const std::string& ask);

    // Gets snapshot by symbol and code of instrument
    // Check autolock after calling - it must be not empty when snapshotDelegate has owned write section
    Snapshot* snapshotDelegate(const char* symbol, qint32 code, 
                               QSharedPointer<QWriteLocker>& autolock);

    // Gets snapshot by message sequence number (for session reject only)
    // check autolock after calling - it must be not empty when snapshotDelegate has owned write section
    Snapshot* snapshotDelegate(qint32 msgSeqNum, 
                               QSharedPointer<QWriteLocker>& autolock);

private:
    typedef QMap<qint32,std::string> SeqnumToSymT;

    SeqnumToSymT seqnumMap_;
    QReadWriteLock* cacheLock_;
    SnapshotSet cache_;
    QSharedPointer<FixLog> fixlog_;
    QSharedPointer<MqlProxyServer> mqlProxy_;
};

#endif // __fixdatamodel_h__
