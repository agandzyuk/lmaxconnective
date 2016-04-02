#ifndef __deflineedit_h__
#define __deflineedit_h__

#include <QLineEdit>

class QDefEdit : public QLineEdit
{
public:
    QDefEdit(QWidget* parent, 
        const QString& defText, 
        const QString& helpText);
    
    bool isChanged();
    const QString& defaultText();
    void setDefaultText(const QString& text);

protected:
    void keyPressEvent(QKeyEvent* e);
    void update();
    bool event(QEvent* e);

protected:
    QString defText_;

private:
    QString helpText_;
    bool defColor_;
};

#endif /* __deflineedit_h__ */
