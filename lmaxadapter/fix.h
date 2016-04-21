#ifndef __fix_h__
#define __fix_h__

#include "responsehandler.h"
#include <QList>
   
#include <string>

#define SOH (char(0x01))

class BaseIni;

QT_BEGIN_NAMESPACE;
class QReadWriteLock;
QT_END_NAMESPACE;

///////////////////////////////////////////////////////////
class FIX : public ResponseHandler
{
public:
    FIX();
    ~FIX();

    static std::string getField(const QByteArray& message, 
                                const char* field, 
                                unsigned char reqEntryNum = 0);
    static std::string makeTime();

    QByteArray  makeLogon();
    QByteArray  makeTestRequest();
    QByteArray  makeLogout() const;
    QByteArray  makeHeartBeat() const;
    QByteArray  makeMarketSubscribe(const char* symbol, qint32 code) const;
    QByteArray  makeMarketUnSubscribe(const char* symbol, qint32 code) const;
    QByteArray  takeOutgoing();

    bool normalize(QByteArray& rawFix);
    void setLoggedIn(bool on);
    void resetMsgSeqNum(quint32 newMsgSeqNum);
    void setTestRequestSent(bool on);
    bool loggedIn() const;
    bool testRequestSent() const;

    static inline quint16 getChecksum(const char* buf, int buflen) {
	    quint32 cks = 0;
	    for(int i = 0; i < buflen; ++i) cks += (unsigned char)buf[i];
	    return cks % 256;
    }
    inline qint32 getLastIncoming() const { 
        return lastIncomingTime_; 
    }
    inline qint32 getLastOutgoing() const { 
        return lastOutgoingTime_; 
    }
    inline int getHeartbeatInterval() const { 
        return hbi_; 
    }
    inline void setIniModel(const BaseIni* ini) {
        ini_ = ini;
    }

protected:
    QByteArray makeHeader(const char* msgType) const;
    QByteArray makeOnTestRequest(const char* testReqID) const;
    void completeMessage(QByteArray& message) const;

protected:
    QList<QByteArray> outgoing_;

    qint32  lastIncomingTime_;
    qint32  lastOutgoingTime_;
    int     hbi_;
    QReadWriteLock* flagLock_;

private:
    bool  loggedIn_;
    bool  testRequestSent_;
    const BaseIni* ini_;
    mutable quint32 msgSeqNum_;
};

#endif // __fix_h__
