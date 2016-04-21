#include "globals.h"
#include "symbolsmodel.h"

#include <QWidget>

using namespace std;
///////////////////////////////////////////////////////////////////////////////////

const char* DefaultSymbols[] = {
	"AUDJPY",
	"AUDUSD",
	"CHFJPY",
	"EURAUD",
	"EURCAD",
	"EURCHF",
	"EURGBP",
	"EURJPY",
    "EURUSD",
	"GBPAUD",
	"GBPCAD",
	"GBPCHF",
	"GBPJPY",
	"GBPUSD",
	"USDCAD",
	"USDCHF",
	"USDJPY",
	"EURCZK",
	"GBPCZK",
	"USDCZK",
	"EURDKK",
	"GBPDKK",
	"USDDKK",
	"EURHKD",
    "GBPHKD",
	"USDHKD",
	"EURHUF",
	"GBPHUF",
	"USDHUF",
	"EURMXN",
	"GBPMXN",
	"USDMXN",
	"EURNOK",
	"GBPNOK",
	"USDNOK",
	"EURNZD",
	"GBPNZD",
	"EURPLN",
    "GBPPLN",
	"USDPLN",
	"EURSEK",
	"GBPSEK",
	"USDSEK",
	"EURSGD",
	"GBPSGD",
	"USDSGD",
	"EURTRY",
	"GBPTRY",
	"USDTRY",
	"EURZAR",
	"GBPZAR",
	"USDZAR",
	"NZDUSD",
	"AUDNZD",
	"NZDJPY",
	"AUDCHF",
	"AUDCAD",
	"CADCHF",
	"CADJPY",
	"NZDCAD",
	"NZDCHF",
	"NZDSGD",
	"EURRUB",
	"USDRUB",
	"USDCNH",
	"UK100",
	"Wall_Street_30",
	"US_SPX_500",
	"US_Tech_100",
	"Germany_30",
	"France_40",
	"Europe_50",
	"Japan_225",
	"Gold_Spot",
	"Silver_Spot",
	"XAUAUD",
	"XAGAUD",
	"US_Crude_Spot",
	"UK_Brent_Spot",
	"XPTUSD",
	"XPDUSD",
    0 // end of list
};

const int DefaultCodes[] = {
	4008,
	4007,
	4009,
	4016,
	4015,
	4011,
	4003,
	4006,
	4001,
	4017,
	4014,
	4012,
	4005,
	4002,
	4013,
	4010,
	4004,
	100479,
	100481,
	100483,
	100485,
	100487,
	100489,
	100491,
	100493,
	100495,
	100497,
	100499,
	100501,
	100503,
	100505,
	100507,
	100509,
	100511,
	100513,
	100515,
	100517,
	100519,
	100521,
	100523,
	100525,
	100527,
	100529,
	100531,
	100533,
	100535,
	100537,
	100539,
	100541,
	100543,
	100545,
	100547,
	100613,
	100615,
	100617,
	100619,
	100667,
	100671,
	100669,
	100673,
	100675,
	100677,
	100679,
	100681,
	100892,
	100089,
	100091,
	100093,
	100095,
	100097,
	100099,
	100101,
	100105,
	100637,
	100639,
	100803,
	100804,
	100800,
	100805,
	100806,
	100807,
    0 // end of list
};

///////////////////////////////////////////////////////////////////////////////////
SymbolsModel::SymbolsModel(QWidget* parent)
    : QAbstractTableModel(parent),
    hash_(new SymHash()),
    hashLock_(new QReadWriteLock()),
    monitoringStateLock_(new QReadWriteLock()),
    suspend_(false)
{
    if( !isIniLoaded() ) {
        if( !isRegistryLoaded() )
            registrySaveDefaults();
        registryReloadDefaults();
    }

    loadIni();
}

SymbolsModel::~SymbolsModel()
{
    saveIni();
    delete hashLock_;
    hashLock_ = NULL;
    {
        QWriteLocker g(monitoringStateLock_);
    }
    delete monitoringStateLock_;
    monitoringStateLock_ = NULL;
}

void SymbolsModel::registrySaveDefaults()
{
    QString rkey = registryAppRoot;
    rkey += Global::organizationName() + "\\";
    rkey += Global::productFullName() + "\\Defaults\\";
    rkey += registryInstrumentsGroup;
    CDebug() << rkey << " - save default instruments into registry";

    registry_.beginGroup(registryInstrumentsGroup);

    int n = 0;
    while( DefaultCodes[n] && DefaultSymbols[n] ) {
        registry_.setValue(DefaultSymbols[n], DefaultCodes[n]);
        n++;
    }
    registry_.endGroup();
}

void SymbolsModel::registryReloadDefaults()
{
    QString rkey = registryAppRoot;
    rkey += Global::organizationName() + "\\";
    rkey += Global::productFullName() + "\\Defaults\\";
    rkey += registryInstrumentsGroup;
    CDebug() << rkey << " - reload default instruments from registry";

    SymbolsT symMap;
    int n = 0;
    while( DefaultCodes[n] && DefaultSymbols[n] ) {
        symMap.insert(DefaultSymbols[n], DefaultCodes[n]);
        n++;
    }
    
    registry_.beginGroup(registryInstrumentsGroup);
    ini_.beginGroup(registryInstrumentsGroup);

    QStringList keyList = registry_.allKeys();
    for(QStringList::iterator It = keyList.begin(); It != keyList.end(); ++It)
    {
        SymbolsT::iterator defIt = symMap.find(It->toStdString().c_str());
        if( symMap.end() == defIt )
            n = registry_.value(*It).toInt();
        else
            n = registry_.value(*It, defIt.value()).toInt();

        ini_.setValue(defIt.key().c_str(), n);
    }

    ini_.endGroup();
    registry_.endGroup();
}

bool SymbolsModel::isIniLoaded()
{
    QStringList ls = ini_.childGroups();
    if( ls.end() == qFind(ls,registryInstrumentsGroup) )
        return false;

    ini_.beginGroup(registryInstrumentsGroup);
    ls = ini_.allKeys();
    ini_.endGroup();

    return !ls.empty();
}

bool SymbolsModel::isRegistryLoaded()
{
    QStringList ls = registry_.childGroups();
    if( ls.end() == qFind(ls,registryInstrumentsGroup) )
        return false;

    registry_.beginGroup(registryInstrumentsGroup);
    ls = registry_.allKeys();
    registry_.endGroup();

    return !ls.empty();
}

void SymbolsModel::loadIni()
{
    ini_.beginGroup(registryInstrumentsGroup);
    QStringList keyList = ini_.allKeys();
    QStringList::iterator It = keyList.begin();
    for(; It != keyList.end(); ++It)
        hash_->insert(*It, ini_.value(*It).toInt());
    ini_.endGroup();

    ini_.beginGroup(registryUsedGroup);
    keyList = ini_.allKeys();
    for(It = keyList.begin(); It != keyList.end(); ++It)
        hash_->mount(It->toStdString().c_str());
    ini_.endGroup();
}

void SymbolsModel::saveIni()
{
    QWriteLocker g(hashLock_);

    BaseIni::saveIni();

    // clear [instruments] and [last_used] before which was commited as global
    ini_.beginGroup(registryInstrumentsGroup);
    QStringList keylist = ini_.allKeys();
    ini_.endGroup();

    QStringList::iterator del_It = keylist.begin();
    for(; del_It != keylist.end(); ++del_It) 
    {
        // remove last_used all
        QString key = registryUsedGroup + "/" + *del_It;
        ini_.remove(key);

        if( clobalCommitToRemove_.end() == clobalCommitToRemove_.find(*del_It) ) 
            continue;

        key = registryInstrumentsGroup + "/" + *del_It;
        ini_.remove(key);
    }

    // and place it from container
    QVector<const char*> vv;
    hash_->symbolsSupported(vv);

    QVector<const char*>::iterator It = vv.begin();
    for(; It != vv.end(); ++It)
    {
        if( isGlobalAdded(*It) || isGlobalChanged(*It) ) {
            qint32 code = (*hash_)[*It];
            if(code != -1) {
                QString key = registryInstrumentsGroup + "/" + QString(*It);
                ini_.setValue(key,code);
            }
        }

        if( hash_->isMounted(*It) ) {
            qint32 code = (*hash_)[*It];
            if( code != -1 ){
                QString key = registryUsedGroup + "/" + QString(*It);
                ini_.setValue(key,code);
            }
        }
    }
}

void SymbolsModel::setMonitoring(const Instrument& inst, bool enable, bool forced)
{
    if( !forced  && 0 == (enable ^ isMonitored_unlocked(inst)) )
        return;

    Instrument fixed = inst;
    qint16 orderRow = -1;
    {
        QWriteLocker g(hashLock_);
        const char* sym = inst.first.c_str();
        qint32 code = (*hash_)[sym];
        if( code != -1 && code != inst.second ) {
            sym = (*hash_)[code];
            if( sym == NULL )
                return;
            fixed.first = sym;
        }
        
        if(!enable && forced && isMonitored_unlocked(inst))
        {
            orderRow = getOrderRow_unlocked(sym);
            g.unlock();
            emit unsubscribeImmediate(inst);
            hash_->unmount(sym);
        }
        else if(!enable) {
            orderRow = getOrderRow_unlocked(sym);
            hash_->unmount(sym);
        }
        else
            hash_->mount(sym);
    }
    // should be out of the lock
    if(enable) {
        emit notifyInstrumentAdd(fixed);
        emit activateRequest(fixed);
    }
    else
        emit notifyInstrumentRemove(fixed,orderRow);
}


void SymbolsModel::instrumentCommit(const Instrument& param, bool globalCommit)
{
    bool changedBySymbol = false;
    bool changedByCode = false;

    // disable any monitoring calls at commit
    setMonitoringEnabled(false);

    string oldsym = param.first.c_str();
    qint32 oldcode = (*hash_)[oldsym.c_str()];
    if(oldcode != -1) {
        changedBySymbol = true;
        if( oldcode == param.second )
            changedByCode = true;
    }
    else { 
        oldcode = param.second;
        const char* sz = (*hash_)[oldcode];
        if( sz ) {
            oldsym = sz;
            changedByCode = true;
            if( param.first == oldsym )
                changedBySymbol = true;
        }
    }

    Instrument oldinst(oldsym, oldcode);
    bool monitored = isMonitored(oldinst);

    if( changedBySymbol && changedByCode  ) // remove entire instrument
    {
        if(monitored)
            setMonitoring(oldinst, false, true);
        hash_->remove(oldsym.c_str());
        if( globalCommit ) 
            setRemoveStateGlobal(oldsym.c_str());
    }
    else if( changedBySymbol ) // change instrument code
    {
        hash_->change(oldcode, param.second);
        if(monitored)
            emit notifyInstrumentChange(oldinst, Instrument(oldsym, param.second));
        if( globalCommit ) {
            clobalCommitToRemove_.insert(oldsym.c_str());
            clobalCommitToAdd_.insert(oldsym.c_str());
            clobalCommitToChange_.insert(param.first.c_str());
        }
    }
    else if( changedByCode ) // change instrument symbol
    {
        hash_->change(oldsym.c_str(), param.first.c_str());
        if(monitored)
            emit notifyInstrumentChange(oldinst, Instrument(param.first.c_str(), oldcode));
        if( globalCommit ) 
            setEditStateGlobal(oldsym.c_str(), param.first.c_str());
    }
    else // add instrument
    {
        hash_->insert(param.first.c_str(), param.second);
        if( globalCommit )
            clobalCommitToAdd_.insert(oldsym.c_str());
    }

    // enale monitoring calls
    setMonitoringEnabled(true);
}

void SymbolsModel::setEditStateGlobal(const QString& oldsym, const QString& newsym)
{
    std::set<QString>::iterator It = clobalCommitToRemove_.find(newsym);
    if( It != clobalCommitToRemove_.end() ) 
    {
        clobalCommitToRemove_.erase(It);
        It = clobalCommitToAdd_.find(newsym);
        if( It != clobalCommitToAdd_.end() )
            clobalCommitToAdd_.erase(It);
        It = clobalCommitToChange_.find(newsym);
        if( It != clobalCommitToChange_.end() )
            clobalCommitToChange_.erase(It);
        return;
    }

    clobalCommitToRemove_.insert(oldsym);
    clobalCommitToAdd_.insert(oldsym);
    if( clobalCommitToChange_.end() == clobalCommitToChange_.find(newsym) )
        clobalCommitToChange_.insert(newsym);
}

void SymbolsModel::setRemoveStateGlobal(const QString& oldsym)
{
    clobalCommitToRemove_.insert(oldsym);

    std::set<QString>::iterator It = clobalCommitToChange_.find(oldsym);
    if( clobalCommitToChange_.end() != It )
        clobalCommitToChange_.erase(It);

    It = clobalCommitToAdd_.find(oldsym);
    if( clobalCommitToAdd_.end() != It )
        clobalCommitToAdd_.erase(It);
}

