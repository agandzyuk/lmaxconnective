#include "globals.h"
#include "fixlogger.h"
#include "fix.h"

#include <QtCore>
#include <Windows.h>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
// Macros for Windows API Event
#define InitEvent() ((quintptr)CreateEvent(NULL,FALSE,FALSE,NULL))
#define DeleteEvent(waiter) (CloseHandle((HANDLE)waiter))
#define SignalEvent(waiter) (SetEvent((HANDLE)waiter))
#define WaitForEvent(waiter) (WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)waiter,INFINITE))
#define TimedWaitForEvent(waiter,tm) (WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)waiter,tm))

////////////////////////////////////////////////////////////////////////////////
const char sampleTimestamp[] = 
    "[YYYYMMDD-HH:MM:SS.msc]";

enum FixType {
    Outgoing  = 1,   // 0000001 - outgoing mask
    Type_1    = 3,   // 0000011 
    Type_2    = 5,   // 0000101
    Type_5    = 7,   // 0000111
    Type_A    = 9,   // 0001001
    Type_V    = 11,  // 0001011 
    Incoming  = 128, // 1000000 - incoming mask
    Type_1in  = 130, // 1000010 - (2|128)
    Type_2in  = 132, // 1000100 - (4|128)
    Type_5in  = 134, // 1000110 - (6|128)
    Type_Ain  = 136, // 1001000 - (8|128)
    Type_3    = 138, // 1001100 - incoming only (12|128)
    Type_4    = 140, // 1001110 - incoming only (14|128)
    Type_W    = 142, // 1010000 - incoming only (16|128)
    Type_Y    = 144, // 1010010 - incoming only (18|128)
};

struct FixRecord
{
    qint32  position_;
    qint32  size_;
    quint8  type_;
    qint64  time_;
    qint32  code_;
    std::string symbol_;
    QByteArray  message_;
};

class AutoLocker {
public:
    AutoLocker(QReadWriteLock* autolock) : autolock_(autolock) {}

    inline void lockRead() {
        writelock_.reset();
        if( readlock_.isNull() ) 
            readlock_.reset(new QReadLocker(autolock_) );
    }

    inline void lockWrite() {
        readlock_.reset();
        if( writelock_.isNull() ) 
            writelock_.reset(new QWriteLocker(autolock_) );
    }

    inline void unlock() {
        readlock_.reset();
        writelock_.reset();
    }

private:
    QReadWriteLock* autolock_;
    QScopedPointer<QReadLocker> readlock_;
    QScopedPointer<QWriteLocker> writelock_;
};

///////////////////////////////////////////////////////////
FixLogFile::FixLogFile()
    : QFile(FILENAME_FIXMESSAGES),
    createdOnce_(false)
{
    Global::truncateMbFromLog(FILENAME_FIXMESSAGES, MAX_FIXMESSAGES_FILESIZE);
    checkLoggingEnabled();
}

FixLogFile::~FixLogFile()
{
    if(isOpen()) {
        char buf[128];
        sprintf_s(buf,128,"\nClosing time %s\n\n\n", Global::timestamp().c_str());
        QFile::writeData(buf, strlen(buf));
    }
}

bool FixLogFile::checkLoggingEnabled()
{
    if( Global::logging_ && !isOpen() ) {
        if( !open(QIODevice::ReadWrite|QIODevice::Append|QIODevice::Unbuffered) ) {
            std::string text = "Cannot open file \"" + std::string(FILENAME_FIXMESSAGES) + "\" for logging\n"
                "Fix messages logging will be unavailable for opened sessions";
            ::MessageBoxA(NULL, text.c_str(), "Error", MB_OK|MB_ICONSTOP);
            return false;
        }
    }
    else if( !Global::logging_ && isOpen() ) {
        close();
    }

    if( Global::logging_ && !createdOnce_ )  {
        char buf[256];
        sprintf_s(buf, 256,"///////////////////////////////////////////////////////////\n"
                           "// %s                           //\n"
                           "// Log session created. Start time %s //\n"
                           "///////////////////////////////////////////////////////////\n\n",
                           Global::productFullName().toStdString().c_str(),
                           Global::timestamp().c_str());
        QFile::writeData(buf, strlen(buf));
        createdOnce_ = true;
    }
    return Global::logging_;
}

///////////////////////////////////////////////////////////
FixLog::FixLog(QObject* parent)
    : QThread(parent),
    qLock_(new QReadWriteLock()),
    qEvent_(InitEvent()),
    reader_(FILENAME_FIXMESSAGES),
    readerPos_(0),
    online_(false)

{
    setDevice(&writer_);

    if( pos() > 0 && openReader() )
        findRecordsOffline();

    readerPos_ = 0;
    online_ = 1;
    start();
}

FixLog::~FixLog()
{
    stop();
}

void FixLog::stop()
{
    online_ = -1;

    if( writer_.checkLoggingEnabled() )
    {
        bool qempty;
        do {
            msleep(20);
            {
                QReadLocker g(qLock_);
                qempty = queue_.empty();
            }
        } 
        while(!qempty);
    }

    {
        QWriteLocker g(qLock_); 
        SignalEvent(qEvent_);
    }
    exit();
    wait();

    delete qLock_;
    qLock_ = NULL;
}

void FixLog::run()
{
    bool qempty;
    AutoLocker autolock(qLock_);

    do{ 
        autolock.lockRead();
        qempty = queue_.empty();
        if( qempty )
        {
            autolock.unlock();
            if( !WaitForEvent(qEvent_) || (online_ == -1) )
                break;
        }
        else
        {
            dequeue(autolock);
        }
    } 
    while(online_ == 1);
    QThread::exec();
}

QTextStream& FixLog::operator<<(const QByteArray& data)
{
    QString str(data);
    str = "[" + QString(Global::timestamp().c_str()) + "]  -- " + str;
    return *(QTextStream*)(this) << str;
}

void FixLog::inmsg(const QByteArray& fixmessage, const char* sym, qint32 code)
{
    if(online_ == -1)
        return;

    FixRecord rec = prepare();
    rec.type_ = Incoming;
    rec.code_ = code;
    if(sym) rec.symbol_ = sym;

    AutoLocker autolock(qLock_);
    if(online_ == 1) {
        autolock.lockWrite();
        queue_.push_back(rec);
        queue_.back().message_ = fixmessage;
        SignalEvent(qEvent_);
    }
    else if(online_ == 0) {
        queue_.push_back(rec);
        queue_.back().message_ = fixmessage;
        dequeue(autolock);
    }
}

void FixLog::outmsg(const QByteArray& fixmessage, const char* sym, qint32 code)
{
    if(online_ == -1)
        return;

    FixRecord rec = prepare();
    rec.type_ = Outgoing;
    rec.code_ = code;
    if(sym) rec.symbol_ = sym;

    AutoLocker autolock(qLock_);
    if(online_ == 1) {
        autolock.lockWrite();
        queue_.push_back(rec);
        queue_.back().message_ = fixmessage;
        SignalEvent(qEvent_);
    }
    else if(online_ == 0) {
        queue_.push_back(rec);
        queue_.back().message_ = fixmessage;
        dequeue(autolock);
    }
}

void FixLog::dequeue(AutoLocker& autolock)
{
    FixRecord& rec = queue_.front();
    if( !parse(rec) ) {
        autolock.lockWrite();
        queue_.pop_front();
        return;
    }

    QString str = rec.message_;
    rec.message_.clear();
    rec.size_ = str.length();

    if( online_ && writer_.checkLoggingEnabled() ) {
        if( rec.type_ & Incoming )
            str = "[" + QString(Global::timestamp(rec.time_).c_str()) + "]  << " + str + "\n";
        else if( rec.type_ & Outgoing )
            str = "[" + QString(Global::timestamp(rec.time_).c_str()) + "]  >> " + str + "\n";
        else
            str = "[" + QString(Global::timestamp(rec.time_).c_str()) + "]     " + str + "\n";

        *(QTextStream*)(this) << str;
        flush();
        rec.position_ = (qint32)writer_.size()-1;
        autolock.lockWrite();
    }
    else {
        rec.position_ = reader_.pos();
    }

    RecordsMap::iterator mIt = msgmap_.insert(rec.position_, rec);
    if( readerPos_ && !viewmap_.empty() ) 
    {
        if( mIt.value().position_ > readerPos_ )
            if((rec.code_ == -1) || 
               ((rec.type_ & matching_.type_) &&
                (rec.code_ == matching_.code_ || rec.symbol_ == matching_.symbol_)
               ))
            {
                viewmap_.insert(mIt.value().position_, &mIt.value());
                queue_.pop_front();
                autolock.unlock();
                emit notifyHasNext(true);
            }
            else
                queue_.pop_front();
    }
    else
        queue_.pop_front();
}

FixRecord FixLog::prepare()
{
    FixRecord record;
    record.position_ = 0;
    record.time_ = Global::systemtime();
    return record;
}

bool FixLog::parse(FixRecord& rec)
{
    std::string val = FIX::getField(rec.message_, "35");
    switch( val.c_str()[0] )
    {
    case '1': rec.type_ |= Type_1; break;
    case '2': rec.type_ |= Type_2; break;
    case '3': rec.type_ |= Type_3; break;
    case '4': rec.type_ |= Type_4; break;
    case '5': rec.type_ |= Type_5; break;
    case 'A': rec.type_ |= Type_A; break;
    case 'V': rec.type_ |= Type_V; break;
    case 'W': rec.type_ |= Type_W; break;
    case 'Y': rec.type_ |= Type_Y; break;
    default:
        return false;
    }
    return true;
}

bool FixLog::openReader()
{
    if( Global::logging_ && !reader_.isOpen() && !reader_.open(QIODevice::ReadOnly|QIODevice::Text) )
    {
        CDebug() << "Error opening file " << FILENAME_FIXMESSAGES << ": " << reader_.errorString();
        return false;
    }
    return Global::logging_;
}

void FixLog::findRecordsOffline()
{
    reader_.seek(0);
    QByteArray tail = reader_.read( pos()-1 );
    if( tail.size() < 100 )
        return;

    const char* ptr = tail.data();
    while( ptr-tail.data() < tail.size() ) 
    {
        if( !(ptr = strstr(ptr, "8=FIX.4.4")) ) 
            break;

        readerPos_ = ptr - tail.data();
        reader_.seek(readerPos_);

        QByteArray message = reader_.readLine();
        if(message.isEmpty()) return;
         
        FixRecord rec;
        std::string val = FIX::getField(message, "35");
        std::string sym;
        qint32 code = -1;
        if( val[0] == 'V' || val[0] == 'W' || val[0] == 'Y' ) {
            sym = FIX::getField(message, "262");
            code = atol( FIX::getField(message, "48").c_str() );
            if( code < 1 ) code = -1;
        }

        if( 0 == memcmp(ptr-3, ">> ", 3) )
            outmsg(message, sym.c_str(), code);
        else
            inmsg(message, sym.c_str(), code);
        ptr += message.size() + sizeof(sampleTimestamp);
    }
}

bool FixLog::selectViewBy(const char* sym, qint32 code, bool incoming)
{
    if( !const_cast<FixLog*>(this)->openReader() )
        return false;

    matching_.symbol_ = sym;
    matching_.code_ = code;
    matching_.type_ = incoming ? Incoming : Outgoing;

    AutoLocker autolock(qLock_);
    autolock.lockRead();

    RecordsMap::const_iterator It = msgmap_.begin(); 
    for(; It != msgmap_.end(); ++It) 
    {
        if( (It->code_ == -1) || 
            ((It->type_ & matching_.type_) && (It->code_ == code || It->symbol_ == sym)) )
        {
            autolock.lockWrite();
            viewmap_.insert(It.key(), &It.value());
        }
    }

    return true;
}

void FixLog::deselectView()
{
    QWriteLocker g(qLock_);
    viewmap_.clear();
    readerPos_ = 0;
}

QByteArray FixLog::getReadFirst()
{
    QReadLocker g(qLock_);

    qint32 countOf = viewmap_.size();
    if( countOf == 0 )
        return QByteArray();
    
    RecordsPtrs::const_iterator It = viewmap_.begin(); 
    readerPos_ = It.value()->position_;

    reader_.seek(readerPos_ - It.value()->size_);
    QByteArray ln = reader_.readLine(It.value()->size_+2);
    
    g.unlock();
    emit notifyHasPrev(false);
    emit notifyHasNext(countOf > 0);
    return ln;
}

QByteArray FixLog::getReadLast()
{
    QReadLocker g(qLock_);

    qint32 countOf = viewmap_.size();
    if( countOf == 0 )
        return QByteArray();
    
    RecordsPtrs::const_iterator It = --viewmap_.end(); 
    readerPos_ = It.value()->position_;

    reader_.seek(readerPos_ - It.value()->size_);
    QByteArray ln = reader_.readLine(It.value()->size_+2);

    g.unlock();
    emit notifyHasPrev(countOf > 0);
    emit notifyHasNext(false);
    return ln;
}

QByteArray FixLog::getReadNext()
{
    QReadLocker g(qLock_);

    if( readerPos_ == 0 )
        return QByteArray();

    RecordsPtrs::const_iterator It = viewmap_.find(readerPos_); 
    if( It == viewmap_.end() )
        return QByteArray();

    ++It;
    if( It == viewmap_.end() ) {
        g.unlock();
        emit notifyHasNext(false);
        return QByteArray();
    }

    readerPos_ = It.value()->position_;
    reader_.seek(readerPos_ - It.value()->size_);

    QByteArray ln = reader_.readLine(It.value()->size_+2);
    ++It;

    g.unlock();
    emit notifyHasPrev(true);
    emit notifyHasNext(It != viewmap_.end());
    return ln;
}

QByteArray FixLog::getReadPrev()
{
    AutoLocker autolock(qLock_);
    autolock.lockRead();

    if( readerPos_ == 0 )
        return QByteArray();

    if( viewmap_.size() == 1 ) {
        autolock.unlock();
        emit notifyHasPrev(false);
        emit notifyHasNext(false);
        return QByteArray();
    }

    RecordsPtrs::const_iterator It = viewmap_.find(readerPos_); 
    if( It == viewmap_.end() )
        return QByteArray();

    bool hasPrev = (It != viewmap_.begin());
    if( hasPrev ) {
        --It;
        hasPrev = (It != viewmap_.begin());
    }

    readerPos_ = It.value()->position_;
    reader_.seek(readerPos_ - It.value()->size_);
    QByteArray ln = reader_.readLine(It.value()->size_+2);

    autolock.unlock();
    emit notifyHasNext(true);
    emit notifyHasPrev(hasPrev);
    return ln;
}
