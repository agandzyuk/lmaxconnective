#include "mqlbridge.h"

#include <QtCore>
#include <algorithm>

const char sc_provider[] = "RESULT";

using namespace std;

static char sp_message[512] = {0};

////////////////////////////////////////////////////////////////////////////
MQLBridge::MQLBridge()
    : QSharedMemory(MEMORY_SHARING_NAME),
    recurse_(0), 
    sh(0)
{}

MQLBridge::~MQLBridge()
{}

bool MQLBridge::attach()
{
    bool b = QSharedMemory::isAttached();

    if( !b ) {
        b = create(sizeof(SharedData));
        if( !b && QSharedMemory::AlreadyExists != error() ) 
        {
            string errortext = errorString().toStdString();
            if( !errortext.empty() ) {
                strcpy_s(sp_message, 512, errortext.c_str());
#ifdef NDEBUG
                ::MessageBoxA(NULL, sp_message, "Error loading mql.dll", MB_OK|MB_ICONSTOP);
#else
                ::MessageBoxA(NULL, sp_message, "Error loading mqld.dll", MB_OK|MB_ICONSTOP);
#endif
            }
            return false;
        }
    }

    b = QSharedMemory::isAttached();
    if( !b )
        QSharedMemory::attach();
    return true;
}

void MQLBridge::detach()
{
    if(isAttached())
        QSharedMemory::detach();
}

bool MQLBridge::isAttached()
{
    return QSharedMemory::isAttached();
}

////////////////////////////////////////////////////////////////////////
char* MQLBridge::provider(int providerNum)
{
    return &(sh->providers_[providerNum * MAX_PROVIDER_LENGTH]);
}

Provider& MQLBridge::providerData(int providerNum)
{
    return sh->data_[providerNum];
}

char* MQLBridge::symbol(int providerNum, int symbolNum)
{
    return &(providerData(providerNum).symbols_[symbolNum * MAX_SYMBOL_LENGTH]);
}

////////////////////////////////////////////////////////////////////////
bool MQLBridge::getSymbols(vector<string>& symbols)
{
    Locker g(this);

	bool result = false;
	for(int prId = 0; prId < MAX_PROVIDERS; ++prId)
    {
        char* prName = provider(prId);
        if( prName == NULL || *prName == 0 )
            break;

		if(strcmp(prName, "MT") && strcmp(prName, "RESULT")) 
            continue;

        for(int symNum = 0; symNum < MAX_SYMBOLS; ++symNum)
        {
            char* symName = symbol(prId, symNum);
            if( NULL == symName || *symName == 0 )
                break;

            if( symbols.end() == find( symbols.begin(), symbols.end(), symName) ) {
			    symbols.push_back(symName);
				result = true;
            }
        }
    }
	return result;
}

int MQLBridge::addProvider(const char* szProvider)
{
    char* szEnd = NULL;
    int prNum = 0;
    for(; prNum < MAX_PROVIDERS; ++prNum) {
        szEnd = provider(prNum);
        if( szEnd == NULL )
            return -1;
        if( *szEnd == 0 )
            break;
    }

    if( szEnd ) {
	    strcpy_s(szEnd, MAX_PROVIDER_LENGTH, szProvider);
	    return prNum;
    }
    return -1;
}


int MQLBridge::findProvider(const char* szProvider)
{
    for(int prNum = 0; prNum < MAX_PROVIDERS ; ++prNum) {
        char* curPr = provider(prNum);
        if( curPr == NULL || 0 == *curPr )
            break;
	    if( 0 == strcmp(curPr, szProvider) )
		    return prNum;
    }
    return addProvider(szProvider);
}

int MQLBridge::addSymbol(const int prId, const char* szSymbol)
{
    char* szEnd = NULL;
    int symNum = 0;
    for(; symNum < MAX_SYMBOLS; ++symNum) {
        szEnd = symbol(prId, symNum);
        if( szEnd == NULL )
            return -1;
        if( *szEnd == 0 )
            break;
    }

    if( szEnd ) {
	    strcpy_s(szEnd, MAX_SYMBOL_LENGTH, szSymbol);
	    return symNum;
    }
    return -1;
}

int MQLBridge::findSymbol(const int prId, const char* szSymbol)
{
    for( int symNum = 0; symNum < MAX_SYMBOLS; ++symNum) {
        char* curSym = symbol(prId, symNum);
        if(curSym == NULL || 0 == *curSym )
            break;
	    if (0 == strcmp(curSym, szSymbol))
		    return symNum;
    }
    return addSymbol(prId, szSymbol);
}

double MQLBridge::getBid(const char* szSymbol)
{
    Locker g(this);

    int prId = findProvider(sc_provider);
    if (prId >= 0) {
	    int code = findSymbol(prId, szSymbol);
	    if (code >= 0)
		    return providerData(prId).qoutes_[code].bid_;
    }
    return -1;
}

double MQLBridge::getAsk(const char* szSymbol)
{
    Locker g(this);

    int prId = findProvider(sc_provider);
    if (prId >= 0) {
	    int code = findSymbol(prId, szSymbol);
	    if (code >= 0)
		    return providerData(prId).qoutes_[code].ask_;
    }
    return -1;
}

void MQLBridge::setBid(const char* szSymbol, double value)
{
    Locker g(this);

    int prId = findProvider(sc_provider);
    if (prId >= 0) {
	    int code = findSymbol(prId, szSymbol);
	    if (code >= 0)
            providerData(prId).qoutes_[code].bid_ = value;
    }
}

void MQLBridge::setAsk(const char* szSymbol, double value)
{
    Locker g(this);

    int prId = findProvider(sc_provider);
    if (prId >= 0) {
	    int code = findSymbol(prId, szSymbol);
	    if (code >= 0) 
            providerData(prId).qoutes_[code].ask_ = value;
    }
}
