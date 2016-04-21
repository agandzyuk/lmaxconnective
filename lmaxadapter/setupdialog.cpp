#include "globals.h"

#include "setupdialog.h"
#include "symboleditdialog.h"
#include "symbolsmodel.h"

#include <QAbstractItemModel>
#include <QtWidgets>
#include <string>

using namespace std;

/////////////////////////////////////////////////////////////////////////
bool func_less(QListWidgetItem* left, QListWidgetItem* right)
{
    return *left < *right;
}

/////////////////////////////////////////////////////////////////////////
SetupDialog::SetupDialog(SymbolsModel& model, QWidget* parent) 
    : QDialog(parent),
    model_(model),
    symbolViewModified_(false)
{
    setFixedSize(parent->width()*0.75f, parent->height()*0.75f);
    
    QPoint pt = parent->pos();
    int w = parent->width();
    int h = parent->height();
    
    int movePosX = pt.x() + w*0.3f;
    if( movePosX + width() >= Global::desktop.width() )
        movePosX = pt.x() - w*0.3f;

    int movePosY = pt.y() + h*0.3f;
    if( movePosY + height() >= Global::desktop.height() )
        movePosY = pt.y() - h*0.3f;

    move(movePosX, movePosY);

    setWindowTitle(tr("Settings"));
}

void SetupDialog::resetInfo()
{
    QVector<const char*> origin;
    QVector<string> monitored;
    initialMonitoringSet_.clear();

    model_.getSymbolsSupported(origin);
    model_.getSymbolsUnderMonitoring(monitored);

    for(QVector<const char*>::const_iterator It = origin.begin();
        It != origin.end(); ++It)
    {
        QListWidgetItem* itemL = new QListWidgetItem(QIcon(getInstrumentPixmap(*It,false)), *It,listOrigin_);
        itemL->setToolTip(getInstrumentToolTip(*It,false,false));
        listOrigin_->addItem( itemL );
        QListWidgetItem* itemR = new QListWidgetItem(QIcon(getInstrumentPixmap(*It,false)), *It,listView_);
        itemR->setToolTip(getInstrumentToolTip(*It,false,true));
        listView_->addItem( itemR );

        qint32 code = model_.getCode(*It);
        if( code != -1 && model_.isMonitored(Instrument(*It,code)) ) {
            initialMonitoringSet_.insert(code);
            itemL->setHidden(true);
            itemL->setIcon(QIcon(getInstrumentPixmap(*It,true)));
            itemL->setToolTip(getInstrumentToolTip(*It,true,false));
        }
        else {
            itemR->setHidden(true);
            itemR->setIcon(QIcon(getInstrumentPixmap(*It,true)));
            itemR->setToolTip(getInstrumentToolTip(*It,true,true));
        }
    }
    listOrigin_->sortItems();
    listView_->sortItems();

    sslMethod_->addItem(ProtoSSLv2);
    sslMethod_->addItem(ProtoSSLv3);
    sslMethod_->addItem(ProtoTLSv1_0);
    sslMethod_->addItem(ProtoTLSv1_x);
    sslMethod_->addItem(ProtoTLSv1_1);
    sslMethod_->addItem(ProtoTLSv1_1x);
    sslMethod_->addItem(ProtoTLSv1_2);
    sslMethod_->addItem(ProtoTLSv1_2x);
    sslMethod_->addItem(ProtoTLSv1_SSLv3);
    sslMethod_->addItem(ProtoSSLAny);
    sslMethod_->connect(sslMethod_, SIGNAL(currentIndexChanged(int)), this, SLOT(onSecureMethodSelectionChanged(int)));

    QString secMethod = model_.value(ProtocolParam);
    int idx = sslMethod_->findText(secMethod, Qt::MatchExactly);
    if( idx >= 0 )
        sslMethod_->setCurrentIndex(idx);
    sslMethod_->update();
}

QPixmap& SetupDialog::getInstrumentPixmap(const QString& sym, bool hide) const
{
    if( model_.isGlobalAdded(sym) )
        return *(hide ? Global::pxInstNewMoved : Global::pxInstNewNoChange);
    else if( model_.isGlobalChanged(sym) )
        return *(hide ? Global::pxInstEdtMoved : Global::pxInstEdtNoChange);
    return *(hide ? Global::pxInstMoved : Global::pxInstNoChange);
}

QString SetupDialog::getInstrumentToolTip(const QString& sym, bool hide, bool monitoring) const
{
    QString stat;
    if( model_.isGlobalAdded(sym) )
        stat = "New";
    else if( model_.isGlobalChanged(sym) )
        stat = "Changed";
    else if( monitoring && hide )
        return "Will be monitored after Apply";
    else if( monitoring && !hide )
        return "Subscribed";
    else if( !monitoring && hide )
        return "Will be unsubscribed after Apply";
    else if( !monitoring && !hide )
        return "Instrument";

    if( monitoring && hide )
        return stat + ", will be monitored after Apply";
    else if( monitoring && !hide )
        return stat + " and subscribed";
    else if( !monitoring && hide )
        return stat + ", will be unsubscribed after Apply";

    return stat + " instrument";
}

int SetupDialog::exec()
{
    QHBoxLayout* hlayout = new QHBoxLayout();
    QVBoxLayout* vlayout = new QVBoxLayout();

    QWidget* group = getIniGroup();
    group->setFont(*Global::native);
    hlayout->addWidget(group,0,Qt::AlignLeft);

    group = getSetViewGroup();
    group->setFont(*Global::native);
    hlayout->addWidget(group,2,Qt::AlignRight);

    vlayout->addLayout(hlayout);
    vlayout->setAlignment(hlayout, Qt::AlignTop);

    hlayout = new QHBoxLayout();
    btnApply_ = new QPushButton(tr("Apply"),this);
    btnApply_->setFont(*Global::buttons);
    QObject::connect(btnApply_, SIGNAL(clicked()), this, SLOT(onApply()));
    btnApply_->setEnabled(false);
    hlayout->addWidget(btnApply_, 5, Qt::AlignRight);

    QPushButton* btn = new QPushButton(tr("Close"),this);
    btn->setFont(*Global::buttons);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(close()));

    hlayout->addWidget(btn, 0, Qt::AlignRight);
    vlayout->addLayout(hlayout);
    vlayout->setAlignment(hlayout, Qt::AlignBottom);

    setLayout(vlayout);
    resetInfo();

    return QDialog::exec();
}

void SetupDialog::onApply()
{
    model_.setValue(ServerParam, serverDomain_->text());
    model_.setValue(SenderCompParam, senderCompID_->text());
    model_.setValue(TargetCompParam, targetCompID_->text());
    model_.setValue(HeartbeatParam, hbInterval_->text());
    model_.setValue(ProtocolParam, sslMethod_->currentText());
    model_.setValue(PasswordParam, password_->text());

    // searching updatings in viewed instruments
    qint32 countOf = listView_->count();
    for(int row = 0; row < countOf; ++row)
    {
        string sym = listView_->item(row)->text().toStdString();
        if( listView_->item(row)->isHidden() )
        {
            // exists unmounted from view
            QSet<qint32>::iterator It = initialMonitoringSet_.find(model_.getCode(sym.c_str()));
            if( It != initialMonitoringSet_.end() ) {
                initialMonitoringSet_.erase(It);
                qint32 code = model_.getCode(sym.c_str());
                if( code != -1) {
                    model_.setMonitoring(Instrument(sym,code), false, true);
                    listOrigin_->item(row)->setIcon(QIcon(getInstrumentPixmap(sym.c_str(),false)));
                    listOrigin_->item(row)->setToolTip(getInstrumentToolTip(sym.c_str(),false,false));
                }
            }
        }
        else
        {
            // new mounted into view
            qint32 code = model_.getCode(sym.c_str());
            if( initialMonitoringSet_.end() == initialMonitoringSet_.find(code) ) {
                model_.setMonitoring(Instrument(sym,code), true, true);
                initialMonitoringSet_.insert(code);
                listView_->item(row)->setIcon(QIcon(getInstrumentPixmap(sym.c_str(),false)));
                listView_->item(row)->setToolTip(getInstrumentToolTip(sym.c_str(),false,true));
            }
        }
    }
    btnApply_->setEnabled(false);
}

void SetupDialog::onRemoveOrigin()
{
    SelectedT items = listOrigin_->selectedItems();
    if( 0 == items.count() )
        return;
    // sort needed
    qSort(items.begin(),items.end(), &func_less);

    QString names;
    QList<int> rows; 
    while( !items.empty() ) 
    {
        const QString& symbol = items.front()->text();
        if( symbol.isEmpty() ) 
            continue;
        qint32 code = model_.getCode(symbol);
        if( code == -1 ) 
            continue;

        // if first - autoscroll to first item
        // if not first - CRLF
        if( rows.empty() ) {
            int row = listOrigin_->row(items.front());
            QModelIndex rowindex = listOrigin_->model()->index(row,0);
            listOrigin_->scrollTo(rowindex,QAbstractItemView::PositionAtTop);
            listOrigin_->update(rowindex);
        }
        else  
            names += "\n";
            
        names += "\"" + symbol + QString(":%1\"").arg(code);
        rows.push_back(listOrigin_->row( items.front() ));
        items.pop_front();
    }

    QString text = "All settings are rewriting automatically into \"" + 
                    QString(FILENAME_SETTINGS) + "\" after the application exit.\n";
    if( rows.size() == 1 ) 
        text += "Do you want to delete instrument " + names + " from the settings?";
    else
        text += "Do you want to delete selected instruments from the settings?\n" + names;
    
    QMessageBox box( QMessageBox::Question, tr("Delete Instrument"), text, QMessageBox::Yes|QMessageBox::Cancel,this);
    if( 0 == (QMessageBox::Yes & box.exec()) ) 
        return;

    QListWidgetItem* item;
    while( !rows.empty() && (item = listOrigin_->item(rows.back())) )
    {
        string sym = item->text().toStdString();
        qint32 code = model_.getCode(sym.c_str());
        if(code == -1)
            continue;
        model_.instrumentCommit(Instrument(sym.c_str(), code), true);
        listOrigin_->model()->removeRow( rows.back() );
        listView_->model()->removeRow( rows.back() );
        rows.pop_back();
    }
}

void SetupDialog::onAddOrigin()
{
    SymbolEditDialog("", "", model_, this).exec();
}

void SetupDialog::onEditOrigin()
{
    SelectedT items = listOrigin_->selectedItems();
    if( 0 == items.count() || NULL == items.back() )
        return;

    QString sym = items.back()->text();
    if( sym.isEmpty() ) return;

    qint32 code = model_.getCode(sym);
    if( code == -1 ) return;

    SymbolEditDialog(sym, QString("%1").arg(code), model_, this).exec();
}

void SetupDialog::onPlaceToView()
{
    listView_->clearSelection();
    SelectedT its = listOrigin_->selectedItems();
    SelectedT::const_iterator It = its.begin();
    for(; It != its.end(); ++It)
    {
        QListWidgetItem* item = *It;
        if( !item->isSelected() )
            continue;
        const QString& refInstrumentName = item->text();
        item->setHidden(true);
        item->setIcon(QIcon(getInstrumentPixmap(refInstrumentName,true)));
        item->setToolTip(getInstrumentToolTip(refInstrumentName,true,false));

        SelectedT right = listView_->findItems(refInstrumentName, Qt::MatchExactly);
        if( !right.empty() ) {
            item = *right.begin();
            item->setHidden(false);
            item->setSelected(true);
            if( It == its.begin() ) 
                listOrigin_->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
    }
    listOrigin_->clearSelection();

    if( isSymbolViewModified() )
        btnApply_->setEnabled(true);
    else
        btnApply_->setEnabled( isSettingsModified() );
}

void SetupDialog::onRemoveFromView()
{
    listOrigin_->clearSelection();

    SelectedT its = listView_->selectedItems();
    SelectedT::const_iterator It = its.begin();
    for(; It != its.end(); ++It)
    {
        QListWidgetItem* item = *It;
        if( !item->isSelected() )
            continue;
        const QString& refInstrumentName = item->text();
        item->setHidden(true);
        item->setIcon(QIcon(getInstrumentPixmap(refInstrumentName,true)));
        item->setToolTip(getInstrumentToolTip(refInstrumentName,true,true));

        SelectedT left = listOrigin_->findItems(refInstrumentName, Qt::MatchExactly);
        if( !left.empty() ) {
            item = *left.begin();
            item->setHidden(false);
            item->setSelected(true);
            if( It == its.begin() ) 
                listOrigin_->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
    }
    listView_->clearSelection();

    if( isSymbolViewModified() )
        btnApply_->setEnabled(true);
    else
        btnApply_->setEnabled( isSettingsModified() );
}

void SetupDialog::onSelectionChanged()
{
    SelectedT its = listOrigin_->selectedItems();
    btnRemoveOrigin_->setEnabled(!its.empty());
    btnEditOrigin_->setEnabled(!its.empty());
}

QWidget* SetupDialog::getIniGroup()
{
    QGroupBox* groupBox = new QGroupBox(tr("Setup INI"), this);
    groupBox->setAlignment(Qt::AlignCenter);
    groupBox->setFlat(true);

    QVBoxLayout* vlayout = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();

    hlayout->addWidget( new QLabel(tr("Add new instrument (to INI collection):"), this) );
    QPushButton* btn = new QPushButton(NULL,this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onAddOrigin()));
    btn->setFixedSize(24,24);
    btn->setEnabled(true);
    btn->setAutoDefault(false);
    btn->setIconSize( QSize(24,22) );
    btn->setIcon(QIcon( *Global::pxAddOrigin ));
    btn->setToolTip(tr("This operation will stores the new instrument(s) in INI for the next sessions\n"
                       "Note: settings file will be physically updated after adapter will be closed\n"));

    hlayout->addWidget( btn, 1, Qt::AlignTop );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Update instrument symbol/code:"), this) );
    btnEditOrigin_ = new QPushButton(NULL,this);
    QObject::connect(btnEditOrigin_, SIGNAL(clicked()), this, SLOT(onEditOrigin()));
    btnEditOrigin_->setFixedSize(24,24);
    btnEditOrigin_->setIconSize(QSize(23,20));
    btnEditOrigin_->setIcon(QIcon(*Global::pxEditOrigin));
    hlayout->addWidget(btnEditOrigin_, 2, Qt::AlignTop);
    btnEditOrigin_->setEnabled(false);
    btnEditOrigin_->setAutoDefault(false);
    btnEditOrigin_->setToolTip(tr("This operation allows to edit an original instrument and save it in INI for the next sessions\n"
                       "Note: settings file will be physically updated after adapter will be closed\n"
                       "To restore it to original value make delete settings file, then run and close adapter"));

    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Remove instrument (removing from INI):"), this) );
    btnRemoveOrigin_ = new QPushButton(NULL,this);
    QObject::connect(btnRemoveOrigin_, SIGNAL(clicked()), this, SLOT(onRemoveOrigin()));
    btnRemoveOrigin_->setFixedSize(24,24);
    btnRemoveOrigin_->setIconSize(QSize(22,24));
    btnRemoveOrigin_->setIcon(QIcon(*Global::pxRemoveOrigin));
    hlayout->addWidget(btnRemoveOrigin_, 3, Qt::AlignTop); 
    btnRemoveOrigin_->setEnabled(false);
    btnRemoveOrigin_->setAutoDefault(false);
    btnRemoveOrigin_->setToolTip(tr("This operation allows to remove an original instrument from INI settings for ever\n"
                        "Note: settings file will be physically updated after adapter will be closed\n"
                        "To restore it to original value make delete settings file, then run and close adapter"));

    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Host:"), this) );
    serverDomain_ = new DefaultEdit( this, model_.value(ServerParam), tr("Hostname or DNS of the marker server"));

    hlayout->addWidget( serverDomain_);
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("SenderCompID:"), this) );
    senderCompID_ = new DefaultEdit( this, model_.value(SenderCompParam), tr("Name which used for LogIn"));

    hlayout->addWidget( senderCompID_, 4, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("TargetCompID:"), this) );
    targetCompID_ = new DefaultEdit( this, model_.value(TargetCompParam), tr("Trader's service identifier"));

    hlayout->addWidget( targetCompID_, 5, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Heartbeat:"), this) );
    hbInterval_ = new DefaultEdit( this, model_.value(HeartbeatParam), tr("Session heartbeat interval (sec)"));

    hlayout->addWidget( hbInterval_, 7, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Secure Protocol:"), this) );
    sslMethod_ = new QComboBox(this);

    hlayout->addWidget( sslMethod_, 8, Qt::AlignBottom );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Password:"), this) );
    password_ = new DefaultEdit(this, model_.value(PasswordParam), tr("Password for Login"));
    password_->setEchoMode(QLineEdit::Password);

    senderCompID_->setFixedWidth(150);
    targetCompID_->setFixedWidth(150);
    hbInterval_->setFixedWidth(150);
    sslMethod_->setFixedWidth(150);
    password_->setFixedWidth(150);

    hlayout->addWidget( password_, 1, Qt::AlignBottom );
    vlayout->addLayout(hlayout);

    groupBox->setLayout(vlayout);
    return groupBox;
}

QWidget* SetupDialog::getSetViewGroup()
{
    QGroupBox* groupBox = new QGroupBox(tr("Setup table view"), this);
    groupBox->setFlat(true);
    groupBox->setAlignment(Qt::AlignCenter);
    QHBoxLayout* hlayout = new QHBoxLayout();

    ///
    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->addWidget(new QLabel(tr("Instruments base"), this),1,Qt::AlignCenter | Qt::AlignTop);

    listOrigin_ = new QListWidget(this);
    listOrigin_->setFixedSize( width()*0.18f, height()*0.71f);
    listOrigin_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listOrigin_->setFont(*Global::compact);
    QObject::connect(listOrigin_, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
    

    vlayout->addWidget(listOrigin_);
    hlayout->addLayout(vlayout);

    ///
    QPushButton* btn = new QPushButton(NULL,this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onRemoveFromView()));
    btn->setFixedSize(32,32);
    btn->setIconSize( QSize(32,32) );
    btn->setIcon(QIcon( *Global::pxToRemove ));
    btn->setShortcut(QKeySequence(Qt::Key_Delete));
    btn->setToolTip(tr("<Delete>\nRemove selected instrument(s) from the view on table\n"
                       "Note that removing instruments(s) are not cancels really on LMAX side\n"
                       "till logout, simply incoming snapshots will be ignored"));
    btn->setAutoDefault(false);
    vlayout = new QVBoxLayout();
    vlayout->addWidget( btn,7, Qt::AlignBottom );

    btn = new QPushButton(NULL,this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onPlaceToView()));
    btn->setFixedSize(32,32);
    btn->setIconSize( QSize(32,32) );
    btn->setIcon(QIcon( *Global::pxToAdd ));
    btn->setShortcut(QKeySequence(Qt::Key_Insert));
    btn->setToolTip(tr("<Insert>\nMove selected instrument(s) to the right list then view quotes subscription on the table view\n"
                       "Request(s) on selected will be sent immediately in case LMAX is online"));
    btn->setAutoDefault(false);
    vlayout->addWidget( btn, 1, Qt::AlignBottom );
    hlayout->addLayout(vlayout);

    ///
    vlayout = new QVBoxLayout();
    vlayout->addWidget(new QLabel(tr("To be requested"), this),1,Qt::AlignCenter|Qt::AlignTop);

    listView_ = new QListWidget(this);
    listView_->setFixedSize( width()*0.18f, height()*0.71f);
    listView_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listView_->setFont(*Global::compact);
    vlayout->addWidget( listView_, Qt::AlignRight );

    hlayout->addLayout( vlayout );
    groupBox->setLayout(hlayout);

    return groupBox;
}

bool SetupDialog::isSettingsModified()
{
    // verify common settings
    if( serverDomain_->isChanged() )
        return true;
    if( senderCompID_->isChanged() )
        return true;
    if( targetCompID_->isChanged() )
        return true;
    if( hbInterval_->isChanged() )
        return true;
    if( password_->isChanged() )
        return true;
    if( sslMethod_->currentText() != 
        model_.value(ProtocolParam))
        return true;
    return false;
}

bool SetupDialog::isSymbolViewModified()
{
    // verify instruments monitoring settings:
    // are some added?
    qint32 countOf = listView_->count();
    for(int row = 0; row < countOf; ++row)
    {
        if( !listView_->item(row)->isHidden() ) {
            const char* sym = listView_->item(row)->text().toStdString().c_str();
            if( initialMonitoringSet_.end() == initialMonitoringSet_.find(model_.getCode(sym)) )
                return (symbolViewModified_ = true);
        }
    }

    // are some removed?
    for(QSet<qint32>::const_iterator It = initialMonitoringSet_.begin(); 
        It != initialMonitoringSet_.end(); ++It)
    {
        const char* sym = model_.getSymbol(*It);
        SelectedT found = listView_->findItems(sym, Qt::MatchExactly);
        if( found.empty() || (*found.begin())->isHidden() )
            return (symbolViewModified_ = true);
    }
    return (symbolViewModified_ = false);
}

void SetupDialog::onSettingModified(bool differs)
{
    bool applyBtnEnabled = differs | symbolViewModified_ | isSettingsModified();
    btnApply_->setEnabled(applyBtnEnabled);
}

void SetupDialog::onSecureMethodSelectionChanged(int index)
{
    Q_UNUSED(index);

    bool applyBtnEnabled = symbolViewModified_ | isSettingsModified();
    btnApply_->setEnabled(applyBtnEnabled);
}
