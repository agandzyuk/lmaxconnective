#ifndef __symboleditdialog_h__
#define __symboleditdialog_h__

#include "defaultedit.h"
#include <QDialog>

class MarketAbstractModel;

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE


///////////////////////////////////////////////////////////////////////////////////
class SymbolEdit : public DefaultEdit
{
    Q_OBJECT
    friend class SymbolEditDialog;
public:
    enum NotifyControl {
        SymbolAddID = 0,
        SymbolEditID,
        CodeAddID,
        CodeEditID,
    };

public:
    SymbolEdit(const QString& defText, 
               const QString& helpText,
               NotifyControl ctrlID,
               MarketAbstractModel& model,
               QWidget* parent);

    bool updateAllowed() const;

Q_SIGNALS:
    void notifyContentChanged();

protected:
    void keyPressEvent(QKeyEvent* e);
    void update();

private:
    MarketAbstractModel& model_;
    NotifyControl ctrlID_;
    bool updateAllowed_;
};

/////////////////////////////////////////////////////////////////////////////////
class SymbolEditDialog : public QDialog
{
    Q_OBJECT

public:
    SymbolEditDialog(const QString& symbol, 
                     const QString& code, 
                     MarketAbstractModel& model,
                     QWidget* parent);

protected slots:
    void onApply();
    void onOK();
    void onContentChanged();

private:
    QPushButton* btnApply_;
    SymbolEdit* symbolEdit_;
    SymbolEdit* codeEdit_;
    bool addBox_;
};

#endif // __symboleditdialog_h__
