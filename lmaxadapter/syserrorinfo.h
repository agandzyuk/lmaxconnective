#ifndef __syserrorinfo_h__
#define __syserrorinfo_h__

#include <QString>

class SysErrorInfo 
{
public:
    SysErrorInfo(const QString& msg, int errNo);
    inline const QString& get() const
    { return info_; }

private:
    QString info_;
};

#endif // __syserrorinfo_h__
