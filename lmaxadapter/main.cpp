#include "defines.h"
#include "maindialog.h"
#include "ini.h"

#include <QApplication>
#include <QMessageBox>

#include <windows.h>

using namespace std;

////////////////////////////////////////////////////////////////////////
// DLL importing routins from mql(d).dll
////////////////////////////////////////////////////////////////////////
typedef double (*importGetFunction)(const char*);
typedef void   (*importSetFunction)(const char*, double);
typedef bool   (*importSymbolsFunction)(std::vector<std::string>&);

importGetFunction __getBid;
importGetFunction __getAsk;
importSetFunction __setBid;
importSetFunction __setAsk;
importSymbolsFunction __getSymbols;

HINSTANCE sp_hLib = 0;
void InitMQLLibrary();

////////////////////////////////////////////////////////////////////////
// App main
////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

    InitMQLLibrary();


    Global::truncateMbFromLog(MESSAGES_LOGGING_FILE, MAX_LOG_MESSAGE_SIZE);
    Global::truncateMbFromLog(INFO_LOGGING_FILE, MAX_LOG_INFO_SIZE);

    int code = 0;
    try {
        QApplication app(argc, argv);
        QIni ini;
        MainDialog lmxDlg(ini);
        lmxDlg.show();
        app.exec();
    }
    catch(...) 
    { code = -1; }

    string info = "Session closed at " + Global::timestamp();
    CDebug(false) << info.c_str() << "\n\n";


    if( sp_hLib )
        ::FreeLibrary(sp_hLib);
    return code;
}

////////////////////////////////////////////////////////////////////////
// DLL callers
////////////////////////////////////////////////////////////////////////

void InitMQLLibrary()
{
#ifdef NDEBUG
    sp_hLib = ::LoadLibraryA("mql.dll");
    if( sp_hLib == 0 ) {
        ::MessageBoxA(NULL, "Unable to load mql.dll", "Error", MB_OK|MB_ICONSTOP);
        return;
    }
#else
    sp_hLib = ::LoadLibraryA("mqld.dll");
    if( sp_hLib == 0 ) {
        ::MessageBoxA(NULL, "Unable to load mqld.dll", "Error", MB_OK|MB_ICONSTOP);
        return;
    }
#endif
    __getBid = (importGetFunction)::GetProcAddress(sp_hLib, "__getBid");
    __getAsk = (importGetFunction)::GetProcAddress(sp_hLib, "__getAsk");
    __setBid = (importSetFunction)::GetProcAddress(sp_hLib, "__setBid");
    __setAsk = (importSetFunction)::GetProcAddress(sp_hLib, "__setAsk");
    __getSymbols = (importSymbolsFunction)::GetProcAddress(sp_hLib, "__getSymbols");

    if( __setBid == NULL || __setAsk == NULL || __getSymbols == NULL ) {
#ifdef NDEBUG
        ::MessageBoxA(NULL, "unable to find DLL function in mql.dll", "Error", MB_OK|MB_ICONSTOP);
#else
        ::MessageBoxA(NULL, "unable to find DLL function in mqld.dll", "Error", MB_OK|MB_ICONSTOP);
#endif
        ::FreeLibrary(sp_hLib);
        sp_hLib = 0;
    }
}

bool outerSymbolsUpdate(std::vector<std::string>& symbols)
{
    if( __getSymbols )
        return __getSymbols(symbols);
    return false;
}

void outerClearPrices(const QIni& ini)
{
    vector<string> symbols;
    ini.mountedSymbolsStd(symbols);
    
    for( quint16 i = 0; i < symbols.size(); ++i )
	{
        if( __setBid )
            __setBid(symbols[i].c_str(), 0.0f);
        if( __setAsk )
            __setAsk(symbols[i].c_str(), 0.0f);
	}
}

void outerAddQuoteBid(const QString& symbol, double bid)
{
    string stdsym = symbol.toStdString();
    if( __setBid)
        __setBid(stdsym.c_str(), bid);
}

void outerAddQuoteAsk(const QString& symbol, double ask)
{
    string stdsym = symbol.toStdString();
    if( __setAsk )
        __setAsk(stdsym.c_str(), ask);
}
