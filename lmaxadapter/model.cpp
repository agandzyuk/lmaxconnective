#include "defines.h"
#include "model.h"
#include "ini.h"

#include <QtWidgets>

static int rows_before_view = 0;

//////////////////////////////////////////////////////////////
LMXModel::LMXModel(QIni& ini, QScheduler* scheduler)
    : FIXDispatcher(ini, scheduler)
{}

LMXModel::~LMXModel()
{}

void LMXModel::resetView(QAbstractItemView* view)
{
    QMutexLocker g(&monitorLock_);
    view_ = view;
    monitored_.clear();
    rows_before_view = ini_.mountedCount();
}

int LMXModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return ini_.mountedCount();
}

int LMXModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant LMXModel::data(const QModelIndex& index, int role) const
{
    if( role != Qt::DisplayRole )
        return QVariant();

    QMutexLocker g(&monitorLock_);
    int r = index.row(), c = index.column();
    if( c == 0 ) {
        if( monitored_.count() < rows_before_view ) {
            monitored_.insert(r, ini_.mountedBySortingOrder(r));
            setRowHeight(r);
        }
        else if( monitored_.count() > rows_before_view ) {
            setRowHeight(r);
            rows_before_view = monitored_.count();
        }
    }

    QString symbol = monitored_[r];
    QString code = ini_.mountedCodeBySymbol(symbol);
    if( c == 0 )
        return code;
    else if( c == 1 )
        return symbol;
    g.unlock();

    const MarketReport* report = snapshot(symbol, code);
    if( report == NULL )
        return QVariant();

    switch( c ) 
    {
    case 2:
        return report->ask_;
    case 3:
        return report->bid_;
    case 4:
        if( report->responseTime_ && report->requestTime_ )
            return report->responseTime_-report->requestTime_;
        break;
    case 5:
        return report->status_;
    }
    return QVariant();
}

QVariant LMXModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    QVariant hdrName;
    switch (section) {
    case 0:
        hdrName = tr("ID");
        break;
    case 1:
        hdrName = tr("Bus name");
        break;
    case 2:
        hdrName = tr("Ask");
        break;
    case 3:
        hdrName = tr("Bid");
        break;
    case 4:
        hdrName = tr("Msecs");
        break;
    case 5:
        hdrName = tr("Status");
        break;
     default:
        break;
    }

    setColumnWidth(section);
    return hdrName;
}

void LMXModel::setColumnWidth(int column) const
{
    if( NULL == view_)
        return;

    QTableView* tbl = qobject_cast<QTableView*>(view_);
    if( NULL == tbl ) {
        ::MessageBoxA(NULL, "Error object type <QTableView>", "Error", MB_OK|MB_ICONSTOP);
        return;
    }

    int clientWdt = tbl->width() - 20;
    static int lastWdt = clientWdt;
    
    int wdc = clientWdt*0.15f;
    if( column == 1 )
        wdc = clientWdt*0.25f;
    else if( column == 5 )
        wdc = lastWdt-1;

    tbl->setColumnWidth(column, wdc);
    lastWdt -= wdc;
}

void LMXModel::setRowHeight(int row) const
{
    if( NULL == view_)
        return;

    QTableView* tbl = qobject_cast<QTableView*>(view_);
    if( NULL == tbl ) {
        ::MessageBoxA(NULL, "Error object type <QTableView>", "Error", MB_OK|MB_ICONSTOP);
        return;
    }
    tbl->setRowHeight( row, tbl->height() / 20);
}

void LMXModel::onResponseReceived(const QString& symbol)
{
    Q_UNUSED(symbol);
    CDebug() << "LMXModel::onResponseReceived: \"" + symbol + "\"";

    // a new market interment response on interactive MarketData reqiest
    // should be updated
    QMutexLocker g(&monitorLock_);
    int idx = monitored_.indexOf(symbol);
    if(-1 == idx) 
    {
        int row = 0, lcode = ini_.originCodeBySymbol(symbol).toInt();
        for(; row < monitored_.count(); row++) {
            int rcode = ini_.originCodeBySymbol(monitored_.at(row)).toInt();
            if( lcode < rcode )
                break;
        }

        beginInsertRows(QModelIndex(), row, row);
        monitored_.insert(row,symbol);
        endInsertRows();

        QModelIndex rowIndex = index(row, 0);
        view_->scrollTo(rowIndex,QAbstractItemView::PositionAtCenter);
    }
    else
        setRowHeight(idx);
    view_->doItemsLayout();
}
