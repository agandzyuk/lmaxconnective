#include "mqlbridge.h"
#include "mqlproxyclient.h"

#include <Windows.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////
MqlBridge::MqlBridge()
    : connection_(NULL),
    quotesLock_(new QReadWriteLock()),
    proxyWaiter_(InitEvent()),
    proxyConnected_(0),
    attached_(0)
{}

MqlBridge::~MqlBridge()
{
    // inactive is lost
    if( connection_ ) {
        delete connection_;
        connection_ = NULL;
    }
}

void MqlBridge::attach()
{
    attached_ = 1;
    return ;
}

void MqlBridge::detach()
{
    attached_ = 0;
    terminate();

    // active connection should be flushed first
    if( proxyConnected_ == 1 ) 
    {
        QMutexLocker guard(&proxyLock_);
        delete connection_;
        connection_ = NULL;
    }
    
    {
        SignalEvent(proxyWaiter_);
        DeleteEvent(proxyWaiter_);
        proxyWaiter_ = NULL;
    }

    { 
        QReadLocker g(quotesLock_); 
    }
    delete quotesLock_;
    quotesLock_ = NULL;
}

bool MqlBridge::isAttached()
{
    return (attached_ == 1);
}

void MqlBridge::run()
{
    // This signal to runner what thread is started
    QObject::connect(connection_, SIGNAL(notifyReadyRead(const char*,qint32,qint32*)), 
                     this, SLOT(onTransaction(const char*,qint32,qint32*)), Qt::DirectConnection);
    QObject::connect(connection_, SIGNAL(notifyNewConnection()), 
                     this, SLOT(onNewConnection()), Qt::DirectConnection);
    QObject::connect(connection_, SIGNAL(notifyDisconnect()), 
                     this, SLOT(onDisconnect()), Qt::DirectConnection);

    // !wake one: thread starter
    SignalEvent(proxyWaiter_);

    while(1)
    {
        // on first entering this region will be accessed after full pipe initialization or when connection failed
        // on second entering when connection is established or failed
        {
            QMutexLocker guard(&proxyLock_);
        }

        // !wait two: events loop
        if( NULL == proxyWaiter_ || !WaitForEvent(proxyWaiter_))
        {
            // waiter returns false in one case of destroying and waiter is in waiting state
            break;
        }

        if( proxyConnected_ == 1 ) // starting reading
        {
            (*connection_).readSync();
            if( proxyWaiter_ )
                SignalEvent(proxyWaiter_);
            else
                break;
        }
        else // or closing the pipe if already opened
            (*connection_).checkPipeState();
    }
    // thread should be destroyed with exit(0)
    QThread::exec();
}

bool MqlBridge::proxyStart(QScopedPointer<QMutexLocker>& autolock)
{
    bool res = false;
    autolock.reset(new QMutexLocker(&proxyLock_) );

    if( NULL != (connection_ = new MqlProxyClient(this)) )
    {
        start();
        // !wait one: thread starter
        if( !(res = TimedWaitForEvent(proxyWaiter_,500)) )
            apiShowError("Starting the thread of pipe client is timed out (MqlBridge::proxyStart)");
    }
    return res;
}

void MqlBridge::proxyConnect(QScopedPointer<QMutexLocker>& autolock)
{
    if( autolock.isNull() )
        autolock.reset(new QMutexLocker(&proxyLock_));

    if( 1 == (proxyConnected_ = ((*connection_).connectToServer()?1:0))) {
        incomingQuotes_.clear();
        autolock.reset();
        SignalEvent(proxyWaiter_); // !wake two: proxy connected and have to start the pipe reading
    }
}

void MqlBridge::onDisconnect()
{
    QMutexLocker g(&proxyLock_);
    proxyConnected_ = 0;
}

void MqlBridge::onTransaction(const char* buffer, qint32 size, qint32* remainder)
{
    *remainder = 0;
    //(*connection_).dbgInfo("MqlBridge::onTransaction");

    if( size < 2 ) {
        (*connection_).dbgInfo("Error in MqlBridge::onTransaction: Received an empty data");
        Q_ASSERT_X(size >= 2, "MqlBridge::onTransaction()", "Received an empty data");
        return;
    }

    qint32 parsed = 0;
    while( parsed < size )
    {
        MqlProxyQuotes* transaction = (MqlProxyQuotes*)buffer;
        if( transaction->numOfQuotes_ >= MAX_SYMBOLS ) {
            (*connection_).dbgInfo("Error in MqlBridge::onTransaction: Received an unexpected amount of symbols");
            Q_ASSERT_X(transaction->numOfQuotes_ < MAX_SYMBOLS, "MqlBridge::onTransaction()", "Received an unexpected amount of symbols");
            return;
        }

        if( size - parsed < qint32(2 + transaction->numOfQuotes_*sizeof(MqlProxyQuotes::Quote)) ) {
            *remainder = size - parsed;
            //char buf[256];
            //sprintf_s(buf, 256, "buffer is truncated with %u bytes - move ahead", *remainder);
            //(*connection_).dbgInfo(buf);
            return;
        }

/*        string txt = "Received transactions:";
        for(int i = 0; i < transaction->numOfQuotes_; i++) {
            const char* sym = transaction->quotes_[i].symbol_;
            char buf[60];
            sprintf_s(buf, 60,"\n\"%s\" (%f,%f)", sym, transaction->quotes_[i].ask_, transaction->quotes_[i].bid_);
            txt += buf;
        }
        (*connection_).dbgInfo(txt);
*/

        QScopedPointer<QReadLocker> autolock(new QReadLocker(quotesLock_));
        // Insert LMAX adapter symbols which not registered in MQL

        for(int i = 0; i < transaction->numOfQuotes_; i++) {
            bool newAdded = false;
            if(transaction->quotes_[i].ask_ >= 0)
                newAdded = setQuote(transaction->quotes_[i].symbol_, transaction->quotes_[i].ask_, false, autolock);
            if(transaction->quotes_[i].bid_ >= 0)
                newAdded = setQuote(transaction->quotes_[i].symbol_, transaction->quotes_[i].bid_, true, autolock);
            if(newAdded)
                incomingQuotes_.insert(transaction->quotes_[i].symbol_);
        }

        parsed += (2 + transaction->numOfQuotes_*sizeof(MqlProxyQuotes::Quote));
        buffer += (2 + transaction->numOfQuotes_*sizeof(MqlProxyQuotes::Quote));
    }
}

void MqlBridge::onNewConnection()
{
//    (*connection_).dbgInfo("MqlBridge::onNewConnection");

    set<string> existing;
    QReadLocker guard(quotesLock_);
    QuotesT::iterator It = quotes_.begin();
    for(; It != quotes_.end(); ++It)
        if(incomingQuotes_.end() == incomingQuotes_.find(It->first))
            existing.insert(It->first);

    // Sending symbols which registered in MQL but not found in adapter transaction
    if( !existing.empty() ) {
        char* message = createMultipleTransaction(existing);
        if(message)
            (*connection_).sendMessage(message);
        free(message);
    }
}

void MqlBridge::apiShowError(const std::string& info)
{
#ifndef NDEBUG
    ::MessageBoxA(NULL,info.c_str(),"mqld.dll internal error", MB_OK|MB_ICONSTOP);
#else
    ::MessageBoxA(NULL,info.c_str(),"mql.dll internal error", MB_OK|MB_ICONSTOP);
#endif
}

////////////////////////////////////////////////////////////////////////
MqlQuote* MqlBridge::addQuote(const char* szSymbol, QScopedPointer<QWriteLocker>& autolock)
{
    autolock.reset(new QWriteLocker(quotesLock_));
    QuotesT::iterator It = quotes_.insert(QuotesT::value_type(szSymbol, MqlQuote(0,0))).first;
    return &It->second;
}

MqlQuote* MqlBridge::findQuote(const char* szSymbol, QScopedPointer<QReadLocker>& autolock)
{
    if( autolock.isNull() )
        autolock.reset(new QReadLocker(quotesLock_));
    QuotesT::iterator It = quotes_.find(szSymbol);
    if( It == quotes_.end() ) {
        autolock.reset();
        return NULL;
    }
    return &It->second;
}

double MqlBridge::getQuote(const char* sym, bool bid)
{
    {
        QScopedPointer<QMutexLocker> autolock;
        if( connection_ == NULL && !proxyStart(autolock) )
            return 0;

        if( (attached_ == 1) && (proxyConnected_ == 0) )
            proxyConnect(autolock);
    }

    QScopedPointer<QReadLocker> readlock;
    MqlQuote* quote = findQuote(sym, readlock);
    if(quote)
        return (bid ? quote->bid_ : quote->ask_);
    else if( (proxyConnected_ == 0) || (attached_ == 0) ) // don't add symbols when no pipe (or detach progress)
        return 0;

    // readlock is already released inside findQuote() in such condition
    {
        QScopedPointer<QWriteLocker> writelock;
        if( NULL == addQuote(sym, writelock) )
            return 0;
        sendSingleTransaction(sym);
    }
    return 0;
}

void MqlBridge::setAsk(const char* sym, double ask)
{
    QScopedPointer<QReadLocker> autolock;
    setQuote(sym, ask, false, autolock);
}


void MqlBridge::setBid(const char* sym, double bid)
{
    QScopedPointer<QReadLocker> autolock;
    setQuote(sym, bid, true, autolock);
}

bool MqlBridge::setQuote(const char* sym, double value, bool bid, QScopedPointer<QReadLocker>& readlock)
{
    bool added = false;
    MqlQuote* quote = findQuote(sym, readlock);
    while(1) {
        if( quote ) {
            if( bid )
                quote->bid_ = value;
            else
                quote->ask_ = value;
            break;
        }
        QScopedPointer<QWriteLocker> writelock;
        if( NULL == (quote = addQuote(sym, writelock))) break;
        added = true;
    }
    return added;
}

void MqlBridge::sendSingleTransaction(const char* sym)
{
    if( incomingQuotes_.end() != incomingQuotes_.find(sym) )
        return;

    MqlProxySymbols message;
    message.numOfSymbols_ = 1;
    strcpy_s(message.symbols_[0], MAX_SYMBOL_LENGTH, sym);
    (*connection_).sendMessage((const char*)&message);
    incomingQuotes_.insert(sym);
}

char* MqlBridge::createMultipleTransaction(const set<string>& symbols)
{
    MqlProxySymbols* message = (MqlProxySymbols*)malloc( 2 + symbols.size()*sizeof(MqlProxySymbols().symbols_[0]) );
    message->numOfSymbols_ = symbols.size();
    set<string>::const_iterator It = symbols.begin();
    for(qint16 i = 0; It != symbols.end(); ++It, ++i)
        strcpy_s(message->symbols_[i], MAX_SYMBOL_LENGTH, It->c_str());
    return (char*)message;
}
