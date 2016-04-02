#include "defines.h"

#include "setupdlg.h"
#include "ini.h"
#include "instrumentbox.h"

#include <QAbstractItemModel>
#include <QtWidgets>
#include <QtGui>
#include <string>


/////////////////////////////////////////////////////////////////////////
bool func_less(QListWidgetItem* left, QListWidgetItem* right)
{
    return *left < *right;
}

/////////////////////////////////////////////////////////////////////////
SetupDlg::SetupDlg(QIni& ini, QWidget* parent) 
    : QDialog(parent),
    ini_(ini),
    changed_(false)
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

void SetupDlg::resetInfo()
{
    QVector<QString> origin;
    QVector<QString> mounted;

    ini_.originSymbols(origin);
    ini_.mountedSymbols(mounted);

    for(int row = 0; row < origin.count(); ++row)
    {
        QListWidgetItem* itemL = new QListWidgetItem(origin[row],listOrigin_);
        listOrigin_->addItem( itemL );
        QListWidgetItem* itemR = new QListWidgetItem(origin[row],listView_);
        listView_->addItem( itemR );

        if( mounted.end() == qFind(mounted, origin[row]) )
            itemR->setHidden(true);
        else
            itemL->setHidden(true);
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

    QString secMethod = ini_.value(ProtocolParam);
    int idx = sslMethod_->findText(secMethod, Qt::MatchExactly);
    if( idx >= 0 )
        sslMethod_->setCurrentIndex(idx);
    sslMethod_->update();
}

int SetupDlg::exec()
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
    QPushButton* btn = new QPushButton(tr("Apply"),this);
    btn->setFont(*Global::buttons);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(close()));
    hlayout->addWidget(btn, 5, Qt::AlignRight);

    btn = new QPushButton(tr("Cancel"),this);
    btn->setFont(*Global::buttons);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(beforeClose()));

    hlayout->addWidget(btn, 0, Qt::AlignRight);
    vlayout->addLayout(hlayout);
    vlayout->setAlignment(hlayout, Qt::AlignBottom);

    setLayout(vlayout);
    resetInfo();

    return QDialog::exec();
}

bool SetupDlg::event(QEvent* e)
{
    if( (e->type() == QEvent::Close) && 
        !e->spontaneous() && 
        isSettingsChanged() )
    {
        onApply();
        accept();
    }
    return QDialog::event(e);
}

void SetupDlg::beforeClose()
{
    changed_ = false; 
    close();
}

void SetupDlg::onApply()
{
    ini_.clear();

    ini_.setValue(ServerParam, serverDomain_->text());
    ini_.setValue(SenderCompParam, senderCompID_->text());
    ini_.setValue(TargetCompParam, targetCompID_->text());
    ini_.setValue(HeartbeatParam, hbInterval_->text());
    ini_.setValue(ProtocolParam, sslMethod_->currentText());
    ini_.setValue(PasswordParam, password_->text());

    ini_.unmountAll();
    int c = listView_->count();
    for(int i=0; i<c; ++i)
        if( !listView_->item(i)->isHidden() )
        {
            const QString& name = listView_->item(i)->text();
            ini_.mountSymbol(name);
        }
}

void SetupDlg::onRemoveOrigin()
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
        const QString& code = ini_.originCodeBySymbol(symbol);
        if( code.isEmpty() ) 
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
            
        names += "\"" + symbol + ":" + code + "\"";
        rows.push_back(listOrigin_->row( items.front() ));
        items.pop_front();
    }

    QString text = "All INI settings are rewriting automatically onto \"" + 
                    QString(DEF_INI_NAME) + "\" after the application exit.\n";
    if( rows.size() == 1 ) 
        text += "Do you want to delete instrument " + names + " from INI?";
    else
        text += "Do you want to delete selected instruments from INI?\n" + names;
    
    QMessageBox box( QMessageBox::Question, tr("Delete Instrument"), text, QMessageBox::Yes|QMessageBox::Cancel,this);
    if( 0 == (QMessageBox::Yes & box.exec()) ) 
        return;

    QListWidgetItem* item;
    while( !rows.empty() && (item = listOrigin_->item(rows.back())) )
    {
        QString symbol = item->text();
        ini_.originRemoveSymbol(symbol);
        listOrigin_->model()->removeRow( rows.back() );
        listView_->model()->removeRow( rows.back() );
        rows.pop_back();
    }

    changed_ = true;
}

void SetupDlg::onAddOrigin()
{
    InstrumentBox("", "", ini_, this).exec();
}

void SetupDlg::onEditOrigin()
{
    SelectedT items = listOrigin_->selectedItems();
    if( 0 == items.count() || NULL == items.back() )
        return;

    QString symbol = items.back()->text();
    if( symbol.isEmpty() )
        return;
    QString code = ini_.originCodeBySymbol(symbol);
    if( code.isEmpty() ) 
        return;

    InstrumentBox(symbol, code, ini_, this).exec();
}

void SetupDlg::onPlaceToView()
{
    SelectedT its = listOrigin_->selectedItems();
    SelectedT::const_iterator It = its.begin();
    for(; It != its.end(); ++It)
    {
        QListWidgetItem* item = *It;
        if( !item->isSelected() )
            continue;
        const QString& refInstrumentName = item->text();
        item->setHidden(true);

        SelectedT right = listView_->findItems(refInstrumentName, Qt::MatchExactly);
        if( !right.empty() ) {
            item = *right.begin();
            item->setHidden(false);
            item->setSelected(true);
            if( It == its.begin() ) 
                listOrigin_->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
        changed_ = true;
    }
    listOrigin_->clearSelection();
}

void SetupDlg::onRemoveFromView()
{
    SelectedT its = listView_->selectedItems();
    SelectedT::const_iterator It = its.begin();
    for(; It != its.end(); ++It)
    {
        QListWidgetItem* item = *It;
        if( !item->isSelected() )
            continue;
        const QString& refInstrumentName = item->text();
        item->setHidden(true);

        SelectedT left = listOrigin_->findItems(refInstrumentName, Qt::MatchExactly);
        if( !left.empty() ) {
            item = *left.begin();
            item->setHidden(false);
            item->setSelected(true);
            if( It == its.begin() ) 
                listOrigin_->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
        changed_ = true;
    }
    listView_->clearSelection();
}

void SetupDlg::onSelectionChanged()
{
    SelectedT its = listOrigin_->selectedItems();
    btnRemoveOrigin_->setEnabled(!its.empty());
    btnEditOrigin_->setEnabled(!its.empty());
}

QWidget* SetupDlg::getIniGroup()
{
    QGroupBox* groupBox = new QGroupBox(tr("Setup INI"), this);
    groupBox->setAlignment(Qt::AlignCenter);
    groupBox->setFlat(true);

    QVBoxLayout* vlayout = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();

    hlayout->addWidget( new QLabel(tr("Add new instrument (to INI collection):"), this) );
    QPushButton* btn = new QPushButton("+",this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onAddOrigin()));
    btn->setFixedSize(24,24);
    btn->setIconSize( QSize(20,20) );
//    btn->setIcon(QIcon( *Global::pxAddOrigin ));
    hlayout->addWidget( btn, 1, Qt::AlignTop );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Update instrument symbol/code:"), this) );
    btn = new QPushButton("*",this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onEditOrigin()));
    btn->setFixedSize(24,24);
//    btn->setIconSize( QSize(24,24) );
//    btn->setIcon(QIcon( *Global::pxEditOrigin ));
    hlayout->addWidget( btn, 2, Qt::AlignTop );
    btnEditOrigin_ = btn;
    btnEditOrigin_->setEnabled(false);
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Remove instrument (removing from INI):"), this) );
    btn = new QPushButton("-",this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onRemoveOrigin()));
    btn->setFixedSize(24,24);
//    btn->setIconSize( QSize(20,20) );
//    btn->setIcon(QIcon( *Global::pxRemoveOrigin ));
    hlayout->addWidget( btn, 3, Qt::AlignTop ); 
    btnRemoveOrigin_ = btn;
    btnRemoveOrigin_->setEnabled(false);
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Host:"), this) );
    serverDomain_ = new QDefEdit( this, ini_.value(ServerParam), tr("Hostname or DNS of the marker server"));

    hlayout->addWidget( serverDomain_);
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("SenderCompID:"), this) );
    senderCompID_ = new QDefEdit( this, ini_.value(SenderCompParam), tr("Name which used for LogIn"));

    hlayout->addWidget( senderCompID_, 4, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("TargetCompID:"), this) );
    targetCompID_ = new QDefEdit( this, ini_.value(TargetCompParam), tr("Trader's service identifier"));

    hlayout->addWidget( targetCompID_, 5, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Heartbeat:"), this) );
    hbInterval_ = new QDefEdit( this, ini_.value(HeartbeatParam), tr("Session heartbeat interval (sec)"));

    hlayout->addWidget( hbInterval_, 7, Qt::AlignBottom  );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Secure Protocol:"), this) );
    sslMethod_ = new QComboBox(this);

    hlayout->addWidget( sslMethod_, 8, Qt::AlignBottom );
    vlayout->addLayout(hlayout);

    hlayout = new QHBoxLayout();
    hlayout->addWidget( new QLabel(tr("Password:"), this) );
    password_ = new QDefEdit(this, ini_.value(PasswordParam), tr("Password for Login"));
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

QWidget* SetupDlg::getSetViewGroup()
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
    QPushButton* btn = new QPushButton("<--",this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onRemoveFromView()));
    btn->setFixedSize(32,24);
//    btn->setIconSize( QSize(32,24) );
//    btn->setIcon(QIcon( *Global::pxToRemove ));

    vlayout = new QVBoxLayout();
    vlayout->addWidget( btn,7, Qt::AlignBottom );

    btn = new QPushButton("-->",this);
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(onPlaceToView()));
    btn->setFixedSize(32,24);
//    btn->setIconSize( QSize(32,24) );
//    btn->setIcon(QIcon( *Global::pxToAdd ));
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

bool SetupDlg::isSettingsChanged()
{
    if( changed_ )
        return true;
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
        ini_.value(ProtocolParam))
        return true;
    return false;
}
