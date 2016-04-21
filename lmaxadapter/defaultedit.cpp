#include "defaultedit.h"
#include <QtWidgets>

///////////////////////////////////////////////////////////////////////////////////
DefaultEdit::DefaultEdit(QWidget* parent, const QString& defText, const QString& helpText)
    : QLineEdit(parent),
    defText_(defText),
    helpText_(helpText),
    defColor_(false)
{
    setText(defText_);
    update();
    QObject::connect(this, SIGNAL(notifyModified(bool)), parent, SLOT(onSettingModified(bool)) );
}

void DefaultEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);
    update();
}

void DefaultEdit::update()
{
    QLineEdit::update();

    if( !defColor_ && text() == defText_ ) {
        setStyleSheet("QLineEdit {color: #004499}");
        defColor_ = true;
        emit notifyModified(false);
    }
    else if( defColor_ && text() != defText_ ) {
        setStyleSheet("QLineEdit {color: 0}");
        defColor_ = false;
        emit notifyModified(true);
    }
}

bool DefaultEdit::event(QEvent* e)
{
    if( e->type() == QEvent::ToolTip && !helpText_.isEmpty() ) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(e);
        if( helpEvent )
            QToolTip::showText(helpEvent->globalPos(), helpText_);
        return true;
    }
    return QLineEdit::event(e);
}

bool DefaultEdit::isChanged()
{ 
    return !defColor_; 
}

const QString& DefaultEdit::defaultText()
{
    return defText_;
}

void DefaultEdit::setDefaultText(const QString& text)
{
    defText_ = text;
    setText(defText_);
    update();
}
