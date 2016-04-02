#ifndef __connectdlg_h__
#define __connectdlg_h__

#include "requesthandler.h"
#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
QT_END_NAMESPACE

class QIni;

////////////////////////////////////
class ConnectDlg : public QDialog
{
public:
    ConnectDlg(const QIni& ini_, QWidget* parent);

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
    const QIni& ini_;
    QTextEdit* statusEdit_;
    QPushButton* disconnectButton_;

    RequestHandler::ConnectionState state_;
    bool reconnecting_;
    bool hasError_;
    bool hasWarning_;
};

#endif // __connectdlg_h__
