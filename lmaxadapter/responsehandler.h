#ifndef __responsehandler_h__
#define __responsehandler_h__

#include <QByteArray>

///////////////////////////
class ResponseHandler
{
public:
    virtual ~ResponseHandler() {}

protected:
    virtual void onLogon(const QByteArray& message) = 0;
    virtual void onLogout(const QByteArray& message) = 0;
    virtual void onHeartbeat(const QByteArray& message) = 0;
    virtual void onTestRequest(const QByteArray& message) = 0;
    virtual void onResendRequest(const QByteArray& message) = 0;
    virtual void onSequenceReset(const QByteArray& message) = 0;
    virtual void onMarketData(const QByteArray& message) = 0;
    virtual void onMarketDataReject(const QByteArray& message) = 0;
    virtual void onSessionReject(const QByteArray& message) = 0;
};

#endif /* __responsehandler_h__ */
