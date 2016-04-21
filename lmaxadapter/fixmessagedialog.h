#ifndef __fixmessagedialog_h__
#define __fixmessagedialog_h__

#include <QDialog>

class QuotesTableModel;

QT_BEGIN_NAMESPACE
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

////////////////////////////////////
class FixMessageDialog : public QDialog
{
    Q_OBJECT
public:
    FixMessageDialog(const QString& symbol, qint32 code,
                     bool readOnly, QuotesTableModel* model);
    ~FixMessageDialog();

protected slots:
    void showEvent(QShowEvent* e);
    void hideEvent(QHideEvent* e);
    void onNext();
    void onPrev();
    void onFirst();
    void onLast();
    void onSend();
    void onHasNext(bool has);
    void onHasPrev(bool has);
    void onCopyAvailable(bool yes);
    void onClipboardDataChanged();
    void onTextChanged();

private:
    void setupWidgets(bool readOnly);
    void fromAscii(QByteArray& text) const;
    void toAscii(QByteArray& text) const;

private:
    QuotesTableModel* model_;
    QTextEdit* doc_;
    QPushButton* next_;
    QPushButton* prev_;
    QPushButton* first_;
    QPushButton* last_;
    QPushButton* send_;

    std::string symbol_;
    qint32 code_;
};

#endif // __fixmessagedialog_h__
