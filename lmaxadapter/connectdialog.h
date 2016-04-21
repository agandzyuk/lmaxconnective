#ifndef __connectdialog_h__
#define __connectdialog_h__

#include "requesthandler.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
QT_END_NAMESPACE

class BaseIni;

////////////////////////////////////
class ConnectDialog : public QDialog
{
public:
    ConnectDialog(const BaseIni& ini, QWidget* parent);

    void updateStatus(const QString& info);
    void setReconnect(bool on);
    bool getReconnect() { return reconnecting_; }

protected:
    void hideEvent(QHideEvent*);
    void showEvent(QShowEvent*);

private:
    void adjustColors();
    void adjustButton();
    void setVisibility();

private:
    const BaseIni& ini_;
    QTextEdit* statusEdit_;
    QPushButton* disconnectButton_;

    RequestHandler::ConnectionState state_;
    bool reconnecting_;
    bool hasError_;
    bool hasWarning_;
};

#endif // __connectdialog_h__
