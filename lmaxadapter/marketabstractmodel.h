#ifndef __marketabstractmodel_h__
#define __marketabstractmodel_h__

#include <QAbstractTableModel>
#include <QSet>

///////////////////////////////////////////////////////////////
QT_BEGIN_NAMESPACE;
class QReadWriteLock;
class QReadLocker;
template <class T, typename Cleanup = QScopedPointerDeleter<T>> class QScopedPointer;
QT_END_NAMESPACE;

///////////////////////////////////////////////////////////////
typedef QPair<std::string, qint32> Instrument;

///////////////////////////////////////////////////////////////
struct Snapshot
{
    enum Status {
        StatNoChange = 0,
        StatSubscribe = 1,
        StatAskChange = 2,
        StatBidChange = 4,
        StatBidAndAskChange = 6,
        StatBusinessReject = 16,
        StatSessionReject = 32,
        StatUnSubscribed = 64,
    };

    Instrument  instrument_;
    Status      statuscode_;
    QString     description_;
    QString     bid_;
    QString     ask_;
    qint32      requestTime_;
    qint32      responseTime_;

    inline bool operator==(const Snapshot& rval) const {  
        return (instrument_.second == rval.instrument_.second); 
    }
};

///////////////////////////////////////////////////////////////
QT_BEGIN_NAMESPACE;
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint 
qHash( const Snapshot& key, 
       uint seed = 0 ) Q_DECL_NOTHROW
{ 
    return uint(key.instrument_.second) ^ seed; 
}
QT_END_NAMESPACE;
///////////////////////////////////////////////////////////////

class SnapshotSet : public QSet<Snapshot>
{
    typedef QSet<Snapshot> BaseT;
public:
    SnapshotSet(){};

    inline BaseT::iterator erase(qint32 code) {
        Snapshot tofnd; 
        tofnd.instrument_.second = code;
        BaseT::iterator It = find(tofnd);
        if( It != BaseT::end() ) It = BaseT::erase(It);
        return It;
    }

    inline const Snapshot* operator[](qint32 code) const {
        Snapshot tofnd; 
        tofnd.instrument_.second = code;
        BaseT::const_iterator It = find(tofnd);
        return (It == BaseT::end() ? NULL : &(*It));
    }

    inline Snapshot* operator[](qint32 code) {
        Snapshot tofnd; 
        tofnd.instrument_.second = code;
        BaseT::iterator It = find(tofnd);
        return (It == BaseT::end() ? NULL : const_cast<Snapshot*>(&(*It)));
    }
};

///////////////////////////////////////////////////////////////////////////
class MarketAbstractModel
{
public:
    MarketAbstractModel() { qRegisterMetaType<Instrument>("Instrument"); }
    virtual ~MarketAbstractModel() {}

    virtual void onInstrumentAdd(const Instrument& inst) = 0;
    virtual void onInstrumentRemove(const Instrument& inst, qint16 orderRow) = 0;
    virtual void onInstrumentChange(const Instrument& old, const Instrument& cur) = 0;
    virtual void onInstrumentUpdate() = 0;
    virtual void onInstrumentUpdate(const Instrument& inst) = 0;

    virtual bool   isMonitored(const Instrument& inst) const = 0;
    virtual qint16 monitoredCount() const = 0;

    // Method verifies that instrument already under monitoring then declines if it is
    // 'forced' flag disables this validation: reqests will be created once again if 
    // was already enabled, and tries to remove a row from the table never mind the row exists or no 
    // Note: monitoring routing faster with forced mode enabled because it doesn't search instrument 
    // in the internal monitoring set
    virtual void setMonitoring(const Instrument& inst, bool enable, bool forced) = 0;

    // Disable monitoring for all incoming messages and requests from RPC client
    // Useful on time of short term editing of original instruments
    virtual bool isMonitoringEnabled() const = 0;
    virtual void setMonitoringEnabled(bool enable) = 0;

    // Gets snapshot by the table data fetching (readonly)
    // check autolock after calling - it must be not empty when getSnapshot has owned the section
    virtual Snapshot* getSnapshot(const char* sym, QSharedPointer<QReadLocker>& autolock) const = 0;

    virtual const char* getSymbol(qint32 code) const = 0;
    virtual qint32 getCode(const char* sym) const = 0;
    virtual qint32 getCode(const QString& sym) const = 0;
    virtual qint16 getOrderRow(const char* sym) const = 0;
    virtual qint16 getOrderRow(const QString& sym) const = 0;
    virtual Instrument getByOrderRow(qint16 row) const = 0;

    // Fetching a vector with supported symbols 
    // Important: returns the vector of pointers without copying because it may nether changing or very seldom
    virtual void getSymbolsSupported(QVector<const char*>& out) const = 0;

    // Fetching a vector with symbols under monitoring (which already requested)
    // Note: returns the vector of std::strings because whatever has been changed in table may does modify any values at realtime
    virtual void getSymbolsUnderMonitoring(QVector<std::string>& out) const = 0;

    // Add - when symbol and code not found
    // Edit - one of symbol or code found
    // Remove - both symbol and code are present in a single instrument
    // @globalCommit signals that commit must be saved into INI too
    virtual void instrumentCommit(const Instrument& param, bool globalCommit) = 0;

private:
};

#endif // __marketabstractmodel_h__
