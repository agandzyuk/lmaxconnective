#ifndef __setupdialog_h__
#define __setupdialog_h__

#include <QDialog>
#include <QSet>

class DefaultEdit;
class SymbolsModel;

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QComboBox;
QT_END_NAMESPACE

typedef QList<QListWidgetItem*> SelectedT;

class SetupDialog : public QDialog
{
    Q_OBJECT
    friend class SymbolEditDialog;

public:
    SetupDialog(SymbolsModel& model, QWidget* parent);
    int exec();

protected slots:
    void onAddOrigin();
    void onEditOrigin();
    void onRemoveOrigin();
    void onPlaceToView();
    void onRemoveFromView();
    void onSelectionChanged();
    void onApply();
    void onSettingModified(bool differs = true);
    void onSecureMethodSelectionChanged(int index);

private:
    QWidget* getIniGroup();
    QWidget* getSetViewGroup();
    void resetInfo();

    bool isSettingsModified();
    bool isSymbolViewModified();
    QPixmap& getInstrumentPixmap(const QString& sym, bool hide) const;
    QString getInstrumentToolTip(const QString& sym, bool hide, bool monitoring) const;

private:
    SymbolsModel& model_;

    QListWidget* listOrigin_;
    QListWidget* listView_;
    bool symbolViewModified_;

    QSet<qint32> initialMonitoringSet_;

    DefaultEdit* serverDomain_;
    DefaultEdit* senderCompID_;
    DefaultEdit* targetCompID_;
    DefaultEdit* hbInterval_;
    DefaultEdit* password_;
    QComboBox*   sslMethod_;

    // save pointers as a memebers for enable/disable 
    QPushButton* btnEditOrigin_;
    QPushButton* btnRemoveOrigin_;
    QPushButton* btnApply_;
};

#endif // __setupdialog_h__
