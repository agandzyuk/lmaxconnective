#ifndef __instrumentbox_h__
#define __instrumentbox_h__

#include "defedit.h"
#include <QDialog>

class QIni;

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE


///////////////////////////////////////////////////////////////////////////////////
class QNotifyEdit : public QDefEdit
{
    Q_OBJECT

public:
    enum NotifyControl {
        SymbolAddID = 0,
        SymbolEditID,
        CodeAddID,
        CodeEditID,
    };

public:
    QNotifyEdit(const QIni& ini_,
                const QString& defText, 
                const QString& helpText,
                NotifyControl ctrlID,
                QWidget* parent);

    bool updateAllowed() const;

Q_SIGNALS:
    void notifyContentChanged();

protected:
    void keyPressEvent(QKeyEvent* e);
    void update();

private:
    const QIni& ini_;
    NotifyControl ctrlID_;
    bool updateAllowed_;
};

/////////////////////////////////////////////////////////////////////////////////
class InstrumentBox : public QDialog
{
    Q_OBJECT

public:
    InstrumentBox(const QString& symbol, 
                  const QString& code, 
                  const QIni& ini,
                  QWidget* parent);

protected slots:
    void onApply();
    void onContentChanged();

private:
    QPushButton* btnApply_;
    QNotifyEdit* symbolEdit_;
    QNotifyEdit* codeEdit_;
    bool addBox_;
};

#endif // __instrumentbox_h__
