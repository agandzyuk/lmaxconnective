#ifndef __defaultedit_h__
#define __defaultedit_h__

#include <QLineEdit>

class DefaultEdit : public QLineEdit
{
    Q_OBJECT

public:
    DefaultEdit(QWidget* parent, 
                const QString& defText, 
                const QString& helpText);
    
    bool isChanged();
    const QString& defaultText();
    void setDefaultText(const QString& text);

Q_SIGNALS:
    void notifyModified(bool differs);

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

#endif /* __defaultedit_h__ */
