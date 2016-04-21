#ifndef __maindialog_h__
#define __maindialog_h__

#include "requesthandler.h"
#include <QDialog>

class QuotesTableView;
class QuotesTableModel;
class NetworkManager;
class MqlProxyClient;

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog();
    ~MainDialog();

public slots:
    void onReconnectSetCheck(bool on);
    void onStateLoggingEnabled(int checked);

protected:
    void setupButtons();
    void setupTable();

    // UI slots
protected Q_SLOTS:
    void onStart();
    void onStop();
    void onSettings();
    void asyncStart();
    void asyncStop();
    void initiateReconnect();
    void onStateChanged(int state, short disconnectStatus = 0);
    
private:
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QPushButton* colorButton_;
    QPushButton* settingsButton_;
    QCheckBox*   reconnectBox_;
    QCheckBox*   loggingBox_;

    QSharedPointer<QuotesTableView> tableview_;
    QSharedPointer<NetworkManager>  netman_;
};

#endif // __maindialog_h__
