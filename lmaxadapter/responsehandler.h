#ifndef __responsehandler_h__
#define __responsehandler_h__

#include <QList>

///////////////////////////
struct MarketReport 
{
    QString symbol_;
    QString code_;
    QString bid_;
    QString ask_;
    QString status_;
    qint32  requestTime_;
    qint32  responseTime_;
    qint32  msgSeqNum_;

    bool operator==(const MarketReport& r) const
    { return (symbol_ == r.symbol_ ? code_ == r.code_ : false); }
};
typedef QList<MarketReport> ReportListT;

///////////////////////////
class ResponseHandler : public ReportListT
{
public:
    virtual ~ResponseHandler() {}
    virtual const MarketReport* snapshot(const QString& symbol, const QString& code) const = 0;
    virtual void onResponseReceived(const QString& symbol) = 0;
};

#endif /* __responsehandler_h__ */
