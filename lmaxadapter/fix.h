#ifndef __fix_h__
#define __fix_h__

#include <QByteArray>
#include <QString>
#include <QList>
#include <QReadWriteLock>
    
#include <string>

#define SOH (char(0x01))

class QIni;

///////////////////////////////////////////////////////////
class FIX
{
public:
    FIX(QIni& ini);
    virtual ~FIX();

    static quint16 GetChecksum(const char* buf, int buflen);
    static std::string MakeTime();
    static std::string getField(const QByteArray& message, 
                                const char* field, 
                                unsigned char reqEntryNum = 0);

    QByteArray  MakeLogon();
    QByteArray  MakeTestRequest();
    QByteArray  MakeLogout() const;
    QByteArray  MakeHeartBeat() const;
    QByteArray  MakeMarketDataRequest(const char* symbol, const char* code) const;

    bool LoggedIn() const;
    void SetLoggedIn(bool on);

    bool TestRequestSent() const;
    void SetTestRequestSent(bool on);
    QByteArray  TakeOutgoing();

    // heartbeat functions
    qint32 lastIncoming() const
    { return lastIncomingTime_; }
    qint32 lastOutgoing() const
    { return lastOutgoingTime_; }
    int heartbeatInterval() const
    { return hbi_; }

protected:
    QByteArray MakeHeader(char msgType) const;
    QByteArray MakeOnTestRequest(const char* testReqID) const;
    void CompleteMessage(QByteArray& message) const;
    void ResetMsgSeqNum(quint32 newMsgSeqNum);

protected:
    QIni& ini_;
    QList<QByteArray> outgoing_;

    qint32  lastIncomingTime_;
    qint32  lastOutgoingTime_;
    int     hbi_;
    QReadWriteLock flagMutex_;

private:
    bool loggedIn_;
    bool testRequestSent_;
    mutable quint32 msgSeqNum_;
};

#endif // __fix_h__
