#include "globals.h"
#include "logger.h"
#include "quotestableview.h"
#include "quotestablemodel.h"
#include "symboleditdialog.h"
#include "fixmessagedialog.h"

#include <QHeaderView>
#include <QtWidgets>

//////////////////////////////////////////////////////////////
QuotesTableView::QuotesTableView(QWidget* parent)
    : QTableView(parent)
{
    verticalHeader()->setVisible(false);

    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAttribute(Qt::WA_TranslucentBackground);
}

void QuotesTableView::updateStyles()
{
    QTableView::updateGeometries();

    QHeaderView* hdr = horizontalHeader();
    if( !hdr ) return;

    hdr->setFont( *Global::native );
    hdr->setSectionsClickable(true);
    hdr->setStretchLastSection(true);
    hdr->setSortIndicator(0, Qt::AscendingOrder);
    hdr->setSortIndicatorShown(true);
    QObject::connect(hdr, SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), 
                          this, SLOT(onSortIndicatorChanged(int, Qt::SortOrder)) );
}

void QuotesTableView::setModel(QAbstractItemModel* dataModel)
{
    sortingModel_.setSourceModel(dataModel);
    QTableView::setModel(&sortingModel_);
}

bool QuotesTableView::event(QEvent* e)
{
    if( e->type() == QEvent::ToolTip ) {
        onToolTip(dynamic_cast<QHelpEvent*>(e));
    }

    /*if( e->type() != QEvent::Paint && e->type() != QEvent::Timer && 
        e->type() != QEvent::MouseMove && e->type() != QEvent::Enter && 
        e->type() != QEvent::Leave )
    {
        qDebug() << e->type();
    }
    */
    return QTableView::event(e);
}

void QuotesTableView::onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    sortingModel_.sort(logicalIndex, order);
}

void QuotesTableView::onToolTip(QHelpEvent* e)
{
    if( !e ) return;

    // QT BUG: substract the header height (header row taken as row=0, 
    // so last row in the table will be mapped from point with row=-1)
    QPoint pt = mapFromGlobal(e->globalPos());
    pt.ry() -= height()/20;

    QModelIndex index = indexAt(pt);
    if( index.data().isNull() ) {
        QToolTip::hideText();
        return;
    }

    QString tipText;
    if( index.column() == 4 )
    {
        QToolTip::showText(e->globalPos(), 
            QString("The last request processed %1 msecs").arg(index.data().toInt()), this);
    }
    else if( index.column() == 5 )
    {
        QToolTip::showText(e->globalPos(), index.data().toString(), this);
    }
}

void QuotesTableView::contextMenuEvent(QContextMenuEvent* e)
{
    QModelIndex sourceIdx = sortingModel_.mapToSource(indexAt(e->pos()));

    if(sourceIdx.column() < 0 || sourceIdx.column() > 5)
        return;

    bool unsubscribed = false;
    bool rejected = false;
    {
        QSharedPointer<QReadLocker> autolock;
        Snapshot* snap = model()->getSnapshot(model()->getByOrderRow(sourceIdx.row()).first.c_str(),autolock);
        if(snap) {
            unsubscribed = (snap->statuscode_ & Snapshot::StatUnSubscribed);
            rejected = (snap->statuscode_ & (Snapshot::StatBusinessReject|Snapshot::StatSessionReject));
        }
    }

    QMenu* ctxMenu = new QMenu(this);
    ctxMenu->addAction(QIcon(*Global::pxViewFIX), tr("FIX Incoming"), this, SLOT(onFIXIncoming()));
    ctxMenu->addAction(QIcon(*Global::pxEditFIX), tr("FIX Outgoing"), this, SLOT(onFIXOutgoing()));

    ctxMenu->addSeparator();
    if(sourceIdx.column() == 0)
        ctxMenu->addAction(QIcon(*Global::pxEditOrigin), tr("Change Code"), this, SLOT(onEditCode()));
    else if(sourceIdx.column() == 1)
        ctxMenu->addAction(QIcon(*Global::pxEditOrigin), tr("Change Symbol"), this, SLOT(onEditSymbol()));

    if( !unsubscribed )
        ctxMenu->addAction(QIcon(*Global::pxUnSubscribe), tr("UnSubscribe"), this, SLOT(onSendUnSubscribe()));
    else if( unsubscribed )
        ctxMenu->addAction(QIcon(*Global::pxSubscribe), tr("Subscribe"), this, SLOT(onSendSubscribe()));

    ctxMenu->addAction(QIcon(*Global::pxRemoveRow), tr("Remove Instrument"), 
                       this, SLOT(onRemoveRow()))->setEnabled(unsubscribed||rejected);

    ctxMenu->popup(mapToGlobal(e->pos()));
}

void QuotesTableView::onEditCode()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    QModelIndex sourceIdx = sortingModel_.mapToSource( *idxs.begin() );
    qint32 code = sourceIdx.data().toInt();
    if( code == 0 )
        return;

    SymbolEditDialog("", sourceIdx.data().toString(), *model(), this).exec();
}

void QuotesTableView::onEditSymbol()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    QModelIndex sourceIdx = sortingModel_.mapToSource( *idxs.begin() );
    QString symbol = sourceIdx.data().toString();
    if( symbol.isEmpty() )
        return;

    SymbolEditDialog(symbol, "", *model(), this).exec();
}

void QuotesTableView::onRemoveRow()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    qint16 row = sortingModel_.mapToSource( *idxs.begin() ).row();
    if( row < 0)
        return;

    Instrument inst = model()->getByOrderRow(row);
    if( inst.second != -1 ) 
        model()->setMonitoring(inst, false, true);
}

void QuotesTableView::onSendRequest(bool subscribe)
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    qint16 row = sortingModel_.mapToSource( *idxs.begin() ).row();
    if( row < 0)
        return;

    Instrument inst = model()->getByOrderRow(row);
    if( inst.second != -1 )
    {
        bool sendSubscription = false;
        QSharedPointer<QReadLocker> autolock;
        Snapshot* snap = model()->getSnapshot(model()->getByOrderRow(row).first.c_str(),autolock);
        if(snap) 
            sendSubscription = (snap->statuscode_ & Snapshot::StatUnSubscribed);
        autolock.reset();

        if( sendSubscription && subscribe)
            emit model()->activateRequest(inst);
        else if( !(sendSubscription || subscribe) )
            emit model()->activateRequest( Instrument("Disable_" + inst.first, inst.second) );
        else
        { /*do nothing */}
    }
}

void QuotesTableView::onFIXOutgoing()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    qint16 row = sortingModel_.mapToSource( *idxs.begin() ).row();
    if( row < 0)
        return;

    Instrument inst = model()->getByOrderRow(row);
    if( inst.second != -1 )
        FixMessageDialog(inst.first.c_str(), inst.second, false, model()).exec();
}

void QuotesTableView::onFIXIncoming()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;


    qint16 row = sortingModel_.mapToSource( *idxs.begin() ).row();
    if( row < 0)
        return;

    Instrument inst = model()->getByOrderRow(row);
    if( inst.second != -1 )
        FixMessageDialog(inst.first.c_str(), inst.second, true, model()).exec();
}

qint16 QuotesTableView::toSourceRow(qint16 currentRow)
{
    return sortingModel_.mapToSource( sortingModel_.index(currentRow,0) ).row();
}

qint16 QuotesTableView::fromSourceRow(qint16 sourceRow)
{
    return sortingModel_.mapFromSource(model()->index(sourceRow,0) ).row();
}

QuotesTableModel* QuotesTableView::model() const
{
    return qobject_cast<QuotesTableModel*>(sortingModel_.sourceModel());
}
