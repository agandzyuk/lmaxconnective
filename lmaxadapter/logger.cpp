#include "defines.h"
#include "logger.h"

#include <windows.h>
#include <string>
#include <algorithm>

#include <QtCore>

bool enableTimestamp = true;
QMutex mdbg;

////////////////////////////////////////////////////////////
CDebug::CDebug(bool timestamp)
    : QTextStream( Global::infoLogFile )
{
    enableTimestamp = timestamp;
}

////////////////////////////////////////////////////////////
CLogFile::CLogFile()
    : QFile(INFO_LOGGING_FILE)
{
    if( !open(QIODevice::ReadWrite|QIODevice::Append) ) {
        QString info = QString("Cannot open logging file %1\nLogging will be disabled!").arg(INFO_LOGGING_FILE);
        ::MessageBoxA(NULL, info.toStdString().c_str(), "Error", MB_OK|MB_ICONSTOP);
    }
}


CLogFile::~CLogFile()
{}

qint64 CLogFile::writeData(const char *data, qint64 len)
{
    std::string str(data,len);
#ifndef NDEBUG
    QMutexLocker lock(&mdbg);
    qDebug() << str.c_str();
#endif

    if( enableTimestamp )
        str = "[" + Global::timestamp() + "] -  " + str + "\n";
    else if( 0 == memcmp(data,"Session",4) )
        str += "\n";
    else
        str = "                        " + str + "\n";
    return QFile::writeData(str.c_str(),str.length());
}
