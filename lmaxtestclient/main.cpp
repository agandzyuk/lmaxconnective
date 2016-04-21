#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

#include <conio.h>
#include <ctype.h>
#include <time.h>

using namespace std;

#include "symbols.cpp"
///////////////////////////////////////////////////////////////////////////////////
typedef double (__stdcall *importGetFunction)(const char*);
typedef void   (__stdcall *importSetFunction)(const char*, double);
typedef bool   (__stdcall *importSymbolsFunction)(std::vector<std::string>&);
typedef void   (__stdcall *importSignalFunction)(LPLONG);

///////////////////////////////////////////////////////////////////////////////////
struct find_string {
    const string& lvalue_;
    find_string(const string& value) : lvalue_(value) {}
    bool operator()(const string& rvalue) const
    { return lvalue_ == rvalue; }
};

///////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
#ifndef NDEBUG
    HINSTANCE hInst = ::LoadLibraryA("mqld.dll");
    if( hInst == NULL ) {
        ::MessageBoxA(NULL, "Can't load mqld.dll!", "Error", MB_OK|MB_ICONSTOP);
        return -1;
    }
#else
    HINSTANCE hInst = ::LoadLibraryA("mql.dll");
    if( hInst == NULL )
        ::MessageBoxA(NULL, "Can't load mqld.dll!", "Error", MB_OK|MB_ICONSTOP);
#endif

    importGetFunction __getBid = (importGetFunction)::GetProcAddress(hInst, "__getBid");
    importGetFunction __getAsk = (importGetFunction)::GetProcAddress(hInst, "__getAsk");
    importSymbolsFunction __getSymbols = (importSymbolsFunction)::GetProcAddress(hInst, "__getSymbols");

    bool good = false;
    if( NULL == __getBid )
    {
        ::MessageBoxA(NULL, "Can't import __getBid from dll!", "Error", MB_OK|MB_ICONSTOP);
    }
    else if( NULL == __getAsk )
    {
        ::MessageBoxA(NULL, "Can't import __getAsk from dll!", "Error", MB_OK|MB_ICONSTOP);
    }
    else if( NULL == __getAsk )
    {
        ::MessageBoxA(NULL, "Can't import __getSymbols from dll!", "Error", MB_OK|MB_ICONSTOP);
    }
    else
    {
        good = true;
    }

    if( !good ) {
        FreeLibrary(hInst);
        return -1;
    }

    int testcases = 0;
    if( argc > 1 && argv[1] == "1" && (testcases = 1) ) 
        goto __testsymbols;
    else if( argc == 1 || argc > 1 && argv[1] == "2" && (testcases = 2) ) 
        goto __traffic;
    else
        goto __end;

__testsymbols:
    ////////////////////////////////////////////////////////////////////////////////////////
    // Test on receive symbols. 
    // Some set of instruments (>10) must be choosen to view 
    // in LMAXAdapter settings dialog
    // This are symbols should be obtained and here


    printf( "Running test for symbols receiving.\n"
            "Please run LMAXAdaptor and choose at least three symbols to view in the settings dialog.\n"
            "Warning: test is limited by 10 min!\n\n");

{
    time_t starttime = time(0);
    vector<string> symbols;
    for(;;) {
        vector<string> copy = symbols;
        if( __getSymbols(symbols) && copy.size() != symbols.size() )
        {
            for(vector<string>::iterator s_It = symbols.begin(); s_It != symbols.end(); ++s_It)
            {
                vector<string>::iterator copy_It = find_if(copy.begin(), copy.end(), find_string(*s_It));
                if( copy_It != copy.end() ) {
                    copy.erase(copy_It);
                    continue;
                }
                printf("\"%s\" symbol loaded\n", s_It->c_str());
            }
        }
        if( time(0) - starttime > 600 ) 
        {
            printf("Test FAIL by timeout!\n\n");
            printf("Press a key to exit...\n");
            _getch();
            return -1;
        }
        else if( symbols.size() >= 3 )
            break;
        Sleep(100);
    }
}
    printf("Test passed OK!\n\n\n");

    printf("Press a key to exit...\n");
    fflush(stdin);
    _getch();

    if( testcases == 1)
        goto __end;

__traffic:
    ////////////////////////////////////////////////////////////////////////////////////////
    // Run this test with 10-20 processes with logged LMAXAdapter
    // Don't load any instruments on view in LMAX settings
    // 
    // 

    printf("Run this test on 10-20 processes with logged LMAXAdapter.\n"
           "Don't load any instruments on view in LMAX instruments settings.\n"
           "Each test process must requests for random three Market symbols.\n"
           "Symbols should appeared in table of LMAXAdapter automatically.\n"
           "Symbol duration about 10 min, symbols request period 50 msecs.\n\n"
           "NOTE: TO ACTIVATE FIRST TESTER LISTENER simple add one instrument to LMAX table view.\n\n");

{
    time_t starttime = time(0);
    int count = sizeof(InstNameDefaults)/sizeof(string);
    
    double lastBid[] = {0.0f, 0.0f, 0.0f};
    double lastAsk[] = {0.0f, 0.0f, 0.0f};

    vector<string> choosen;
    int request_idx2 = 0, request_idx = 0;
    for(;;)
    {
        vector<string> symbols;
        if( (0 == choosen.size() && __getSymbols(symbols)) ||
            (choosen.size() && choosen.size() < 2) )
        {
            DWORD tick = GetTickCount();
            srand(tick);
            int j = tick/rand();
            srand(j);
            request_idx = rand()%count;
            choosen.push_back( InstNameDefaults[request_idx] );
            request_idx += count; 
            request_idx %= choosen.size()+1;
            request_idx2 = rand()%(choosen.size()+1);
            printf("New request for \"%s\" is ready!\n", choosen.back().c_str());
            fflush(stdout);
        }

        Sleep(50);
        int ck = choosen.size();
        if( ck == 0 )
            continue;
        do {
            ck--;
            const char* szSym = choosen[ck].c_str();
            double bid = __getBid(szSym);
            double ask = __getAsk(szSym);
            if( bid != lastBid[ck] ) {
                printf("bid changed for \"%s\"\t= %f\n", szSym, bid);
                lastBid[ck] = bid;
            }
            Sleep(50);
            if( ask != lastAsk[ck] ) {
                printf("ask changed for \"%s\"\t= %f\n", szSym, ask);
                lastAsk[ck] = ask;
            }
        }
        while(ck);

        if( time(0) - starttime > 600 )
            break;
    }
}
    printf("Test passed OK!\n\n");

    printf("Press a key to exit...\n");
    fflush(stdin);
    _getch();

    if(testcases == 2)
        goto __end;
__end:

    return 0;
}