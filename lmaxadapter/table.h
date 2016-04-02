#ifndef __lmxtable_h__
#define __lmxtable_h__

#include <QHeaderView>
#include <QTableView>

class LMXTableHdr : public QHeaderView
{
    Q_OBJECT
public:
    LMXTableHdr(QWidget* parent);

    void setResizes();
};

class LMXTable : public QTableView
{
    Q_OBJECT

public:
    LMXTable(QWidget *parent = 0);
    void updateStyles();

protected:
    bool event(QEvent* e);
    void contextMenuEvent(QContextMenuEvent* e);

protected slots:
    void onRequestUpdate();
    void onEditCode();
    void onEditSymbol();
    void onRemoveRow();
    void onEditRequestFIX();
    void onViewSnapshotFIX();
};

#endif // __lmxtable_h__
