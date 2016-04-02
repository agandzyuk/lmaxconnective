#include "defines.h"
#include "connectdlg.h"
#include "ini.h"
#include "requesthandler.h"
#include "sslclient.h"

#include <QtWidgets>
#include <QtGui>

////////////////////////////////////////////////////////////////////////////////////
ConnectDlg::ConnectDlg(const QIni& ini, QWidget* parent)
    : QDialog(parent),
    ini_(ini),
    state_(InitialState),
    reconnecting_(false),
    hasError_(false),
    hasWarning_(false)
{
    setWindowFlags(windowFlags()|Qt::Popup);

    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->addWidget( statusEdit_ = new QTextEdit(this) );
    statusEdit_->setFont(*Global::nativeBold);
    statusEdit_->setReadOnly(true);
    statusEdit_->setFrameStyle(QFrame::NoFrame);

    hlayout->addWidget( disconnectButton_ = new QPushButton(tr("Abort")), 0, Qt::AlignTop);

    // need to be known about width for height
    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->addLayout( hlayout );
    setLayout(vlayout);

    resize(parent->width()*0.60, parent->height()*0.1);

    setWindowTitle(tr("Connection Event"));
}

void ConnectDlg::setReconnect(bool on)
{
    reconnecting_ = on;
}

void ConnectDlg::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
}

void ConnectDlg::hideEvent(QHideEvent* e)
{
    QDialog::hideEvent(e);

    statusEdit_->clear();
    resize(parentWidget()->width()*0.60, parentWidget()->height()*0.1);

    state_ = InitialState;
    hasError_ = false;
    hasWarning_ = false;
}

void ConnectDlg::updateStatus(const QString& info)
{
    QString statusText;

    // last state
    if(info == InfoConnecting)
    {
        if( !reconnecting_ ) {
            hasError_   = false;
            hasWarning_ = false;
            statusText = InfoConnecting + " to ";
        }
        else
            statusText = InfoReconnect + " to ";

        state_ = ProgressState;
        statusText += ini_.value(ServerParam) + ":443\n";
    }
    else if(info == InfoEstablished) 
    {
        hasError_   = false;
        hasWarning_ = false;
        state_ = EstablishState;
    }
    else if( info.startsWith(InfoClosing) )
    {
        state_ = CloseWaitState;
    }
    else if( info.startsWith(InfoError) )
    {
        hasError_ = true;
        state_ = ErrorState;
    }
    else if( info.startsWith(InfoWarning) )
    {
        hasWarning_ = true;
        state_ = WarningState;
    }
    else if( info == InfoUnconnected )
    {
        if( reconnecting_ )
            QTimer::singleShot(0, parentWidget(), SLOT(initiateReconnect()));
        state_ = UnconnectState;
    }
    else
        return;

    // status
    if( state_ != UnconnectState )
    {
        CDebug() << "Dialog status \"" << info << "\"";
        if( statusText.isEmpty() )
            statusText = info + "\n";

        QScrollBar* sb = statusEdit_->verticalScrollBar();
        int sbval1 = sb->maximum();
        statusEdit_->setText( statusEdit_->toPlainText().append(statusText) );
        int sbval2 = sb->maximum();

        // adjusting box size
        QSize dlgsz = size();
        int delta = sbval2 - sbval1;
        if( dlgsz.height() < Global::desktop.height()*0.5f )
            resize(dlgsz.width(), dlgsz.height() + delta + 1);
   
        sb->setValue(sbval2);
    }

    // 
    adjustColors();
    adjustButton();
    setVisibility();
}

void ConnectDlg::adjustColors()
{
    QPalette pal;
    if( hasError_ )
        pal.setColor(QPalette::Text, TEXT_RED);
    else if( hasWarning_ )
        pal.setColor(QPalette::Text, TEXT_YELLOW);
    else if( state_ == ProgressState )
        pal.setColor(QPalette::Text, TEXT_LIME);
    else if( state_ == EstablishState )
        pal.setColor(QPalette::Text, TEXT_GREEN);
    else if( state_ == CloseWaitState )
        pal.setColor(QPalette::Text, TEXT_BROWN);
    else if( state_ == UnconnectState )
        pal.setColor(QPalette::Text, TEXT_DARKGRAY);
    else
        pal.setColor(QPalette::Text, statusEdit_->palette().color(QPalette::Text));

    pal.setColor(QPalette::Base, palette().color(QPalette::Background));
    statusEdit_->setPalette( pal );
}

void ConnectDlg::adjustButton()
{
    QString label = tr("Close");
    if( state_ == ProgressState || state_ == CloseWaitState ) 
    {
        label = tr("Abort");
        QObject::disconnect(disconnectButton_, SIGNAL(clicked()), this, SLOT(hide()));
        QObject::connect(disconnectButton_, SIGNAL(clicked()), parentWidget(), SLOT(asyncStop()));
    }
    else 
    {
        label = tr("Close");
        QObject::disconnect(disconnectButton_, SIGNAL(clicked()), parentWidget(), SLOT(asyncStop()));
        QObject::connect(disconnectButton_, SIGNAL(clicked()), this, SLOT(hide()));
    }

    disconnectButton_->setFixedWidth(label.length()*10);
    disconnectButton_->setText(label);
    disconnectButton_->update();
}

void ConnectDlg::setVisibility()
{
    if( !isVisible() )
    {
        if( (state_ == ProgressState) || 
            (state_ == CloseWaitState) || 
            (state_ == WarningState) )
        {
            show();
        }
    }
    else if( state_ == EstablishState ) 
    {
        QTimer::singleShot(reconnecting_ ? 5000 : 2000, this, SLOT(hide()));
    }
}
