#ifndef __baseini_h__
#define __baseini_h__

#include <QSettings>

// Aliases for common using in global namespace
// [common] parameters
#define ServerParam         (BaseIni::Parameter::ServerDomain)
#define TargetCompParam     (BaseIni::Parameter::TargetCompID)
#define SenderCompParam     (BaseIni::Parameter::SenderCompID)
#define PasswordParam       (BaseIni::Parameter::Password)
#define HeartbeatParam      (BaseIni::Parameter::Heartbeat)
#define ProtocolParam       (BaseIni::Parameter::SecureProtocol)

// SSL protocol names
#define ProtoSSLv2          (BaseIni::Protocol::SSLv2)
#define ProtoSSLv3          (BaseIni::Protocol::SSLv3)
#define ProtoTLSv1_0        (BaseIni::Protocol::TLSv1_0)
#define ProtoTLSv1_x        (BaseIni::Protocol::TLSv1_x)
#define ProtoTLSv1_1        (BaseIni::Protocol::TLSv1_1)
#define ProtoTLSv1_1x       (BaseIni::Protocol::TLSv1_1x)
#define ProtoTLSv1_2        (BaseIni::Protocol::TLSv1_2)
#define ProtoTLSv1_2x       (BaseIni::Protocol::TLSv1_2x)
#define ProtoTLSv1_SSLv3    (BaseIni::Protocol::TLSv1_SSLv3)
#define ProtoSSLAny         (BaseIni::Protocol::SSLAny)


////////////////////////////////////////////////////////////////////////
class BaseIni
{
public:
    struct Parameter {
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
    BaseIni();

    QString value(const char* key) const;
    void setValue(const char* key, const QString& value);

protected:
    void saveIni();

    void registrySaveDefaults();
    void registryReloadDefaults();
    bool isIniLoaded();
    bool isRegistryLoaded();

protected:
    QSettings  registry_;
    QSettings  ini_;

    static const QString registryAppRoot;
    static const QString registryCommonGroup;
    static const QString registryInstrumentsGroup;
    static const QString registryUsedGroup;
};

#endif // __baseini_h__
