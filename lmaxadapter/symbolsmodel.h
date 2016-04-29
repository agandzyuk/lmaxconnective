#ifndef __symbolsmodel_h__
#define __symbolsmodel_h__

#include "baseini.h"
#include "marketabstractmodel.h"

#include <QReadWriteLock>
#include <QMap>
#include <set>

///////////////////////////////////////////////////////////////////////////////////
typedef QMap<std::string,qint32> SymbolsT;

///////////////////////////////////////////////////////////////////////////////////
class SymHash : private QMap<qint32,const char*>
{
    typedef QMap<qint32,const char*> BaseT;
    typedef std::set<qint32> CodeSet;
public:
    SymHash() : BaseT() {}
    ////////////////////////////////////////////////////////////////////
    inline void insert(const QString& sym, qint32 code) {
        insert(sym.toStdString().c_str(), code);
    }
    inline void insert(const char* sym, qint32 code) {
        SymbolsT::iterator It = symbols_.insert(sym, code);
        BaseT::insert(code, It.key().c_str());
    }
    inline void remove(const QString& sym) {
        remove(sym.toStdString().c_str());
    }
    inline void remove(const char* sym) {
        SymbolsT::iterator It = symbols_.find(sym);
        if( It == symbols_.end() ) return;
        iterator base_It = find(It.value());
        if( base_It == end() ) return;
        erase(base_It);
        symbols_.erase(It);
    }
    inline void change(const char* oldsym, const char* newsym) {
        SymbolsT::iterator It = symbols_.find(oldsym);
        if( It == symbols_.end() ) 
            return;
        qint32 code = It.value();
        symbols_.erase(It);
        It = symbols_.insert(newsym, code);
        BaseT::operator[](code) = It.key().c_str();
    }
    inline void change(qint32 oldcode, qint32 newcode) {
        iterator It = find(oldcode);
        if( It == end() ) return;
        SymbolsT::iterator sym_It = symbols_.find(It.value());
        if( sym_It == symbols_.end() ) return;
        symbols_[It.value()] = newcode;
        erase(It);
        BaseT::insert(newcode, sym_It.key().c_str());
        CodeSet::iterator mIt = mounted_.find(oldcode);
        if( mIt != mounted_.end() ) {
            mounted_.erase(mIt);
            mounted_.insert(newcode);
        }
    }
    inline void unmount(const char* sym) { 
        CodeSet::iterator It = mounted_.find(operator[](sym));
        if( It != mounted_.end() )
            mounted_.erase(It);
    }
    inline bool unmountAll() {
        mounted_.clear();
    }
    inline qint32 operator[](const QString& sym) const {
        SymbolsT::const_iterator It = symbols_.find(sym.toStdString().c_str());
        return (It == symbols_.end() ? -1 : It.value());
    }
    inline qint32 operator[](const char* sym) const {
        SymbolsT::const_iterator It = symbols_.find(sym);
        return (It == symbols_.end() ? -1 : It.value());
    }
    inline const char* operator[](qint32 code) const { 
        const_iterator It = find(code);        
        return  (It == end() ? NULL : It.value());
    }
    inline void mount(const char* sym) {
        int i = operator[](sym);
        if( i >= 0 )
            mounted_.insert(i);
    }
    inline bool isMounted(const char* sym) const {
        return (mounted_.end() != mounted_.find(operator[](sym)));
    }
    inline qint16 mountedCount() const {
        return mounted_.size();
    }
    inline qint16 orderRow(const QString& sym) const { 
        qint32 i = operator[](sym);
        CodeSet::const_iterator It = mounted_.find(i);
        for(i = 0; It != mounted_.end() && It != mounted_.begin(); It--, ++i);
        return (It == mounted_.end() ? -1 : i);
    }
    inline qint16 orderRow(const char* sym) const { 
        qint32 i = operator[](sym);
        CodeSet::const_iterator It = mounted_.find(i);
        for(i = 0; It != mounted_.end() && It != mounted_.begin(); It--, ++i);
        return (It == mounted_.end() ? -1 : i);
    }
    inline const char* byOrderRow(qint16 row) const { 
        CodeSet::const_iterator It = mounted_.begin();
        for(int i = 0; It != mounted_.end(); ++i, ++It)
            if( i == row ) break;
        if(It == mounted_.end()) return NULL;
        const_iterator symIt = find(*It);
        return (symIt == end() ? NULL : symIt.value());
    }
    inline void symbolsSupported(QVector<const char*>& out) {
        const_iterator It = begin();
        for(; It != end(); ++It) {
            const char* sym = It.value();
            if( sym ) out.push_back(sym);
        }
    }
    inline void symbolsMounted(QVector<std::string>& out) {
        CodeSet::const_iterator It = mounted_.begin();
        for(; It != mounted_.end(); ++It) {
            const char* sym = operator[](*It);
            if( sym ) out.push_back(sym);
        }
    }

private:
    CodeSet  mounted_;
    SymbolsT symbols_;
};

////////////////////////////////////////////////////////////////////////
class SymbolsModel : public QAbstractTableModel, 
                     public BaseIni,
                     public MarketAbstractModel
{
    Q_OBJECT
public:
    SymbolsModel(QWidget* parent);
    ~SymbolsModel();

    inline const char* getSymbol(qint32 code) const;
    inline std::string getSymbol_unlocked(qint32 code) const;
    inline qint32 getCode(const char* sym) const;
    inline qint32 getCode(const QString& sym) const;

    inline void getSymbolsSupported(QVector<const char*>& out) const;
    inline void getSymbolsUnderMonitoring(QVector<std::string>& out) const;

    // Method verifies that instrument already under monitoring then declines if it is
    // 'forced' flag disables this validation: reqests will be created once again if 
    // was already enabled, and tries to remove a row from the table never mind a row exists or no 
    // Note: monitoring routing faster with forced mode enabled because it doesn't search instrument 
    // in the internal monitoring set
    void   setMonitoring(const Instrument& inst, bool enable, bool forced);
    inline bool isMonitored(const Instrument& inst) const;
    inline bool isMonitored_unlocked(const Instrument& inst) const;

    inline qint16 monitoredCount() const;
    inline qint16 getOrderRow(const QString& sym) const;
    inline qint16 getOrderRow(const char* sym) const;
    inline Instrument getByOrderRow(qint16 row) const;

    // Disable monitoring for all incoming messages and requests from RPC client
    // Useful on time of short term editing of original instruments
    inline bool isMonitoringEnabled() const;
    inline void setMonitoringEnabled(bool enable);

    // Add - when symbol and code not found
    // Edit - on of symbol or code found
    // Remove - both symbol and code are presenting a single instrument
    // @globalCommit signals that commit must be saved into INI too
    void instrumentCommit(const Instrument& param, bool globalCommit);

    inline bool isGlobalChanged(const QString& sym) const;
    inline bool isGlobalAdded(const QString& sym) const;

Q_SIGNALS:
    void activateRequest(const Instrument& instrument);
    void activateResponse(const Instrument& instrument);
    void notifySendingManual(const QByteArray& message);
    void notifyInstrumentAdd(const Instrument& inst);
    void notifyInstrumentRemove(const Instrument& inst, qint16);
    void notifyInstrumentChange(const Instrument& old, const Instrument& cur);
    void notifyServerLogout(const QString& reason);
    void unsubscribeImmediate(const Instrument& instrument);

protected:
    void loadIni();
    void saveIni();

private:
    inline qint16 monitoredCount_unlocked() const;
    inline qint16 getOrderRow_unlocked(const char* sym) const;

    void registrySaveDefaults();
    void registryReloadDefaults();
    bool isIniLoaded();
    bool isRegistryLoaded();
    void setEditStateGlobal(const QString& oldsym, const QString& newsym);
    void setRemoveStateGlobal(const QString& oldsym);

private:
    SymHash* hash_;
    QReadWriteLock*  hashLock_;
    QReadWriteLock*  monitoringStateLock_;
    std::set<QString> clobalCommitToAdd_;
    std::set<QString> clobalCommitToRemove_;
    std::set<QString> clobalCommitToChange_;
    bool suspend_;
};

inline qint16 SymbolsModel::monitoredCount() const
{
    QReadLocker g(hashLock_);
    return hash_->mountedCount();
}

inline qint16 SymbolsModel::monitoredCount_unlocked() const
{
    return hash_->mountedCount();
}

inline bool SymbolsModel::isMonitored(const Instrument& inst) const
{
    QReadLocker g(hashLock_);

    const char* sym = inst.first.c_str();
    qint32 code = (*hash_)[sym];
    if( inst.second != code )
        if( NULL == (sym = (*hash_)[code]))
            return false;
    return hash_->isMounted(sym);
}

inline bool SymbolsModel::isMonitored_unlocked(const Instrument& inst) const
{
    std::string sym = inst.first.c_str();
    qint32 code = (*hash_)[sym.c_str()];
    if( inst.second != code )
        if( (sym = (*hash_)[code]).empty() )
            return false;
    return hash_->isMounted(sym.c_str());
}

inline bool SymbolsModel::isMonitoringEnabled() const
{
    QReadLocker g(monitoringStateLock_);
    return !suspend_;
}

inline void SymbolsModel::setMonitoringEnabled(bool enable)
{
    QWriteLocker g(monitoringStateLock_);
    suspend_ = !enable;
}

inline const char* SymbolsModel::getSymbol(qint32 code) const
{
    QReadLocker g(hashLock_);
    return (*hash_)[code];
}

inline std::string SymbolsModel::getSymbol_unlocked(qint32 code) const
{
    return (*hash_)[code];
}

inline qint32 SymbolsModel::getCode(const QString& sym) const
{
    QReadLocker g(hashLock_);
    return (*hash_)[sym];
}

inline qint32 SymbolsModel::getCode(const char* sym) const
{
    QReadLocker g(hashLock_);
    return (*hash_)[sym];
}

inline qint16 SymbolsModel::getOrderRow(const char* sym) const
{
    QReadLocker g(hashLock_);
    return hash_->orderRow(sym);
}

inline qint16 SymbolsModel::getOrderRow(const QString& sym) const
{
    QReadLocker g(hashLock_);
    return hash_->orderRow(sym);
}

inline qint16 SymbolsModel::getOrderRow_unlocked(const char* sym) const 
{ 
    return hash_->orderRow(sym); 
}


inline Instrument SymbolsModel::getByOrderRow(qint16 row) const
{
    QReadLocker g(hashLock_);
    const char* sym = hash_->byOrderRow(row);
    return Instrument(sym ? sym : "", sym ? (*hash_)[sym] : -1);
}

inline void SymbolsModel::getSymbolsSupported(QVector<const char*>& out) const
{
    hash_->symbolsSupported(out);
}

inline void SymbolsModel::getSymbolsUnderMonitoring(QVector<std::string>& out) const
{
    QReadLocker g(hashLock_);
    hash_->symbolsMounted(out);
}

inline bool SymbolsModel::isGlobalChanged(const QString& sym) const
{
    bool newSymChange = (clobalCommitToChange_.end() != clobalCommitToChange_.find(sym));
    bool oldSymChange = (clobalCommitToAdd_.end() != clobalCommitToAdd_.find(sym) && 
                         clobalCommitToRemove_.end() != clobalCommitToRemove_.find(sym));
    return (newSymChange || oldSymChange);
}

inline bool SymbolsModel::isGlobalAdded(const QString& sym) const
{
    return (clobalCommitToAdd_.end() != clobalCommitToAdd_.find(sym) && 
            clobalCommitToRemove_.end() == clobalCommitToRemove_.find(sym));
}

#endif // __symbolsmodel_h__
