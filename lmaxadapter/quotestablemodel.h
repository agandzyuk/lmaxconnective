#ifndef __quotestablemodel_h__
#define __quotestablemodel_h__

#include "fixdatamodel.h"

#include <QList>
#include <QMutex>

class QAbstractItemView;
class QuotesTableView;
class Scheduler;
class MqlProxyServer;

class QuotesTableModel : public FixDataModel
{
    Q_OBJECT
public:
    QuotesTableModel(QSharedPointer<MqlProxyServer>& mqlProxy, QWidget* parent);
    ~QuotesTableModel();

    void resetView(QuotesTableView* view);
    QAbstractItemView* view() const;

protected:
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role = Qt::EditRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

protected slots:
    void onInstrumentAdd(const Instrument& inst);
    void onInstrumentRemove(const Instrument& inst, qint16 orderRow);
    void onInstrumentChange(const Instrument& old, const Instrument& cur);
    void onInstrumentUpdate();
    void onInstrumentUpdate(const Instrument& inst);

private:
    QSize columnProportionWidth(int column) const;
    void setColumnWidth(int column) const;
    void setRowHeight(int row) const;

    mutable QMutex monitorLock_;
    
    typedef QMap<qint32,QString> Code2SymT;
    mutable Code2SymT monitored_;
    QuotesTableView* view_;
};

#endif // __quotestablemodel_h__
