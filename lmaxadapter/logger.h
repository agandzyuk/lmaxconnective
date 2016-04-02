#ifndef __logger_h__
#define __logger_h__

#include <QDebug>
#include <QTextStream>
#include <QFile>

/////////////////////////////////////////////////////////////////
class CLogFile: public QFile
{
public:
    CLogFile();
    ~CLogFile();

protected:
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE;
};

/////////////////////////////////////////////////////////////////
class CDebug: public QTextStream
{
public:
    CDebug(bool enableTimestamp = true);
};

#endif // __logger_h__
