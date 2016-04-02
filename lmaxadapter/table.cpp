#include "defines.h"
#include "table.h"

#include <QtWidgets>

//////////////////////////////////////////////////////////////
LMXTableHdr::LMXTableHdr(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
{
    setFont( *Global::native );
    setSectionsClickable(true);
}

void LMXTableHdr::setResizes()
{
    setSortIndicator(0, Qt::AscendingOrder);
    setSortIndicatorShown(true);

    setStretchLastSection(true);
    setCascadingSectionResizes(true);
    updateGeometries();
}

//////////////////////////////////////////////////////////////
LMXTable::LMXTable(QWidget* parent)
    : QTableView(parent)
{
    setHorizontalHeader( new LMXTableHdr(this) );
    verticalHeader()->setVisible(false);

    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAttribute(Qt::WA_TranslucentBackground);
}

void LMXTable::updateStyles()
{
    QTableView::updateGeometries();

    LMXTableHdr* hdr = qobject_cast<LMXTableHdr*>(horizontalHeader());
    if( hdr )
        hdr->setResizes();
}

bool LMXTable::event(QEvent* e)
{
    if( e->type() != QEvent::Paint && e->type() != QEvent::Timer && 
        e->type() != QEvent::MouseMove && e->type() != QEvent::Enter && 
        e->type() != QEvent::Leave )
    {
        //qDebug() << e->type();
    }
    return QTableView::event(e);
}

void LMXTable::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu* ctxMenu = new QMenu(this);
    int c = QTableView::indexAt(e->pos()).column();

    if(c < 0 && c > 5)
        return;

    ctxMenu->addAction(/*QIcon(*Global::pxToRefresh), */tr("Resend Request"), this, SLOT(onRequestUpdate()));

    if(c == 0)
        ctxMenu->addAction(/*QIcon(*Global::pxEditOrigin), */tr("Change Code"), this, SLOT(onEditCode()));
    else if(c == 1)
        ctxMenu->addAction(/*QIcon(*Global::pxEditOrigin), */tr("Change Symbol"), this, SLOT(onEditSymbol()));

    ctxMenu->addAction(/*QIcon(*Global::pxRemoveOrigin), */tr("Remove Request"), this, SLOT(onRemoveRow()));

    ctxMenu->addSeparator();
    ctxMenu->addAction(/*QIcon(*Global::pxViewFIX), */tr("View FIX Snapshot"), this, SLOT(onViewSnapshotFIX()));
    ctxMenu->addAction(/*QIcon(*Global::pxEditFIX), */tr("Edit FIX Request"), this, SLOT(onEditRequestFIX()));

    ctxMenu->popup(mapToGlobal(e->pos()));
}

void LMXTable::onEditCode()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    int code = idxs.begin()->data().toInt();
    if( code == 0 )
        return;
}

void LMXTable::onEditSymbol()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    QString symbol = idxs.begin()->data().toString();
    if( symbol.isEmpty() )
        return;
}

void LMXTable::onRemoveRow()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    int row = idxs.begin()->row();
    if( row < 0)
        return;
}

void LMXTable::onRequestUpdate()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    int row = idxs.begin()->row();
    if( row < 0)
        return;
 
}

void LMXTable::onEditRequestFIX()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    int row = idxs.begin()->row();
    if( row < 0)
        return;
 
}

void LMXTable::onViewSnapshotFIX()
{
    QModelIndexList idxs = selectedIndexes();
    if( idxs.empty() ) 
        return;

    int row = idxs.begin()->row();
    if( row < 0)
        return;
 
}
