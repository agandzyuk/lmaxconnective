#include "globals.h"
#include "fixmessagedialog.h"
#include "quotestablemodel.h"
#include "fixlogger.h"

#include <QtWidgets>
#include <string>

#if (_WIN32_WINNT >= 0x0600)
static std::string defaultCodecForLocale;
#endif

using namespace std;


////////////////////////////////////////////////////////////////////////////////////
FixMessageDialog::FixMessageDialog(const QString& symbol,
                                   qint32 code,
                                   bool readOnly, 
                                   QuotesTableModel* model)
    : QDialog(model->view()),
    model_(model),
    doc_(NULL),
    next_(NULL),
    prev_(NULL),
    first_(NULL),
    last_(first_),
    send_(NULL),
    symbol_(symbol.toStdString()),
    code_(code)
{
#if (_WIN32_WINNT >= 0x0600)
    defaultCodecForLocale = QTextCodec::codecForLocale()->name().toStdString();
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    setWindowFlags(windowFlags()|Qt::Popup);
    setupWidgets(readOnly);

    if( readOnly )
        setWindowTitle(tr("Incoming FIX for \"")+symbol+":"+QString("%1").arg(code)+"\"");
    else
        setWindowTitle(tr("Outgoing FIX for \"")+symbol+":"+QString("%1").arg(code)+"\"");

    QObject::connect(&model_->msglog(), SIGNAL(notifyHasPrev(bool)), this, SLOT(onHasPrev(bool)));
    QObject::connect(&model_->msglog(), SIGNAL(notifyHasNext(bool)), this, SLOT(onHasNext(bool)));
}

FixMessageDialog::~FixMessageDialog()
{
#if (_WIN32_WINNT >= 0x0600)
    if(defaultCodecForLocale.empty())
        QTextCodec::setCodecForLocale(QTextCodec::codecForName(defaultCodecForLocale.c_str()));
#endif

    QObject::disconnect(&model_->msglog(), SIGNAL(notifyHasPrev(bool)), this, SLOT(onHasPrev(bool)));
    QObject::disconnect(&model_->msglog(), SIGNAL(notifyHasNext(bool)), this, SLOT(onHasNext(bool)));
}

void FixMessageDialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    activateWindow();

    if( !model_->msglog().selectViewBy(symbol_.c_str(), code_, send_ == NULL) )
        return;

    QByteArray text = model_->msglog().getReadLast();
    doc_->setText(text);
    if( send_ ) send_->setEnabled( !text.isEmpty() && model_->loggedIn() );
}

void FixMessageDialog::fromAscii(QByteArray& text) const
{
#if (_WIN32_WINNT >= 0x0600)
    text.replace('\x01', '\x02');
    text = QString(text).toUtf8();
#endif
}

void FixMessageDialog::toAscii(QByteArray& text) const
{
#if (_WIN32_WINNT >= 0x0600)
    text.replace('\x02',1);
    text = QString(text).toLocal8Bit();
#endif
}

void FixMessageDialog::hideEvent(QHideEvent* e)
{
    QDialog::hideEvent(e);
    model_->msglog().deselectView();
}

void FixMessageDialog::onNext()
{
    QByteArray text = model_->msglog().getReadNext();
    doc_->setText(text);
    if( send_ ) send_->setEnabled( !text.isEmpty() && model_->loggedIn() );
}

void FixMessageDialog::onPrev()
{
    QByteArray text = model_->msglog().getReadPrev();
    doc_->setText(text);
    if( send_ ) send_->setEnabled( !text.isEmpty() && model_->loggedIn() );
}

void FixMessageDialog::onFirst()
{
    QByteArray text = model_->msglog().getReadFirst();
    doc_->setText(text);
    if( send_ ) send_->setEnabled( !text.isEmpty() && model_->loggedIn() );
}

void FixMessageDialog::onLast()
{
    QByteArray text = model_->msglog().getReadLast();
    doc_->setText(text);
    if( send_ ) send_->setEnabled( !text.isEmpty() && model_->loggedIn() );
}

void FixMessageDialog::onSend()
{
    QByteArray text = doc_->toPlainText().trimmed().toLocal8Bit();
    toAscii(text);
    if( text.left(9) == "8=FIX.4.4" ) 
        if( model_->normalize(text) )
            emit model_->notifySendingManual(text);
}

void FixMessageDialog::onHasNext(bool has)
{
    next_->setEnabled(has);
    last_->setEnabled(has);
}

void FixMessageDialog::onHasPrev(bool has)
{
    prev_->setEnabled(has);
    first_->setEnabled(has);
}

void FixMessageDialog::setupWidgets(bool readOnly)
{
    QVBoxLayout* vlayout = new QVBoxLayout();
    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setSpacing(10);
    hlayout->setAlignment(Qt::AlignCenter);

    hlayout->addWidget(first_ = new QPushButton(NULL));
    QObject::connect(first_, SIGNAL(clicked()), this, SLOT(onFirst()));
    first_->setFixedSize(24,24);
    first_->setAutoDefault(false);
    first_->setEnabled(false);
    first_->setIconSize(QSize(24,24));
    first_->setIcon(QIcon(*Global::pxFirst));
    first_->setShortcut(QKeySequence(Qt::Key_Home));
    first_->setToolTip(tr("First message <Home>"));

    hlayout->addWidget(prev_ = new QPushButton(NULL));
    QObject::connect(prev_, SIGNAL(clicked()), this, SLOT(onPrev()));
    prev_->setFixedSize(32,24);
    prev_->setAutoDefault(false);
    prev_->setEnabled(false);
    prev_->setIconSize(QSize(32,24));
    prev_->setIcon(QIcon(*Global::pxPrev));
    prev_->setShortcut(QKeySequence(Qt::Key_PageUp));
    prev_->setToolTip(tr("Previous message <PageUp>"));

    hlayout->addWidget(next_ = new QPushButton(NULL));
    QObject::connect(next_, SIGNAL(clicked()), this, SLOT(onNext()));
    next_->setFixedSize(32,24);
    next_->setAutoDefault(false);
    next_->setEnabled(false);
    next_->setIconSize(QSize(32,24));
    next_->setIcon(QIcon(*Global::pxNext));
    next_->setShortcut(QKeySequence(Qt::Key_PageDown));
    next_->setToolTip(tr("Next message <PageDown>"));

    hlayout->addWidget(last_ = new QPushButton(NULL));
    QObject::connect(last_, SIGNAL(clicked()), this, SLOT(onLast()));
    last_->setFixedSize(24,24);
    last_->setAutoDefault(false);
    last_->setEnabled(false);
    last_->setIconSize(QSize(24,24));
    last_->setIcon(QIcon(*Global::pxLast));
    last_->setShortcut(QKeySequence(Qt::Key_End));
    last_->setToolTip(tr("Last message <End>"));

    vlayout->addLayout(hlayout);


    vlayout->addWidget(doc_ = new QTextEdit(this));
    doc_->setFont(*Global::nativeBold);
    doc_->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    doc_->setFixedSize(parentWidget()->width()*0.7f, parentWidget()->height()*0.2f);
    doc_->setAlignment(Qt::AlignJustify);
    doc_->setWordWrapMode(QTextOption::WrapAnywhere);

    // convert 0x02 symbols to SOH in the clipboard then text copied from editor
    // no need for XP
#if (_WIN32_WINNT >= 0x0600)
    QObject::connect(doc_, SIGNAL(copyAvailable(bool)), this, SLOT(onCopyAvailable(bool)));
    QObject::connect(doc_, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
#endif

    if( readOnly ) {
        QPalette pal;
        pal.setColor(QPalette::Base, palette().color(QPalette::Background));
        doc_->setPalette(pal);
        doc_->setReadOnly(true);
    }

    hlayout = new QHBoxLayout();
    hlayout->setSpacing(10);
    if(!readOnly) {
        hlayout->addWidget(send_ = new QPushButton(tr("Send")), 1, Qt::AlignRight);
        QObject::connect(send_, SIGNAL(clicked()), this, SLOT(onSend()));
        send_->setAutoDefault(false);
        send_->setEnabled(false);
        send_->setToolTip(tr("Send message from this editor window (connection may falls down)"));
    }

    QPushButton* btn = new QPushButton(tr("Close"));
    QObject::connect(btn, SIGNAL(clicked()), this, SLOT(close()));
    btn->setAutoDefault(true);
    hlayout->addWidget(btn, 0, Qt::AlignRight);
    vlayout->addLayout(hlayout);
    setLayout(vlayout);
}

void FixMessageDialog::onCopyAvailable(bool yes)
{
    QClipboard* clipboard = QApplication::clipboard();
    if( yes ) {
        QObject::connect(clipboard, SIGNAL(dataChanged()), this, SLOT(onClipboardDataChanged()));
    }
    else {
        QObject::disconnect(clipboard, SIGNAL(dataChanged()), this, SLOT(onClipboardDataChanged()));
    }
}

void FixMessageDialog::onTextChanged()
{
    QChar soh[2] = {QChar(1), QChar(0)};
    QChar stk[2] = {QChar(2), QChar(0)};

    QTextCursor tc = doc_->document()->find(QString(soh));
    while( !tc.isNull() ) {
        tc.deleteChar(); 
        tc.insertText(QString(stk));
        tc = doc_->document()->find(QString(soh), tc);
    }
}

void FixMessageDialog::onClipboardDataChanged()
{
    QClipboard* clipboard = qobject_cast<QClipboard*>(sender());
    if( clipboard ) {
        const QMimeData* mime = clipboard->mimeData();
        if( mime ) {
            QStringList ls = mime->formats();
            // is this mime from our QTextEdit?
            if( -1 != ls.indexOf("application/vnd.oasis.opendocument.text") &&
                -1 != ls.indexOf("text/html") &&
                -1 != ls.indexOf("text/plain") )
            {
                QByteArray txt = mime->data("text/plain");
                toAscii(txt);
                clipboard->setText(txt);
            }
        }
    }
}
