#ifndef __lmxmodel_h__
#define __lmxmodel_h__

#include "fixdispatcher.h"

#include <QAbstractTableModel>
#include <QList>
#include <QMutex>

class QAbstractItemView;
class Scheduler;

class LMXModel : public QAbstractTableModel, public FIXDispatcher
{
public:
    LMXModel(QIni& ini, QScheduler* scheduler);
    ~LMXModel();

    void resetView(QAbstractItemView* view);

protected:
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role = Qt::EditRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void onResponseReceived(const QString& symbol);

private:
    void setColumnWidth(int column) const;
    void setRowHeight(int row) const;

    mutable QMutex monitorLock_;
    mutable QStringList monitored_;
    QAbstractItemView* view_;
};

#endif // __lmxmodel_h__
