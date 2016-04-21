#ifndef __maindialog_h__
#define __maindialog_h__

#include "netmanager.h"
#include <QDialog>

class QIni;
class LMXTable;

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

class MainDialog : public QDialog, public NetworkManager
{
    Q_OBJECT

public:
    MainDialog(QIni& ini);
    ~MainDialog();

protected:
    void setupButtons();
    void setupTable();

public:
    void onStateChanged(ConnectionState state, 
                        short disconnectStatus = 0);

    bool event(QEvent* e);
    // UI slots
protected Q_SLOTS:
    void onStart();
    void onStop();
    void onSettings();
    void asyncStart();
    void asyncStop();
    void initiateReconnect();
    
private:
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QPushButton* colorButton_;
    QPushButton* settingsButton_;
    LMXTable*    table_;

    QIni& ini_;
};

#endif // __maindialog_h__
