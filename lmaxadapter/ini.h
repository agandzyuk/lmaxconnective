#ifndef __ini_h__
#define __ini_h__

#include <QSettings>
#include <QReadWriteLock>
#include <QMap>

// Aliases for common using in global namespace
// [common] parameters
#define ServerParam         (QIni::Parameter::ServerDomain)
#define TargetCompParam     (QIni::Parameter::TargetCompID)
#define SenderCompParam     (QIni::Parameter::SenderCompID)
#define PasswordParam       (QIni::Parameter::Password)
#define HeartbeatParam      (QIni::Parameter::Heartbeat)
#define ProtocolParam       (QIni::Parameter::SecureProtocol)

// SSL protocol names
#define ProtoSSLv2          (QIni::Protocol::SSLv2)
#define ProtoSSLv3          (QIni::Protocol::SSLv3)
#define ProtoTLSv1_0        (QIni::Protocol::TLSv1_0)
#define ProtoTLSv1_x        (QIni::Protocol::TLSv1_x)
#define ProtoTLSv1_1        (QIni::Protocol::TLSv1_1)
#define ProtoTLSv1_1x       (QIni::Protocol::TLSv1_1x)
#define ProtoTLSv1_2        (QIni::Protocol::TLSv1_2)
#define ProtoTLSv1_2x       (QIni::Protocol::TLSv1_2x)
#define ProtoTLSv1_SSLv3    (QIni::Protocol::TLSv1_SSLv3)
#define ProtoSSLAny         (QIni::Protocol::SSLAny)


////////////////////////////////////////////////////////////////////////
class QIni : public QSettings
{
    typedef QMap<int,QString> Code2SymbolT;
    typedef QMap<QString,QString> Symbol2CodeT;

public:
    enum GroupId {
        common = 0,
        instruments,
        last_used,
    };

    struct Parameter
    {
        static const char ServerDomain[];
        static const char TargetCompID[];
        static const char SenderCompID[];
        static const char Password[];
        static const char Heartbeat[];
        static const char SecureProtocol[];
    };

    struct Protocol {
        static const char SSLv2[];
        static const char SSLv3[];
        static const char TLSv1_0[];
        static const char TLSv1_x[];
        static const char TLSv1_1[];
        static const char TLSv1_1x[];
        static const char TLSv1_2[];
        static const char TLSv1_2x[];
        static const char TLSv1_SSLv3[];
        static const char SSLAny[];
        static const char SSLUndefined[];
    };

    // 
public:
QIni();
void Save();
QString value(const char* key, GroupId group = common) const;
void setValue(const char* key, const QString& value, GroupId group = common);

bool mountSymbol(const QString& symbol);
quint16 mountedCount() const;
bool isMounted(const QString& symbol) const;
void mountedSymbols(QVector<QString>& symbols) const;
void mountedSymbolsStd(std::vector<std::string>& symbols) const;
QString mountedSymbolByCode(const QString& code) const;
QString mountedCodeBySymbol(const QString& symbol) const;
QString mountedBySortingOrder(int number) const;
int mountedSortingOrderByName(const QString& symbol) const;
void unmountAll();
void originSymbols(QVector<QString>& symbols) const;
quint16 originCount() const;
void originRemoveSymbol(const QString& symbol);
void originAddOrEdit(const QString& symbol, const QString& code);
void originChangeCode(int oldCode, int newCode);
QString originSymbolByCode(const QString& code) const;
QString originCodeBySymbol(const QString& symbol) const;
QString originBySortingOrder(int number) const;

private:
int mountedSortingOrderByName_nolock(const QString& symbol) const;

    Code2SymbolT    originR_;
    Symbol2CodeT    originL_;
    QStringList      mounted_;
    QReadWriteLock  mtLock_;
};

#define NUM_OF_PARAMETERS 6
extern const QString IniParams[NUM_OF_PARAMETERS];
extern const QString IniDefaults[NUM_OF_PARAMETERS];

#endif // __ini_h__
