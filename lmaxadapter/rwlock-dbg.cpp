#include "external.h"
#include "rwlock-dbg.h"


#include <QThread>
#include <windows.h>

FILE* RWLockDbg::fglob_     = NULL;
quint32 RWLockDbg::tmdiff_  = 0;

RWLockDbg::RWLockDbg()
{
    if( tmdiff_ == 0 )
        tmdiff_ = ::GetTickCount();
    if( fglob_ == NULL )
        fglob_ = fopen("rwlock.dbg","wc+");
}

RWLockDbg::~RWLockDbg()
{
    if( fglob_ )
        fclose(fglob_);
    tmdiff_ = 0;
    fglob_ = NULL;
}

void RWLockDbg::checkpoint(const char* name)
{
    newcheckpoint_ = name;
}

void RWLockDbg::lockForRead()
{
    quint32 diff = ::GetTickCount() - tmdiff_; 
    tmdiff_ += diff;
    char buf[50];
    sprintf_s(buf,50,"Rbefore %s %u: %u ms\n", 
        newcheckpoint_.c_str(), (quint32)QThread::currentThreadId(), diff);
    fwrite(buf,1,strlen(buf),fglob_); fflush(fglob_);

    QReadWriteLock::lockForWrite();
    oldcheckpoint_ = newcheckpoint_;

    diff = ::GetTickCount() - tmdiff_; 
    tmdiff_ += diff;
    sprintf_s(buf,50,"Rafter %s %u: %u ms\n", 
        newcheckpoint_.c_str(), (quint32)QThread::currentThreadId(), diff);
    fwrite(buf,1,strlen(buf),fglob_); fflush(fglob_);

    fflush(fglob_);
}

void RWLockDbg::lockForWrite()
{
    quint32 diff = ::GetTickCount() - tmdiff_; 
    tmdiff_ += diff;
    char buf[50];
    sprintf_s(buf,50,"Wbefore %s %u: %u ms\n", 
        newcheckpoint_.c_str(), (quint32)QThread::currentThreadId(), diff);
    fwrite(buf,1,strlen(buf),fglob_); fflush(fglob_);

    QReadWriteLock::lockForWrite();
    oldcheckpoint_ = newcheckpoint_;

    diff = ::GetTickCount() - tmdiff_; 
    tmdiff_ += diff;
    sprintf_s(buf,50,"Wafter %s %u: %u ms\n", 
        newcheckpoint_.c_str(), (quint32)QThread::currentThreadId(), diff);
    fwrite(buf,1,strlen(buf),fglob_); fflush(fglob_);

    fflush(fglob_);
}

void RWLockDbg::unlock()
{
    QReadWriteLock::unlock();

    quint32 diff = ::GetTickCount() - tmdiff_; tmdiff_ += diff;
    char buf[50];
    sprintf_s(buf,50,"Unlock %s %u: %u ms\n", 
        oldcheckpoint_.c_str(), (quint32)QThread::currentThreadId(), diff);
    fwrite(buf,1,strlen(buf),fglob_); fflush(fglob_);
    fflush(fglob_);
}
