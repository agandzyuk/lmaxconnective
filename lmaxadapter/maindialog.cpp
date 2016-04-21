#include "globals.h"
#include "maindialog.h"

#include "netmanager.h"
#include "quotestableview.h"
#include "quotestablemodel.h"
#include "setupdialog.h"
#include "connectdialog.h"
#include "scheduler.h"

#include <QtWidgets>

/* external routines */
QPixmap qt_pixmapFromWinHICON(HICON icon);

MainDialog::MainDialog() 
{
    Global::init();
    setFixedSize(Global::desktop.width()*0.6f, Global::desktop.height()*0.5f);
    netman_.reset(new NetworkManager(this));

    setupButtons();
    setupTable();

    setWindowTitle(Global::productFullName());
}

MainDialog::~MainDialog()
{}

void MainDialog::onStart()
{
    QTimer::singleShot(0, this, SLOT(asyncStart()));
}

void MainDialog::onStop()
{
    QTimer::singleShot(0, this, SLOT(asyncStop()));
}

void MainDialog::onSettings()
{
    SetupDialog(*netman_->model(), this).exec();
}

void MainDialog::onReconnectSetCheck(bool on)
{
    if( reconnectBox_ )
        reconnectBox_->setChecked(on);
}

void MainDialog::onStateChanged(int state, short disconnectStatus)
{
    QColor clr = BTN_RED;
    if( state == CloseWaitState || state == ProgressState ) {
        QCursor* ovrCur = QApplication::overrideCursor();
        if( ovrCur == NULL || ovrCur->shape() != Qt::WaitCursor )
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        clr = BTN_YELLOW;
    }
    if( state == EstablishState ) {
        QApplication::restoreOverrideCursor();
        startButton_->setEnabled( false );
        stopButton_->setEnabled( true );
        clr = BTN_GREEN;
    }
    else {
        QApplication::restoreOverrideCursor();
        startButton_->setEnabled(true);
        stopButton_->setEnabled(false);
    }

    QPalette pal;
    pal.setColor(QPalette::Button, clr);
    colorButton_->setPalette(pal);
}

void MainDialog::setupButtons()
{
    QRect rc = QFontMetrics(*Global::nativeBold).boundingRect(rc, Qt::TextWordWrap|Qt::AlignCenter,tr("Start"));
    QObject::connect(startButton_ = new QPushButton(tr("Start"), this), SIGNAL(clicked()), this, SLOT(onStart()));
    startButton_->setToolTip(tr("Login to LMAX or connection start\n(account settings can be configured in settings))"));
    startButton_->setFont(*Global::buttons);
    startButton_->setAutoDefault(false);
    startButton_->setFixedSize(rc.width()*2, rc.height()*2);
    startButton_->move(10,10);
    startButton_->show();
    
    QObject::connect(stopButton_ = new QPushButton(tr("Stop"), this), SIGNAL(clicked()), this, SLOT(onStop()));
    stopButton_->setToolTip(tr("Logout from LMAX or connection stop"));
    stopButton_->setFont(*Global::buttons);
    stopButton_->setFixedSize(rc.width()*2, rc.height()*2);
    stopButton_->move(startButton_->pos().x()+startButton_->width()+10, 10);
    stopButton_->setEnabled(false);
    stopButton_->setAutoDefault(false);
    stopButton_->show();


    QPalette pal;
    pal.setColor(QPalette::Button, BTN_RED);
    colorButton_ = new QPushButton(NULL, this);
    colorButton_->setToolTip(tr("\"Traffic light\" or connection state color"));
    colorButton_->setFixedSize(rc.height()*2, rc.height()*2);
    colorButton_->move(stopButton_->pos().x()+stopButton_->width()+10, 10);
    colorButton_->setAutoDefault(false);
    colorButton_->setPalette( pal );
    colorButton_->show();

    QObject::connect(reconnectBox_ = new QCheckBox(tr("Reconnect After Failure"), this), SIGNAL(stateChanged(int)), netman_->scheduler(), SLOT(setReconnectEnabled(int)));
    reconnectBox_->setToolTip(tr("Enable/disable auto-reconnecting after connection failures\nNote that the button takes off itself if disconnection was\nby reason of invalid connection settings,\nnot on server side,\nreceived exchange's logout message"));
    reconnectBox_->setCheckState(Qt::Checked);
    reconnectBox_->setFont(*Global::buttons);
    reconnectBox_->move(colorButton_->pos().x()+colorButton_->width()+10, 10);
    reconnectBox_->show();

    QObject::connect(loggingBox_ = new QCheckBox(tr("Logging"), this), SIGNAL(stateChanged(int)), this, SLOT(onStateLoggingEnabled(int)));
    loggingBox_->setToolTip(tr("Enable/disable logging to files\nDebug log \"%1\"\nFix messages log \"%2\"").arg(FILENAME_DEBUGINFO).arg(FILENAME_FIXMESSAGES));
    loggingBox_->setCheckState(Global::logging_ ? Qt::Checked : Qt::Unchecked);
    loggingBox_->setFont(*Global::buttons);
    loggingBox_->move(reconnectBox_->pos().x()+reconnectBox_->width()+10, 10);

    QObject::connect(settingsButton_ = new QPushButton(NULL, this), SIGNAL(clicked()), this, SLOT(onSettings()));
    settingsButton_->setFixedSize(rc.height()*2, rc.height()*2);
    settingsButton_->setIconSize( QSize(colorButton_->width(), 
                                        colorButton_->height()) );
    settingsButton_->setIcon(QIcon( *Global::pxSettings ));

    settingsButton_->move(width() - rc.height()*2 - 10, 10);
    settingsButton_->setAutoDefault(false);
}

void MainDialog::setupTable()
{
    tableview_.reset(new QuotesTableView(this));

    QSize szt( width() - 20, height() - startButton_->height() - 30);

    netman_->model()->resetView(tableview_.data());

    tableview_->setFixedSize(szt);
    tableview_->setFont(*Global::compact);
    tableview_->move(10, startButton_->height() + 20);
    tableview_->show();
    tableview_->updateStyles();

    tableview_->setModel(netman_->model());
}

void MainDialog::asyncStart()
{
    netman_->start(false);
}

void MainDialog::asyncStop()
{
    netman_->stop();
}

void MainDialog::onStateLoggingEnabled(int checked)
{
    Global::logging_ = (checked == Qt::Checked);
    Global::setDebugLog(checked == Qt::Checked);
}

void MainDialog::initiateReconnect()
{
    if( !netman_->scheduler()->activateSSLReconnect() )
        netman_->connectDialog()->setReconnect(false);
}
