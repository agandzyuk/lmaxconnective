#include "instrumentbox.h"
#include "ini.h"
#include "defedit.h"
#include "setupdlg.h"

#include <QAbstractItemModel>
#include <QtWidgets>

///////////////////////////////////////////////////////////////////////////////////
InstrumentBox::InstrumentBox(const QString& symbol, 
                  const QString& code, 
                  const QIni& ini,
                  QWidget* parent)
    : QDialog(parent),
    addBox_( symbol.isEmpty() )
{
    QVBoxLayout* vlayout  = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();

    QLabel* lbl;
    hlayout->addWidget(lbl = new QLabel(tr("Symbol name"),this));
    lbl->setAlignment(Qt::AlignLeft);
    hlayout->addWidget(lbl = new QLabel(tr("Code"),this));
    lbl->setAlignment(Qt::AlignLeft);
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    if( addBox_ ) {
        symbolEdit_ = new QNotifyEdit(ini, NULL, 
            tr("Type a new symbol to add into INI common settings"), 
            QNotifyEdit::SymbolAddID, this);
        codeEdit_  = new QNotifyEdit(ini, NULL, 
            tr("Type an instrument code to add into INI common settings"), 
            QNotifyEdit::CodeAddID, this);
    }
    else {
        symbolEdit_ = new QNotifyEdit(ini, symbol, 
            tr("Change the symbol of selected instrument in INI common settings"), 
            QNotifyEdit::SymbolEditID, this);
        codeEdit_  = new QNotifyEdit(ini, code, 
            tr("Change a code of selected instrument in INI common settings"), 
            QNotifyEdit::CodeEditID, this);
    }
    hlayout->addWidget(symbolEdit_);
    hlayout->addWidget(codeEdit_);
    vlayout->addLayout(hlayout);

    QObject::connect(symbolEdit_, SIGNAL(notifyContentChanged()), 
                     this, SLOT(onContentChanged()));
    QObject::connect(codeEdit_, SIGNAL(notifyContentChanged()),
                     this, SLOT(onContentChanged()));

    hlayout = new QHBoxLayout();
    btnApply_ = new QPushButton(addBox_ ? tr("Add"):tr("Apply"), this);
    QObject::connect(btnApply_, SIGNAL(clicked()), this, SLOT(onApply()));
    btnApply_->setEnabled(false);
    btnApply_->setAutoDefault(false);
    hlayout->addWidget(btnApply_, 2, Qt::AlignRight);

    QPushButton* btn = new QPushButton(tr("Close"), this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(close()));
    btn->setAutoDefault(false);
    hlayout->addWidget(btn, 0, Qt::AlignRight);

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    setWindowTitle((addBox_ ? tr("Add "):tr("Edit ")) + tr("Instruments"));
}

void InstrumentBox::onApply()
{
    btnApply_->setEnabled(false);

    QListWidget *listBox, *viewBox;
    QListWidgetItem* item;
    SetupDlg* dialog = qobject_cast<SetupDlg*>(parentWidget());
    if( (NULL == dialog) || 
        (NULL == (listBox = dialog->listOrigin_)) || 
        (NULL == (viewBox = dialog->listView_)) )
    {
        return;
    }

    QString oldsymbol = symbolEdit_->defaultText();
    QString newsymbol = symbolEdit_->text();
    QString oldcode   = codeEdit_->defaultText();
    QString newcode   = codeEdit_->text();

    // update INI (both for adding and change)
    dialog->ini_.originAddOrEdit(newsymbol, newcode);

    // searching in INI list by
    SelectedT items;
    if( addBox_ ) {
        listBox->addItem(newsymbol);
        items = listBox->findItems(newsymbol, Qt::MatchExactly);
        if( items.empty() || NULL == (item = *items.begin()))
            return;
    }
    else 
    {
        if( symbolEdit_->updateAllowed() ) 
        {
            // update viewing list on first
            items = viewBox->findItems(oldsymbol, Qt::MatchExactly);
            if( items.empty() || NULL == (item = *items.begin()))
                return;
            item->setText(newsymbol);
            viewBox->sortItems();
        }
        else if( !codeEdit_->updateAllowed() ) // invalid precodition
            return;

        // searching in INI list
        items = listBox->findItems(oldsymbol, Qt::MatchExactly);
        if( items.empty() || NULL == (item = *items.begin()))
            return;

        // update if needed
        if( symbolEdit_->updateAllowed() )
            item->setText(newsymbol);
    }

    // in any case: sort INI list and scrolls to modified instrument position
    listBox->sortItems();
    int row = listBox->row(item);
    if( row > -1) {
        QModelIndex rowindex = listBox->model()->index(row,0);
        listBox->scrollTo(rowindex,QAbstractItemView::PositionAtTop);
        listBox->update(rowindex);
    }

    // in any case update buttons
    symbolEdit_->setDefaultText( addBox_ ? "" : newsymbol );
    codeEdit_->setDefaultText( addBox_ ? "" : newcode );
}

void InstrumentBox::onContentChanged()
{
    if( addBox_ )
        btnApply_->setEnabled( symbolEdit_->updateAllowed() && codeEdit_->updateAllowed() );
    else
        btnApply_->setEnabled( symbolEdit_->updateAllowed() || codeEdit_->updateAllowed() );
}

///////////////////////////////////////////////////////////////////////////////////
QNotifyEdit::QNotifyEdit(const QIni& ini,
                         const QString& defText, 
                         const QString& helpText,
                         NotifyControl ctrlID,
                         QWidget* parent)
    : QDefEdit(parent, defText, helpText),
    ini_(ini),
    ctrlID_(ctrlID),
    updateAllowed_(false)
{}

void QNotifyEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);
    update();
}

bool QNotifyEdit::updateAllowed() const
{
    return updateAllowed_;
}

void QNotifyEdit::update()
{
    QDefEdit::update();

    bool oldState = updateAllowed_;

    QString info = text();
    if( info.isEmpty() )
    {
        updateAllowed_ = false;
    }
    else if( ctrlID_ == SymbolAddID || ctrlID_ == SymbolEditID )
    {
        updateAllowed_ = (true == ini_.originCodeBySymbol(info).isEmpty());
        if( ctrlID_ == SymbolEditID )
            updateAllowed_ &= isChanged();
    }
    else if( ctrlID_ == CodeAddID || ctrlID_ == CodeEditID )
    {
        updateAllowed_ = (true == ini_.originSymbolByCode(info).isEmpty());
        if( ctrlID_ == CodeEditID )
            updateAllowed_ &= isChanged();
    }

    if(updateAllowed_ != oldState)
        emit notifyContentChanged();
}
