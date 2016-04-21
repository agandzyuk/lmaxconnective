#include "globals.h"
#include "symboleditdialog.h"
#include "defaultedit.h"
#include "setupdialog.h"
#include "marketabstractmodel.h"

#include <QAbstractItemModel>
#include <QtWidgets>

///////////////////////////////////////////////////////////////////////////////////
SymbolEditDialog::SymbolEditDialog(const QString& symbol, 
                                   const QString& code, 
                                   MarketAbstractModel& model,
                                   QWidget* parent)
    : QDialog(parent),
    addBox_( symbol.isEmpty() && code.isEmpty() ),
    symbolEdit_(NULL),
    codeEdit_(NULL)
{
    QVBoxLayout* vlayout  = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();

    QLabel* lbl;
    if( addBox_ || (!addBox_ && !symbol.isEmpty()) ) {
        hlayout->addWidget(lbl = new QLabel(tr("Symbol name"),this));
        lbl->setAlignment(Qt::AlignLeft);
    }
    if( addBox_ || (!addBox_ && !code.isEmpty()) ) {
        hlayout->addWidget(lbl = new QLabel(tr("Code"),this));
        lbl->setAlignment(Qt::AlignLeft);
    }
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    if( addBox_ ) {
        symbolEdit_ = new SymbolEdit(NULL, 
            tr("Type a new symbol to add into INI common settings"), 
            SymbolEdit::SymbolAddID, model,this);
        codeEdit_  = new SymbolEdit(NULL, 
            tr("Type an instrument code to add into INI common settings"), 
            SymbolEdit::CodeAddID, model, this);
    }
    else {
        if( !symbol.isEmpty() ) {
            symbolEdit_ = new SymbolEdit(symbol, 
                tr("Change the symbol of selected instrument in INI common settings"), 
                SymbolEdit::SymbolEditID, model, this);
        }
        if( !code.isEmpty() ) {
        codeEdit_  = new SymbolEdit(code, 
            tr("Change a code of selected instrument in INI common settings"), 
            SymbolEdit::CodeEditID, model, this);
        }
    }

    if( symbolEdit_ ) {
        hlayout->addWidget(symbolEdit_);
        QObject::connect(symbolEdit_, SIGNAL(notifyContentChanged()), 
                        this, SLOT(onContentChanged()));
    }
    if( codeEdit_ ) {
        hlayout->addWidget(codeEdit_);
        QObject::connect(codeEdit_, SIGNAL(notifyContentChanged()),
                        this, SLOT(onContentChanged()));
    }
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    if( codeEdit_ && symbolEdit_ ) {
        btnApply_ = new QPushButton(addBox_ ? tr("Add"):tr("Apply"), this);
        QObject::connect(btnApply_, SIGNAL(clicked()), this, SLOT(onApply()));
    }
    else {
        btnApply_ = new QPushButton(tr("OK"), this);
        QObject::connect(btnApply_, SIGNAL(clicked()), this, SLOT(onOK()));
    }

    btnApply_->setEnabled(false);
    btnApply_->setAutoDefault(false);
    hlayout->addWidget(btnApply_, 2, Qt::AlignRight);

    if( codeEdit_ && symbolEdit_ ) {
        QPushButton* btn = new QPushButton(tr("Close"), this);
        QObject::connect(btn, SIGNAL(clicked()), this, SLOT(close()));
        btn->setAutoDefault(false);
        hlayout->addWidget(btn, 0, Qt::AlignRight);
    }

    vlayout->addLayout(hlayout);
    setLayout(vlayout);

    if( codeEdit_ && symbolEdit_ )
        setWindowTitle((addBox_ ? tr("Add "):tr("Edit ")) + tr("Instruments"));
    else if( codeEdit_ )
        setWindowTitle(tr("Change instrument code"));
    else if( symbolEdit_ )
        setWindowTitle(tr("Change instrument symbol"));
}

void SymbolEditDialog::onOK()
{
    if( symbolEdit_ ) {
        QString oldsymbol = symbolEdit_->defaultText();
        std::string newsymbol = symbolEdit_->text().toStdString();
        qint32 nCode = symbolEdit_->model_.getCode(oldsymbol);
        if( nCode != -1 )
            symbolEdit_->model_.instrumentCommit(Instrument(newsymbol, nCode), false);
    }
    else if( codeEdit_ ) {
        qint32 oldcode = codeEdit_->defaultText().toInt();
        qint32 nCode   = codeEdit_->text().toInt();
        if( oldcode == 0 || nCode == 0 ) 
            return;
        const char* newsymbol = codeEdit_->model_.getSymbol(oldcode);
        if( newsymbol != NULL )
            codeEdit_->model_.instrumentCommit(Instrument(newsymbol, nCode), false);
    }
    accept();
    close();
}

void SymbolEditDialog::onApply()
{
    btnApply_->setEnabled(false);

    QListWidget *listBox, *viewBox;
    QListWidgetItem* item;
    SetupDialog* dialog = qobject_cast<SetupDialog*>(parentWidget());
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
    bool addAsEdit = false;
    if(oldsymbol != newsymbol && oldcode != newcode)
        addAsEdit = true;

    qint32 nCode = newcode.toInt();
    if( nCode == 0 ) return;

    // update table view in case of changing instrument has been moved between list without "Apply"
    if( dialog->isSymbolViewModified() )
        dialog->onApply();
    // update INI (both for adding and change)
    symbolEdit_->model_.instrumentCommit(Instrument(newsymbol.toStdString(), nCode), true);

    // searching in INI list by
    SelectedT items;
    if( addBox_ || addAsEdit ) {

        // adding to viewing list, set hidden then sort
        item = new QListWidgetItem(QIcon(dialog->getInstrumentPixmap(newsymbol,true)),newsymbol,viewBox);
        item->setToolTip(dialog->getInstrumentToolTip(newsymbol,true,true));
        viewBox->addItem(item);
        items = viewBox->findItems(newsymbol, Qt::MatchExactly);
        if( items.empty() || NULL == (item = *items.begin()))
            return;
        item->setHidden(true);
        viewBox->sortItems();

        // adding to to INI list, receive pointer to item and continue
        item = new QListWidgetItem(QIcon(dialog->getInstrumentPixmap(newsymbol,false)), newsymbol, listBox);
        item->setToolTip(dialog->getInstrumentToolTip(newsymbol, false, false));
        listBox->addItem(item);
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
            item->setIcon(QIcon(dialog->getInstrumentPixmap(oldsymbol,true)));
            item->setToolTip(dialog->getInstrumentToolTip(oldsymbol,true,true));
            viewBox->sortItems();
        }
        else if( !codeEdit_->updateAllowed() ) // invalid precodition
            return;

        // searching in INI list
        items = listBox->findItems(oldsymbol, Qt::MatchExactly);
        if( items.empty() || NULL == (item = *items.begin()))
            return;

        // update if needed
        if( symbolEdit_->updateAllowed() ) {
            item->setIcon(QIcon(dialog->getInstrumentPixmap(oldsymbol,false)));
            item->setToolTip(dialog->getInstrumentToolTip(oldsymbol,false,false));
            item->setText(newsymbol);
        }
    }

    // in any case: sort INI list and scrolls to modified instrument position
    listBox->sortItems();
    int row = listBox->row(item);
    if( row > -1) {
        QModelIndex rowindex = listBox->model()->index(row,0);
        listBox->scrollTo(rowindex,QAbstractItemView::EnsureVisible/*PositionAtTop*/);
        listBox->update(rowindex);
    }

    // in any case update buttons
    symbolEdit_->setDefaultText( addBox_ ? "" : newsymbol );
    codeEdit_->setDefaultText( addBox_ ? "" : newcode );
}

void SymbolEditDialog::onContentChanged()
{
    if( addBox_ )
        btnApply_->setEnabled( symbolEdit_->updateAllowed() && codeEdit_->updateAllowed() );
    else if( symbolEdit_ && codeEdit_ )
        btnApply_->setEnabled( symbolEdit_->updateAllowed() || codeEdit_->updateAllowed() );
    else if( symbolEdit_ )
        btnApply_->setEnabled( symbolEdit_->updateAllowed() );
    else if( codeEdit_ )
        btnApply_->setEnabled( codeEdit_ ->updateAllowed() );
}

///////////////////////////////////////////////////////////////////////////////////
SymbolEdit::SymbolEdit(const QString& defText, 
                       const QString& helpText,
                       NotifyControl ctrlID,
                       MarketAbstractModel& model,
                       QWidget* parent)
    : DefaultEdit(parent, defText, helpText),
    ctrlID_(ctrlID),
    updateAllowed_(false),
    model_(model)
{}

void SymbolEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);
    update();
}

bool SymbolEdit::updateAllowed() const
{
    return updateAllowed_;
}

void SymbolEdit::update()
{
    DefaultEdit::update();

    bool oldState = updateAllowed_;

    QString info = text();
    if( info.isEmpty() )
    {
        updateAllowed_ = false;
    }
    else if( ctrlID_ == SymbolAddID || ctrlID_ == SymbolEditID )
    {
        updateAllowed_ = (-1 == model_.getCode(info));
        if( ctrlID_ == SymbolEditID )
            updateAllowed_ &= isChanged();
    }
    else if( ctrlID_ == CodeAddID || ctrlID_ == CodeEditID )
    {
        qint32 code = info.toInt();
        updateAllowed_ = (code && (NULL == model_.getSymbol(code)));
        if( ctrlID_ == CodeEditID )
            updateAllowed_ &= isChanged();
    }

    if(updateAllowed_ != oldState)
        emit notifyContentChanged();
}
