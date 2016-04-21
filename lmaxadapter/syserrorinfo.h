#ifndef __syserrorinfo_h__
#define __syserrorinfo_h__

#include <string>

class SysErrorInfo 
{
public:
    SysErrorInfo(const std::string& msg, int errNo);
    inline const std::string& get() const
    { return info_; }

private:
    std::string info_;
};

#endif // __syserrorinfo_h__
