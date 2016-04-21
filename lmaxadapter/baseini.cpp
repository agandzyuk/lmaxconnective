#include "globals.h"
#include "baseini.h"

#include <QCoreApplication>

///////////////////////////////////////////////////////////////////////////////////
const QString BaseIni::registryAppRoot             = "HKEY_CURRENT_USER\\Software\\";
const QString BaseIni::registryCommonGroup         = "common";
const QString BaseIni::registryInstrumentsGroup    = "instruments";
const QString BaseIni::registryUsedGroup           = "last_used";

///////////////////////////////////////////////////////////////////////////////////
const char BaseIni::Parameter::ServerDomain[]   = "ServerDomain";
const char BaseIni::Parameter::TargetCompID[]   = "TargetCompID";
const char BaseIni::Parameter::SenderCompID[]   = "SenderCompID";
const char BaseIni::Parameter::Password[]       = "Password";
const char BaseIni::Parameter::Heartbeat[]      = "HeartbeatInterval";
const char BaseIni::Parameter::SecureProtocol[] = "SecureMethod";

const char BaseIni::Protocol::SSLv2[]          = "SSLv2";
const char BaseIni::Protocol::SSLv3[]          = "SSLv3";
const char BaseIni::Protocol::TLSv1_0[]        = "TLSv1.0";
const char BaseIni::Protocol::TLSv1_x[]        = "TLSv1.later";
const char BaseIni::Protocol::TLSv1_1[]        = "TLSv1.1";
const char BaseIni::Protocol::TLSv1_1x[]       = "TLSv1.1.later";
const char BaseIni::Protocol::TLSv1_2[]        = "TLSv1.2";
const char BaseIni::Protocol::TLSv1_2x[]       = "TLSv1.2.later";
const char BaseIni::Protocol::TLSv1_SSLv3[]    = "TLSv1_and_SSLv3";
const char BaseIni::Protocol::SSLAny[]         = "SSLvAll_and_TLSv1.0";

///////////////////////////////////////////////////////////////////////////////////
const char* DefaultParams[] = {
    "fix-marketdata.london-demo.lmax.com",
    "LMXBDM",
    "MyLogin", // mkcell
    "MyPassword", // mkcell777
    "10",
    BaseIni::Protocol::TLSv1_x
};

///////////////////////////////////////////////////////////////////////////////////
BaseIni::BaseIni()
    : ini_(FILENAME_SETTINGS, QSettings::IniFormat, QCoreApplication::instance()),
    registry_(QSettings::NativeFormat, QSettings::UserScope, 
              Global::organizationName(), Global::productFullName(), QCoreApplication::instance())
{
    if( !isIniLoaded() ) {
        if( !isRegistryLoaded() )
            registrySaveDefaults();
        registryReloadDefaults();
    }

    ini_.beginGroup(registryCommonGroup);
    ini_.endGroup();
}

void BaseIni::registrySaveDefaults()
{
    QString rkey = registryAppRoot;
    rkey += Global::organizationName() + "\\";
    rkey += Global::productFullName() + "\\Defaults\\";
    rkey += registryCommonGroup;
    CDebug() << rkey << " - save default settings into registry";

    registry_.beginGroup(registryCommonGroup);

    registry_.setValue(ServerParam, DefaultParams[0]);
    registry_.setValue(TargetCompParam, DefaultParams[1]);
    registry_.setValue(SenderCompParam, DefaultParams[2]);
    registry_.setValue(PasswordParam, DefaultParams[3]);
    registry_.setValue(HeartbeatParam, DefaultParams[4]);
    registry_.setValue(ProtocolParam, DefaultParams[5]);

    registry_.endGroup();
}

void BaseIni::registryReloadDefaults()
{
    QString rkey = registryAppRoot;
    rkey += Global::organizationName() + "\\";
    rkey += Global::productFullName() + "\\Defaults\\";
    rkey += registryCommonGroup;
    CDebug() << rkey << " - reload default settings from registry";

    registry_.beginGroup(registryCommonGroup);
    ini_.beginGroup(registryCommonGroup);

    QString getval = registry_.value(ServerParam,DefaultParams[0]).toString();
    ini_.setValue(ServerParam,getval);

    getval = registry_.value(TargetCompParam,DefaultParams[1]).toString();
    ini_.setValue(TargetCompParam,getval);

    getval = registry_.value(SenderCompParam,DefaultParams[2]).toString();
    ini_.setValue(SenderCompParam,getval);

    getval = registry_.value(PasswordParam,DefaultParams[3]).toString();
    ini_.setValue(PasswordParam,getval);

    getval = registry_.value(HeartbeatParam,DefaultParams[4]).toString();
    ini_.setValue(HeartbeatParam,getval);

    getval = registry_.value(ProtocolParam,DefaultParams[5]).toString();
    ini_.setValue(ProtocolParam,getval);

    ini_.endGroup();
    registry_.endGroup();
}

bool BaseIni::isIniLoaded()
{
    QStringList ls = ini_.childGroups();
    if( ls.end() == qFind(ls,registryCommonGroup) )
        return false;

    ini_.beginGroup(registryCommonGroup);
    ls = ini_.allKeys();
    ini_.endGroup();

    return !ls.empty();
}

bool BaseIni::isRegistryLoaded()
{
    QStringList ls = registry_.childGroups();
    if( ls.end() == qFind(ls,registryCommonGroup) )
        return false;

    registry_.beginGroup(registryCommonGroup);
    ls = registry_.allKeys();
    registry_.endGroup();

    return !ls.empty();
}

void BaseIni::saveIni()
{
    setValue(ServerParam, value(ServerParam));
    setValue(SenderCompParam, value(SenderCompParam));
    setValue(TargetCompParam, value(TargetCompParam));
    setValue(PasswordParam, value(PasswordParam));
    setValue(HeartbeatParam, value(HeartbeatParam));
    setValue(ProtocolParam, value(ProtocolParam));
}

QString BaseIni::value(const char* key) const
{
    QString getVal;

    QString keyname = registryCommonGroup + "/" + QString(key);
    getVal = ini_.value(keyname).toString();
    if( !getVal.isEmpty() )
        return getVal;

    if( 0 == stricmp(key,ServerParam) )
        getVal = registry_.value(ServerParam, DefaultParams[0]).toString();
    else if( 0 == stricmp(key,TargetCompParam) )
        getVal = registry_.value(TargetCompParam, DefaultParams[1]).toString();
    else if( 0 == stricmp(key,SenderCompParam) )
        getVal = registry_.value(SenderCompParam, DefaultParams[2]).toString();
    else if( 0 == stricmp(key,PasswordParam) )
        getVal = registry_.value(PasswordParam, DefaultParams[3]).toString();
    else if( 0 == stricmp(key,HeartbeatParam) )
        getVal = registry_.value(HeartbeatParam, DefaultParams[4]).toString();
    else if( 0 == stricmp(key,ProtocolParam) )
        getVal = registry_.value(ProtocolParam, DefaultParams[5]).toString();

    return getVal;
}

void BaseIni::setValue(const char* key, const QString& value)
{
    QString keyname = registryCommonGroup + "/" + QString(key);
    ini_.setValue(keyname, value);
}
