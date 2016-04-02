#include "defines.h"
#include "maindialog.h"

#include "ini.h"
#include "table.h"
#include "model.h"
#include "setupdlg.h"
#include "connectdlg.h"
#include "scheduler.h"

#include <QtWidgets>

/* external routines */
QPixmap qt_pixmapFromWinHICON(HICON icon);

MainDialog::MainDialog(QIni& ini) 
    : NetworkManager(ini),
    ini_(ini),
    table_(NULL)
{
    Global::init();

    setFixedSize(Global::desktop.width()*0.6f, Global::desktop.height()*0.5f);
    setWindowTitle(tr("LMAX Online Quotes"));

    NetworkManager::setParent(this);

    setupButtons();
    setupTable();
}

MainDialog::~MainDialog()
{
    ini_.Save();
}

void MainDialog::onStart()
{
    QTimer::singleShot(0, this, SLOT(asyncLaunchConnectionDialog()));
    QTimer::singleShot(0, this, SLOT(asyncStart()));
}

void MainDialog::onStop()
{
    QTimer::singleShot(0, this, SLOT(asyncStop()));
}

void MainDialog::onSettings()
{
    if( SetupDlg(ini_, this).exec() )
        setupTable();
}

void MainDialog::onStateChanged(ConnectionState state, 
                                 short disconnectStatus)
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

    NetworkManager::onStateChanged(state, disconnectStatus);
}

void MainDialog::setupButtons()
{
    QObject::connect(startButton_ = new QPushButton(tr("Start"), this), SIGNAL(clicked()), this, SLOT(onStart()));
    startButton_->setFont(*Global::buttons);
    startButton_->setAutoDefault(false);
    QRect rc;
    rc = QFontMetrics(*Global::nativeBold).boundingRect(rc, 
                                                          Qt::TextWordWrap|Qt::AlignCenter,
                                                          tr("Start"));
    startButton_->setFixedSize(rc.width()*2, rc.height()*2);
    startButton_->move(10,10);
    
    QObject::connect(stopButton_ = new QPushButton(tr("Stop"), this), SIGNAL(clicked()), this, SLOT(onStop()));
    stopButton_->setFont(*Global::buttons);
    stopButton_->setFixedSize(rc.width()*2, rc.height()*2);
    stopButton_->move(25+rc.width()*2, 10);

    stopButton_->setEnabled(false);
    stopButton_->setAutoDefault(false);

    colorButton_ = new QPushButton(NULL, this);
    colorButton_->setFixedSize(rc.height()*2, rc.height()*2);
    colorButton_->move(40+rc.width()*4, 10);
    colorButton_->setAutoDefault(false);
    QPalette pal;
    pal.setColor(QPalette::Button, BTN_RED);
    colorButton_->setPalette( pal );

    QCheckBox* chk = new QCheckBox(tr("Reconnect After Failure"), this);
    QObject::connect(chk, SIGNAL(stateChanged(int)), scheduler_.data(), SLOT(setReconnectEnabled(int)));
    chk->setCheckState(Qt::Checked);
    chk->setFont(*Global::buttons);
    chk->move(55+rc.width()*4+rc.height()*2, 10);

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
    LMXModel* model = dynamic_cast<LMXModel*>(dispatcher_.data());
    if( model == NULL ) {
        ::MessageBoxA(NULL, "Error of displaying table!", "Error", MB_OK|MB_ICONSTOP);
        return;
    }

    if( table_ ) {
        table_->close();
        table_->deleteLater();
    }

    QSize szt( width() - 20, height() - startButton_->height() - 30);

    table_ = new LMXTable(this);
    model->resetView(table_);

    table_->setFixedSize(szt);
    table_->setFont(*Global::compact);
    table_->move(10, startButton_->height() + 20);
    table_->show();
    table_->updateStyles();

    table_->setModel(model);
}

void MainDialog::asyncStart()
{
    NetworkManager::asyncStart(false);
}

void MainDialog::asyncStop()
{
    NetworkManager::asyncStop();
}

void MainDialog::initiateReconnect()
{
    if( !scheduler_->activateSSLReconnect() )
        connectInfoDlg_->setReconnect(false);
}
