#ifndef __connectdialog_h__
#define __connectdialog_h__

#include "requesthandler.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QProgressBar;
QT_END_NAMESPACE

////////////////////////////////////
class StatusBar : public QWidget
{
public:
    StatusBar(QWidget* parent);

    void updateStatus(const QString& info);

    void connecting(const QString& host);
    void reconnecting();
    void established(const QString& warning);
    void disconnected(const QString& reason);
    void loggedOut(const QString& reason);

protected:

private:
    void adjustColors();
    void adjustButton();

private:
    QTextEdit*    statusText_;
    QPushButton*  abortButton_;
    QProgressBar* progress_;
    QString       host_;
    RequestHandler::ConnectionState state_;
    bool hasError_;
};

#endif // __connectdialog_h__
