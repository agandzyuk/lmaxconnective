#ifndef __fixlogger_h__
#define __fixlogger_h__

#include <QTextStream>
#include <QFile>
#include <QThread>
#include <QList>

struct FixRecord;
typedef public QMap<qint32,FixRecord> RecordsMap;
typedef public QMap<qint32,const FixRecord*> RecordsPtrs;

QT_BEGIN_NAMESPACE;
class QReadWriteLock;
QT_END_NAMESPACE;

/////////////////////////////////////////////////////////////////
class FixLogFile: public QFile
{
public:
    FixLogFile();
    ~FixLogFile();

    bool checkLoggingEnabled();
private:
    bool createdOnce_;
};

/////////////////////////////////////////////////////////////////
class AutoLocker;

class FixLog: public QThread, private QTextStream
{
    Q_OBJECT

    friend class FixMessageDialog;
public:
    FixLog(QObject* parent);
    ~FixLog();

    void inmsg(const QByteArray& fixmessage, const char* sym = NULL, qint32 code = -1);
    void outmsg(const QByteArray& fixmessage, const char* sym = NULL, qint32 code = -1);

    bool selectViewBy(const char* sym, qint32 code, bool incoming);
    void deselectView();

Q_SIGNALS:
    void notifyHasPrev(bool has);
    void notifyHasNext(bool has);

protected:
    QTextStream& operator<<(const QByteArray& line);
    QByteArray getReadLast();
    QByteArray getReadFirst();
    QByteArray getReadNext();
    QByteArray getReadPrev();

protected:
    void run();
    void stop();
    void dequeue(AutoLocker& autolock);

private:
    bool parse(FixRecord& rec);
    bool openReader();
    FixRecord prepare();
    void findRecordsOffline();

private:
    qint32 readerPos_;
    QFile reader_;
    FixLogFile writer_;

    QList<FixRecord> queue_;
    QReadWriteLock*  qLock_;
    quintptr qEvent_;

    RecordsMap msgmap_;
    RecordsPtrs viewmap_;

    struct Matching {
        std::string symbol_;
        qint32 code_;
        quint8 type_;
    } matching_;

    QAtomicInt online_;
};

#endif // __fixlogger_h__
