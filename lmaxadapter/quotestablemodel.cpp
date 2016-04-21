#include "globals.h"
#include "quotestablemodel.h"
#include "quotestableview.h"
#include "mqlproxyserver.h"

#include <QtWidgets>

static qint16 rows_before_view = 0;

//////////////////////////////////////////////////////////////
QuotesTableModel::QuotesTableModel(QSharedPointer<MqlProxyServer>& mqlProxy, QWidget* parent)
    : FixDataModel(mqlProxy, parent)
{
    QObject::connect(this, SIGNAL(notifyInstrumentAdd(Instrument)), this, SLOT(onInstrumentAdd(Instrument)), Qt::DirectConnection);
    QObject::connect(this, SIGNAL(notifyInstrumentRemove(Instrument,qint16)), this, SLOT(onInstrumentRemove(Instrument,qint16)), Qt::DirectConnection);
    QObject::connect(this, SIGNAL(notifyInstrumentChange(Instrument,Instrument)), this, SLOT(onInstrumentChange(Instrument,Instrument)), Qt::DirectConnection);
}

QuotesTableModel::~QuotesTableModel()
{}

void QuotesTableModel::resetView(QuotesTableView* view)
{
    QMutexLocker g(&monitorLock_);
    view_ = view;
    monitored_.clear();
    rows_before_view = monitoredCount();
}

int QuotesTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return monitoredCount();
}

int QuotesTableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant QuotesTableModel::data(const QModelIndex& index, int role) const
{
    if( role != Qt::DisplayRole )
        return QVariant();

    qint16 r = index.row(), c = index.column();
    if( c == 0 ) {
        QMutexLocker g(&monitorLock_);
        if( monitored_.count() < rows_before_view ) {
            Instrument ri = getByOrderRow(r);
            monitored_.insert(ri.second, ri.first.c_str());
        }
        else if( monitored_.count() > rows_before_view )
            rows_before_view = monitored_.count();
        g.unlock();
        setRowHeight(r);
    }

    if( c == 0 ) {
        return getByOrderRow(r).second;
    }
    else if( c == 1 )
        return QString::fromStdString(getByOrderRow(r).first);

    QSharedPointer<QReadLocker> autolock;
    std::string sym = QuotesTableModel::index(r,1).data().toString().toStdString();
    const Snapshot* snapshot = getSnapshot(sym.c_str(), autolock);
    if( snapshot == NULL )
        return QVariant();

    switch( c ) 
    {
    case 2:
        return snapshot->ask_;
    case 3:
        return snapshot->bid_;
    case 4:
        if( snapshot->statuscode_ != Snapshot::StatSubscribe )
            return snapshot->requestTime_;
        break;
    case 5:
        return snapshot->description_;
    }
    return QVariant();
}

QVariant QuotesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
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
        return hdrName;
    }
    else if( role == Qt::SizeHintRole )
    {
        QSize sz = columnProportionWidth(section);
        if(sz.isEmpty())
            return QVariant();
        return sz;
    }
    return QVariant();
}

QSize QuotesTableModel::columnProportionWidth(int column) const
{
    static qint8 columnsSetup = 0;

    if( NULL == view_ || columnsSetup > 5 )
        return QSize();

    int clientWdt = view_->width()-2;
    static int lastWdt = clientWdt;

    int wdc = 0;
    if( columnsSetup < 6 ) {
        if( column == 1 )
            wdc = clientWdt*0.23f;
        else if( column != 5 )
            wdc = clientWdt*0.14f;
        else if( column == 5 )
            wdc = lastWdt-2;
        lastWdt -= wdc;
        columnsSetup++;
    }
    else {
        if( column == 5 ) {
            wdc = clientWdt-2;
            for(int i=0; i<5; ++i)
                wdc -= view_->columnWidth(i);
        }
    }
    return QSize(wdc, view_->height()/20);
}

void QuotesTableModel::setRowHeight(int row) const
{
    if( NULL == view_)
        return;

    view_->setRowHeight(row, view_->height()/20);
}

void QuotesTableModel::onInstrumentAdd(const Instrument& inst)
{
    QString qSym = inst.first.c_str();
    CDebug() << "QuotesTableModel::onInstrumentAdd: \"" + qSym + "\"";

    // a new market interment response on interactive MarketData reqiest
    // should be updated
    QMutexLocker g(&monitorLock_);
    int row = -1;
    Code2SymT::iterator It = monitored_.begin();
    for(; It != monitored_.end(); ++It) {
        row++;
        if( It.key() == inst.second )
            break;
    }

    if(It == monitored_.end()) 
    {
        if( row == -1) 
            row = getOrderRow(qSym);

        QModelIndex rowIndex = index(row, 0);
        monitored_.insert(inst.second, qSym);
        g.unlock();

        beginInsertRows(QModelIndex(), row, row);
        endInsertRows();
        view_->scrollTo(rowIndex,QAbstractItemView::PositionAtCenter);
    }

    setRowHeight(row);
    view_->doItemsLayout();
}

void QuotesTableModel::onInstrumentRemove(const Instrument& inst, qint16 orderRow)
{
    QString qSym = inst.first.c_str();
    CDebug() << "QuotesTableModel::onInstrumentRemove: \"" + qSym + "\"";

    removeCached( inst.second );

    QModelIndex prevIndex;
    if( orderRow == -1 ) {
        prevIndex = index(0,1);
        for(orderRow = 0; prevIndex.isValid(); ++orderRow) {
            if( prevIndex.data().toString() == inst.first.c_str() )
                break;
            prevIndex = index(orderRow+1,1);
        }
        if( !prevIndex.isValid() ) {
            CDebug() << "Row of instrument not found to remove \"" + qSym + "\"";
        }
    }

    {
        QMutexLocker g(&monitorLock_);
        Code2SymT::iterator It = monitored_.find(inst.second);
        if(It != monitored_.end())
            monitored_.erase(It);
        prevIndex = index(orderRow > 0 ? orderRow-1 : 0, 0);
    }

    beginRemoveRows(QModelIndex(), orderRow, orderRow);
    endRemoveRows();

    view_->clearSelection();
    view_->setCurrentIndex(prevIndex);
    view_->scrollTo(prevIndex, orderRow > 0 ? QAbstractItemView::PositionAtCenter : QAbstractItemView::PositionAtTop);
    view_->doItemsLayout();
}

void QuotesTableModel::onInstrumentChange(const Instrument& old, const Instrument& cur)
{
    QString oldSym  = old.first.c_str();
    QString newSym  = cur.first.c_str();
    qint32  oldCode = old.second;
    qint32 newCode  = cur.second;

    QString nameOld = "\"" + oldSym + ":" + QString("%1").arg(oldCode) + "\"";
    QString nameNew = "\"" + newSym + ":" + QString("%1").arg(newCode) + "\"";
    CDebug() << "QuotesTableModel::onInstrumentChange: " << nameOld << " --> " << nameNew;

    if(oldCode != newCode)
        setMonitoring(old, false, true);
    else if( oldSym != newSym )
        setMonitoring(cur, false, true);
    else
        return;

    setMonitoring(cur, true, true);
}

void QuotesTableModel::onInstrumentUpdate()
{
//    CDebug() << "onInstrumentUpdate layout all";
    view_->doItemsLayout();
}

void QuotesTableModel::onInstrumentUpdate(const Instrument& inst)
{
    view_->doItemsLayout();
/*
    int row = getOrderRow(inst.first.c_str());
    if( row != -1 ) {
        view_->update(QuotesTableModel::index(row,2));
        view_->update(QuotesTableModel::index(row,3));
        view_->update(QuotesTableModel::index(row,4));
        view_->update(QuotesTableModel::index(row,5));
    }
    CDebug() << "\"" << inst.first.c_str() << "\":" << inst.second
             << " - row " << row << " updated";
    */
}

QAbstractItemView* QuotesTableModel::view() const
{
    return view_;
}
