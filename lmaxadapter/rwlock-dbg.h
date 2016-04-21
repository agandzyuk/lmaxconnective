#ifndef __rwlock_dbg_h__
#define __rwlock_dbg_h__

#include <QReadWriteLock>
#include <string>

/////////////////////////////////////////////////////////////////
class RWLockDbg: public QReadWriteLock
{
public:
    RWLockDbg();
    ~RWLockDbg();

    void lockForRead();
    void lockForWrite();
    void unlock();
    void checkpoint(const char* name);

private:
    static FILE*    fglob_;
    static quint32  tmdiff_;
    std::string     newcheckpoint_;
    std::string     oldcheckpoint_;
};

/////////////////////////////////////////////////
class DReadLocker
{
public:
    inline DReadLocker(RWLockDbg *readWriteLock, const char* name);

    inline ~DReadLocker()
    { unlock(); }

    inline void unlock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(1u)) {
                q_val &= ~quintptr(1u);
                readWriteLock()->unlock();
            }
        }
    }

    inline void relock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(0u)) {
                readWriteLock()->lockForRead();
                q_val |= quintptr(1u);
            }
        }
    }

    inline RWLockDbg *readWriteLock() const
    { return reinterpret_cast<RWLockDbg *>(q_val & ~quintptr(1u)); }

private:
    Q_DISABLE_COPY(DReadLocker)
    quintptr q_val;
};

inline DReadLocker::DReadLocker(RWLockDbg *areadWriteLock, const char* name)
    : q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "DReadLocker", "RWLockDbg pointer is misaligned");
    areadWriteLock->checkpoint(name);
    relock();
}

/////////////////////////////////////////////////////////////////////////////
class DWriteLocker
{
public:
    inline DWriteLocker(RWLockDbg *readWriteLock, const char* name);

    inline ~DWriteLocker()
    { unlock(); }

    inline void unlock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(1u)) {
                q_val &= ~quintptr(1u);
                readWriteLock()->unlock();
            }
        }
    }

    inline void relock()
    {
        if (q_val) {
            if ((q_val & quintptr(1u)) == quintptr(0u)) {
                readWriteLock()->lockForWrite();
                q_val |= quintptr(1u);
            }
        }
    }

    inline RWLockDbg *readWriteLock() const
    { return reinterpret_cast<RWLockDbg *>(q_val & ~quintptr(1u)); }


private:
    Q_DISABLE_COPY(DWriteLocker)
    quintptr q_val;
};

inline DWriteLocker::DWriteLocker(RWLockDbg *areadWriteLock, const char* name)
    : q_val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "DWriteLocker", "RWLockDbg pointer is misaligned");
    areadWriteLock->checkpoint(name);
    relock();
}

#endif // __rwlock_dbg_h__
