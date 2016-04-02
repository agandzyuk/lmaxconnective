#ifndef __setupdlg_h__
#define __setupdlg_h__

#include <QDialog>

class QDefEdit;
class QIni;

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QComboBox;
QT_END_NAMESPACE

typedef QList<QListWidgetItem*> SelectedT;

class SetupDlg : public QDialog
{
    Q_OBJECT

    friend class InstrumentBox;

public:
    SetupDlg(QIni& settings, QWidget* parent);
    int exec();

protected:
    bool event(QEvent* e);
    QWidget* getIniGroup();
    QWidget* getSetViewGroup();
    void resetInfo();
    void onApply();

protected slots:
    void beforeClose();
    void onAddOrigin();
    void onEditOrigin();
    void onRemoveOrigin();
    void onPlaceToView();
    void onRemoveFromView();
    void onSelectionChanged();

private:
    bool isSettingsChanged();

    QIni& ini_;
    bool changed_;

    QListWidget* listOrigin_;
    QListWidget* listView_;

    QDefEdit*   serverDomain_;
    QDefEdit*   senderCompID_;
    QDefEdit*   targetCompID_;
    QDefEdit*   hbInterval_;
    QDefEdit*   password_;
    QComboBox*  sslMethod_;

    // save pointers as a memebers for enable/disable 
    QPushButton* btnEditOrigin_;
    QPushButton* btnRemoveOrigin_;
};

#endif // __setupdlg_h__
