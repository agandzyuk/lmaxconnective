#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>

#include <conio.h>
#include <ctype.h>
#include <time.h>

using namespace std;

const int   timestamp_ms_size  = sizeof("YYYYMMDD-HH:MM:SS.sss");
FILE* fcheck;

namespace {
    void setCursorPos(int row, int column)
    {
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD c;
        c.X = column;
        c.Y = row;
        SetConsoleCursorPosition(hConsoleOutput, c);
        fflush(stdout);
    }

    void getCursorPos(int* row, int* column)
    {
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo( hConsoleOutput, &info);
        *column = info.dwCursorPosition.X;
        *row = info.dwCursorPosition.Y;
    }

    void getConsoleSize(int* width, int* height) 
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        memset(&info, 0, sizeof(info));
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        if( GetConsoleScreenBufferInfo(hConsoleOutput, &info) )
        {
            *width  = info.srWindow.Right;
            *height = info.srWindow.Bottom;
        }
    }

    void clearCell(HANDLE hConsoleOutput, int consoleLine, int consoleColumn)
    {
        COORD c;
        c.X = consoleColumn;
        c.Y = consoleLine;
        unsigned long written;
        SetConsoleCursorPosition(hConsoleOutput, c);
        WriteConsoleA(hConsoleOutput, "                   ", 19, &written, NULL);
        SetConsoleCursorPosition(hConsoleOutput, c);
    }

    void printTransCell(long long trans, int consoleLine)
    {
        char szTrans[15];
        unsigned long written;
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        clearCell(hConsoleOutput, consoleLine, 13);
        sprintf_s(szTrans, 15, "%llu", trans);
        WriteConsoleA(hConsoleOutput, szTrans, strlen(szTrans), &written, NULL);
    }    

    void printBidCell(double bid, int consoleLine)
    {
        char szBid[15];
        unsigned long written;
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        clearCell(hConsoleOutput, consoleLine, 44);
        sprintf_s(szBid, 15, "%f", bid);
        WriteConsoleA(hConsoleOutput, szBid, strlen(szBid), &written, NULL);
    } 

    void printAskCell(double ask, int consoleLine)
    {
        char szAsk[15];
        unsigned long written;
        HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        clearCell(hConsoleOutput, consoleLine, 19);
        sprintf_s(szAsk, 15, "%f", ask);
        WriteConsoleA(hConsoleOutput, szAsk, strlen(szAsk), &written, NULL);
    } 

    typedef unsigned char  u8;
    typedef unsigned short u16;

    u8 sprintfTimeStamp( u16 year, 
                         u8 month, 
                         u8 day, 
                         u8 hour, 
                         u8 minute, 
                         u8 second, 
                         char* buf )
    {
    	u8 pos = 0;
//    	assert(year >= 1000);
//    	assert(year <= 9999);
    	buf[pos + 3] = 48 + year%10; year/=10;
    	buf[pos + 2] = 48 + year%10; year/=10;
    	buf[pos + 1] = 48 + year%10; year/=10;
    	buf[pos + 0] = 48 + year%10; 
    	pos = 4;
//    	assert(month <= 12);
    	buf[pos + 1] = 48 + month%10; month/=10;
    	buf[pos + 0] = 48 + month%10; 
    	pos = 6;
//    	assert(day <= 31);
    	buf[pos + 1] = 48 + day%10; day/=10;
    	buf[pos + 0] = 48 + day%10; 
    	pos = 8; buf[pos] = '-';
    	pos = 9;
//    	assert(hour <= 24);
    	buf[pos + 1] = 48 + hour%10; hour/=10;
    	buf[pos + 0] = 48 + hour%10; 
    	pos = 11; buf[pos] = ':';
    	pos = 12;
//    	assert(minute <= 60);
    	buf[pos + 1] = 48 + minute%10; minute/=10;
    	buf[pos + 0] = 48 + minute%10; 
    	pos = 14; buf[pos] = ':';
    	pos = 15;
//    	assert(second <= 60);
    	buf[pos + 1] = 48 + second%10; second/=10;
    	buf[pos + 0] = 48 + second%10; 
    	buf[17] = 0;
    	return 17;
    }

    u8 sprintfTimeStampWithMSec(u16 year, 
                                u8  month, 
                                u8  day, 
                                u8  hour, 
                                u8  minute, 
                                u8  second, 
                                u16 msecond, 
                                char*  buf)
    {
    	u8 pos = sprintfTimeStamp(year, month, day, hour, minute, second, buf);
    	buf[pos] = '.';
    	pos = 18;
//      assert(msecond <= 999);
    	buf[pos + 2] = 48 + msecond%10; msecond/=10;
    	buf[pos + 1] = 48 + msecond%10; msecond/=10;
    	buf[pos + 0] = 48 + msecond%10; 
    	buf[21] = 0;
        return 21;
    }

    std::string timestamp()
    {
        char out[timestamp_ms_size];
        SYSTEMTIME t;
        ::GetSystemTime(&t);
	    sprintfTimeStampWithMSec(t.wYear, (u8)t.wMonth , (u8)t.wDay, (u8)t.wHour , (u8)t.wMinute , (u8)t.wSecond, t.wMilliseconds, out);
        return out;
    }
}

#include "symbols.cpp"
///////////////////////////////////////////////////////////////////////////////////
typedef double (__stdcall *importGetFunction)(const char*);

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

    bool good = false;
    if( NULL == __getBid )
    {
        ::MessageBoxA(NULL, "Can't import __getBid from dll!", "Error", MB_OK|MB_ICONSTOP);
    }
    else if( NULL == __getAsk )
    {
        ::MessageBoxA(NULL, "Can't import __getAsk from dll!", "Error", MB_OK|MB_ICONSTOP);
    }
    else
    {
        good = true;
    }

    if( !good ) {
        FreeLibrary(hInst);
        return -1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    // Testing __getAsk and __getBid calls with maximum intensity
    // Delay and latency calculated from number of passed transactions 
    // on enough amount of instruments.
    // Test duration about 5 min.

    fcheck = fopen("fromshare.log", "wc+");

    printf("Testing __getAsk() and __getBid() calls with maximum intensity.\n"
           "Delay and latency calculated from number of passed transactions\n"
           "for some amount of instruments (20). Queries for symbols are not randomized and\n"
           "each test iteration is repeating by order of symbols declaration in hardcode.\n"
           "Test duration about 2 min.\n\n");

    int MAX_INSTRUMENT = 2;
    int JOB_TIME = 5; // seconds

    int startRow = 0; 
    int startColumn = 0;
    double bid,ask;
    const char* szSym;

    struct Query {
        string name_;
        double lastBid_;
        double lastAsk_;
    };

    getConsoleSize(&startColumn, &startRow);

    time_t starttime = time(0);
    int count = sizeof(InstNameDefaults)/sizeof(string);

    int i = 0;
    for(; i < startRow-6; ++i)
        printf("\n");
    setCursorPos(7,0);

    vector<Query> insts;

    for(i = 0; i < MAX_INSTRUMENT && i < startRow-6; ++i)
    {
        Query q = {InstNameDefaults[i], 0.0f, 0.0f};
        insts.push_back(q);
        printf("\"%s\"", q.name_.c_str());

        int r, c; 
        getCursorPos(&r,&c);
        setCursorPos(r,15);
        printf("ask=                    ", q.lastAsk_);
        setCursorPos(r,40);
        printf("bid=                    \n", q.lastBid_);
        startRow--;
    }
    MAX_INSTRUMENT = i;

    printf("\nTransactions=0\n");
    startRow = 7;
   
    int ck = 0;
    long long transactions = 0;
    for(;;)
    {
        bool noChangeTransactBefore = false;

        if( ck == MAX_INSTRUMENT ) {
            printTransCell(transactions, startRow+ck+1);
            ck = 0;
        }

        szSym = insts[ck].name_.c_str();
        ask = __getAsk(szSym);
        if( ask < 0 ) {
            ::MessageBoxA(NULL, "Error calling __getAsk()", "Error", MB_OK|MB_ICONSTOP);
            return -1;
        }
        bid = __getBid(szSym);
        if( bid < 0 ) {
            ::MessageBoxA(NULL, "Error calling __getBid()", "Error", MB_OK|MB_ICONSTOP);
            return -1;
        }
        transactions += 2;

        bool changed = false;
        if( ask != insts[ck].lastAsk_ ) {
            insts[ck].lastAsk_ = ask;
            printAskCell(ask,startRow+ck);
            changed = true;
        }
        if( bid != insts[ck].lastBid_ ) {
            insts[ck].lastBid_ = bid;
            printBidCell(bid,startRow+ck);
            changed = true;
        }

        if( changed )
        {
            char buf[256];
            sprintf_s(buf, 256, "%s: \"%s\"\task=%f, bid=%f\n", timestamp().c_str(), szSym, ask, bid);
            fwrite(buf, 1, strlen(buf), fcheck);
            fflush(fcheck);
        }
        
        if( time(0) - starttime > JOB_TIME*60 )
            break;
        ck++;
    }

    long long val = transactions/(time(0)-starttime);
    printf("\nTest passed: transactions per sec = %llu, testing time = %d secs\n\n", val, time(0)-starttime);

    printf("Press a key to exit...\n");
    fflush(stdin);
    _getch();

    return 0;
}