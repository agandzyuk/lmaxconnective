#include "defines.h"
#include "ini.h"

#include <set>

using namespace std;

///////////////////////////////////////////////////////////////////////////////////
const QString IniGroup[3] = {
    "common",
    "instruments", 
    "last_used" 
};

const char QIni::Parameter::ServerDomain[]   = "ServerDomain";
const char QIni::Parameter::TargetCompID[]   = "TargetCompID";
const char QIni::Parameter::SenderCompID[]   = "SenderCompID";
const char QIni::Parameter::Password[]       = "Password";
const char QIni::Parameter::Heartbeat[]      = "HeartbeatInterval";
const char QIni::Parameter::SecureProtocol[] = "SecureMethod";

const char QIni::Protocol::SSLv2[]          = "SSLv2";
const char QIni::Protocol::SSLv3[]          = "SSLv3";
const char QIni::Protocol::TLSv1_0[]        = "TLSv1.0";
const char QIni::Protocol::TLSv1_x[]        = "TLSv1.later";
const char QIni::Protocol::TLSv1_1[]        = "TLSv1.1";
const char QIni::Protocol::TLSv1_1x[]       = "TLSv1.1.later";
const char QIni::Protocol::TLSv1_2[]        = "TLSv1.2";
const char QIni::Protocol::TLSv1_2x[]       = "TLSv1.2.later";
const char QIni::Protocol::TLSv1_SSLv3[]    = "TLSv1_and_SSLv3";
const char QIni::Protocol::SSLAny[]         = "SSLvAll_and_TLSv1.0";

///////////////////////////////////////////////////////////////////////////////////
const QString IniDefaults[] = {
    "fix-marketdata.london-demo.lmax.com",
    "LMXBDM",
    "mkcell",
    "mkcell777",
    "10",
    QIni::Protocol::TLSv1_x
};

const QString InstNameDefaults[] = {
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
    "" // end of list
};

const int InstIdsDefaults[] = {
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
class SymHash : public QMap<qint32,std::string>
{
    typedef QMap<qint32,std::string> BaseT;
    typedef std::set<qint32> CodeSet;

public:
    SymHash() : BaseT() 
    {}

    inline void unmount(const std::string& sym) { 
        CodeSet::iterator It;
        if( mounted_.end() != (It = mounted_.find(operator[](sym))) )
            mounted_.erase(It);
    }

    inline bool unmountAll() {
        mounted_.clear();
    }

    inline qint32 operator[](const std::string& sym) const {
        const_iterator It = qFind(*static_cast<const BaseT*>(this), sym);
        return (It == end() ? -1 : It.key());
    }

    inline std::string& operator[](qint32 code) { 
        return BaseT::operator[](code); 
    }

    inline const std::string operator[](qint32 code) const { 
        return BaseT::operator[](code); 
    }

    inline void mount(const std::string& sym) {
        mounted_.insert(operator[](sym));
    }

    inline bool isMounted(const std::string& sym) const {
        return (mounted_.end() != mounted_.find(operator[](sym)));
    }

    inline int sortOrder(const std::string& sym) const { 
        qint32 i = operator[](sym);
        CodeSet::const_iterator It = mounted_.find(i);
        for(i = 0; It != mounted_.end() && It != mounted_.begin(); It--, ++i);
        return (It == mounted_.end() ? -1 : i);
    }

private:
    CodeSet mounted_;
};

///////////////////////////////////////////////////////////////////////////////////
QIni::QIni()
    : QSettings(DEF_INI_NAME, QSettings::IniFormat)
{
    beginGroup("common");
    endGroup();

    beginGroup("instruments");

    QStringList keyList = childKeys();
    QStringList::iterator from_It = keyList.begin();
    for(; from_It != keyList.end(); ++from_It) {
        originL_.insert( *from_It, QSettings::value(*from_It).toString() );
        originR_.insert( QSettings::value(*from_It).toInt(), *from_It );
    }
    endGroup();

    if( originL_.empty() ) 
    {
        int i = 0;
        while( InstIdsDefaults[i] != 0 && !InstNameDefaults[i].isEmpty() )
        {
            originL_.insert( InstNameDefaults[i], QString("%1").arg(InstIdsDefaults[i]) );
            originR_.insert( InstIdsDefaults[i], InstNameDefaults[i] );
            i++;
        }
    }

    beginGroup("last_used");
    keyList = childKeys();
    from_It = keyList.begin();
    for(; from_It != keyList.end(); ++from_It)
        mounted_.push_back( *from_It );
    endGroup();
}

void QIni::Save()
{
    setValue(ServerParam, value(ServerParam, common), common);
    setValue(SenderCompParam, value(SenderCompParam, common), common);
    setValue(TargetCompParam, value(TargetCompParam, common), common);
    setValue(PasswordParam, value(PasswordParam, common), common);
    setValue(HeartbeatParam, value(HeartbeatParam, common), common);
    setValue(ProtocolParam, value(ProtocolParam, common), common);

    beginGroup("instruments");
    for(Code2SymbolT::const_iterator It = originR_.begin(); It != originR_.end(); ++It)
        QSettings::setValue(It.value(), It.key());
    endGroup();

    QReadLocker g(&mtLock_);
    beginGroup("last_used");
    for(QStringList::const_iterator It = mounted_.begin(); It != mounted_.end(); ++It)
        QSettings::setValue(*It, originL_[*It]);
    endGroup();
}

QString QIni::value(const char* key, GroupId group) const
{
    QString getVal;

    if( group == common ) 
    {
        QString keyname = IniGroup[group] + "/" + QString(key);
        getVal = QSettings::value(keyname).toString();
        if( getVal.isEmpty() )
        {
            if( 0 == stricmp(key,ServerParam) )
                getVal = IniDefaults[0];
            else if( 0 == stricmp(key,TargetCompParam) )
                getVal = IniDefaults[1];
            else if( 0 == stricmp(key,SenderCompParam) )
                getVal = IniDefaults[2];
            else if( 0 == stricmp(key,PasswordParam) )
                getVal = IniDefaults[3];
            else if( 0 == stricmp(key,HeartbeatParam) )
                getVal = IniDefaults[4];
            else if( 0 == stricmp(key,ProtocolParam) )
                getVal = IniDefaults[5];
        }
        return getVal;
    }

    Code2SymbolT::const_iterator It = originR_.find(atol(key));
    if( It == originR_.end() )
        return "";

    getVal = It.value();
    if( group == instruments ) 
        return getVal;

    
    if( group == last_used ) {
        QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));
        if( -1 != mounted_.indexOf(getVal) )
            return getVal;
    }

    return  "";
}

void QIni::setValue(const char* key, const QString& value, GroupId group)
{
    if( group == common ) {
        QString keyname = IniGroup[group] + "/" + QString(key);
        QSettings::setValue(keyname, value);
        return;
    }

    Code2SymbolT::iterator It = originR_.find( atol(key) );
    if( originR_.end() == It )
        return;

    QString oldValue = It.value();
    Symbol2CodeT::iterator old_It = originL_.find(oldValue);
    if( old_It != originL_.end() )
        originL_.erase(old_It);

    originL_.insert(value, key);
    originR_[atol(key)] = value;
    
    if( group == last_used ) {
        QWriteLocker g(&mtLock_);
        QStringList::iterator s_It = qFind(mounted_.begin(),mounted_.end(),oldValue);
        if( s_It != mounted_.end() ) 
            s_It = mounted_.erase(s_It);
        mounted_.insert(s_It, value);
    }
}

bool QIni::mountSymbol(const QString& symbol)
{
    QWriteLocker g(&mtLock_);
    
    if( -1 != mounted_.indexOf(symbol) )
        return false;
    int pos = mountedSortingOrderByName_nolock(symbol);
    mounted_.insert(pos,symbol);
    return true;
}

quint16 QIni::mountedCount() const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));
    return mounted_.count();
}

bool QIni::isMounted(const QString& symbol) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));
    return (-1 != mounted_.indexOf(symbol));
}

void QIni::mountedSymbols(QVector<QString>& symbols) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));
    symbols = mounted_.toVector();
}

void QIni::mountedSymbolsStd(vector<string>& symbols) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));

    for(QStringList::const_iterator It = mounted_.begin();
        It != mounted_.end(); symbols.push_back(It->toStdString()), ++It);
}

QString QIni::mountedSymbolByCode(const QString& code) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));

    QString symbol = originSymbolByCode(code);
    if( -1 == mounted_.indexOf(symbol) )
            return "";
    return symbol;
}

QString QIni::mountedCodeBySymbol(const QString& symbol) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));

    if(-1 == mounted_.indexOf(symbol))
        return "";
    return originCodeBySymbol(symbol);
}

QString QIni::mountedBySortingOrder(int number) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));

/*
    Code2SymbolT::const_iterator It = originR_.begin();
    for(int i = -1; It != originR_.end(); ++It)
        if( -1 != mounted_.indexOf(It.value()) && (++i == number) )
            break;
    return (It == originR_.end() ? "" : It.value());
    */
    return mounted_.at(number);
}

int QIni::mountedSortingOrderByName(const QString& symbol) const
{
    QReadLocker g(const_cast<QReadWriteLock*>(&mtLock_));
    return mountedSortingOrderByName_nolock(symbol);
}

int QIni::mountedSortingOrderByName_nolock(const QString& symbol) const
{
    int lcode = originL_[symbol].toInt();
    int i = 0;
    for(QStringList::const_iterator It = mounted_.begin(); It != mounted_.end(); ++It, ++i)
        if( lcode < originL_[*It].toInt() )
            break;
    return i;
}

void QIni::unmountAll()
{
    QWriteLocker g(&mtLock_);
    mounted_.clear();
}

void QIni::originSymbols(QVector<QString>& symbols) const
{
    symbols = originL_.keys().toVector();
}

quint16 QIni::originCount() const
{
    return originL_.count();
}

void QIni::originRemoveSymbol(const QString& symbol)
{
    Symbol2CodeT::iterator l_It = originL_.find(symbol);
    if( l_It == originL_.end() )
        return;

    Code2SymbolT::iterator r_It = originR_.find(l_It.value().toInt());
    if( r_It == originR_.end() )
        return;

    QWriteLocker g(&mtLock_);
    QStringList::iterator m_It = qFind(mounted_.begin(),mounted_.end(),symbol);
    if( m_It != mounted_.end() )
        mounted_.erase(m_It);

    originR_.erase(r_It);
    originL_.erase(l_It);
}

void QIni::originAddOrEdit(const QString& symbol, const QString& code)
{
    bool changedBySymbol = false;
    bool changedByCode = false;

    Symbol2CodeT::iterator It = originL_.begin();
    for(; It != originL_.end(); ++It)
        if(It.key() == symbol) {
            changedBySymbol = true;
            break;
        }
        else if(It.value() == code) {
            changedByCode = true;
            break;
        }

    // change instrument symbol
    if( changedByCode ) {
        const_cast<QIni*>(this)->setValue(code.toStdString().c_str(), symbol, instruments);
        const_cast<QIni*>(this)->setValue(code.toStdString().c_str(), symbol, last_used);
    }
    else if( changedBySymbol ) { // change instrument code
        originChangeCode(It.value().toInt(), code.toInt());
    }
    else // add instrument
    {
        originR_.insert(code.toInt(),symbol);
        originL_.insert(symbol,code);
    }
}

void QIni::originChangeCode(int oldCode, int newCode)
{
    Code2SymbolT::iterator It = originR_.find(oldCode);

    QString storeValue;
    if( originR_.end() != It )
    {
        storeValue = It.value();
        originR_.erase(It);
        originR_.insert(newCode, storeValue);
        originL_[storeValue] = QString("%1").arg(newCode);
    }
}

QString QIni::originSymbolByCode(const QString& code) const
{
    Code2SymbolT::const_iterator It;
    if( originR_.end() != (It = originR_.find(code.toInt())) )
        return It.value();
    return "";
}

QString QIni::originCodeBySymbol(const QString& symbol) const
{
    Symbol2CodeT::const_iterator It = originL_.begin();
    if( originL_.end() != (It = originL_.find(symbol)) )
        return It.value();
    return "";
}

QString QIni::originBySortingOrder(int number) const
{
    
    Code2SymbolT::const_iterator It = originR_.begin();
    for(int i = 0; It != originR_.end(); ++It, ++i)
        if( i == number )
            break;
    return It.value();
}

