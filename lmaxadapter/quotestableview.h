#ifndef __quotestableview_h__
#define __quotestableview_h__

#include <QTableView>
#include <QSortFilterProxyModel>

////////////////////////////////////////////////////////////////
class QuotesTableModel;

////////////////////////////////////////////////////////////////
class QuotesTableView : public QTableView
{
    Q_OBJECT

public:
    QuotesTableView(QWidget *parent = 0);
    void updateStyles();
    void setModel(QAbstractItemModel* dataModel);
    qint16 toSourceRow(qint16 currentRow);
    qint16 fromSourceRow(qint16 sourceRow);
    QuotesTableModel* model() const;

protected:
    bool event(QEvent* e);
    void contextMenuEvent(QContextMenuEvent* e);
    void onToolTip(QHelpEvent* e);

protected slots:
    void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);
    void onSendSubscribe() { onSendRequest(true); }
    void onSendUnSubscribe() { onSendRequest(false); }
    void onEditCode();
    void onEditSymbol();
    void onRemoveRow();
    void onFIXIncoming();
    void onFIXOutgoing();

private:
    void onSendRequest(bool subscribe);

    QSortFilterProxyModel sortingModel_;
};

#endif // __quotestableview_h__
