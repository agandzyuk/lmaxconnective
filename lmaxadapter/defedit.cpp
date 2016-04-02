#include "defedit.h"
#include <QtWidgets>

///////////////////////////////////////////////////////////////////////////////////
QDefEdit::QDefEdit(QWidget* parent, const QString& defText, const QString& helpText)
    : QLineEdit(parent),
    defText_(defText),
    helpText_(helpText),
    defColor_(false)
{
    setText(defText_);
    update();
}

void QDefEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);
    update();
}

void QDefEdit::update()
{
    QLineEdit::update();

    if( !defColor_ && text() == defText_ ) {
        setStyleSheet("QLineEdit {color: #004499}");
        defColor_ = true;
    }
    else if( defColor_ && text() != defText_ ) {
        setStyleSheet("QLineEdit {color: 0}");
        defColor_ = false;
    }
}

bool QDefEdit::event(QEvent* e)
{
    if( e->type() == QEvent::ToolTip && !helpText_.isEmpty() ) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
        if( helpEvent )
            QToolTip::showText(helpEvent->globalPos(), helpText_);
        return true;
    }
    return QLineEdit::event(e);
}

bool QDefEdit::isChanged()
{ 
    return !defColor_; 
}

const QString& QDefEdit::defaultText()
{
    return defText_;
}

void QDefEdit::setDefaultText(const QString& text)
{
    defText_ = text;
    setText(defText_);
    update();
}
