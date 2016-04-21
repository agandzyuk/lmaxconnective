#ifndef __external_h__
#define __external_h__

#define VERSION_MAJOR 1
#define VERSION_MINOR 11

#define ORGANIZATION_NAME           "AlexsisSoft"
#define PRODUCT_NAME                "LMAX Quotings Monitor"

//////////////////////////////////////////////////////////////////////////////
// Defines used by lmaxadaptor
//////////////////////////////////////////////////////////////////////////////

#define FILENAME_SETTINGS           "lmax.ini"
#define FILENAME_FIXMESSAGES        "lmax_messages.log"
#define FILENAME_DEBUGINFO          "lmax_debug.log"
#define MAX_FIXMESSAGES_FILESIZE    (1024*1024*50)
#define MAX_DEBUGINFO_FILESIZE      (1024*1024*100)

// file logging is disabled by default
#define MQL_LOGGING_ENABLED         0

//////////////////////////////////////////////////////////////////////////////
// Defines used by mql.dll
//////////////////////////////////////////////////////////////////////////////

#define MQL_PROXY_PIPE              "mqlpipe"
#define MAX_SYMBOLS                 300
#define MAX_SYMBOL_LENGTH           30

//////////////////////////////////////////////////////////////////////////////
// Typedefs for mql.dll exported routines
//////////////////////////////////////////////////////////////////////////////
typedef double (__stdcall *importGetFunction)(const char*);

//////////////////////////////////////////////////////////////////////////////
// Structs for common using by mean of mql.dll of MQL client and LMAX adapter

//////////////////////////////////////////////////////////////////////////////
#pragma pack(push,r1,2) // set memory alignment to 2 bytes (for members with short type)
// Transaction from mql proxy server (LMAX adapter)
struct MqlProxyQuotes
{
    short numOfQuotes_;
    struct Quote 
    {
        double ask_;
        double bid_;
        char   symbol_[MAX_SYMBOL_LENGTH];
    } quotes_[1];

    static int maxProxyServerBufferSize() 
    { return (sizeof(Quote)*MAX_SYMBOLS + 16); }
};

// Transaction from mql proxy client (mql.dll)
struct MqlProxySymbols
{
    short numOfSymbols_;
    char  symbols_[1][MAX_SYMBOL_LENGTH];

    static int maxProxyClientBufferSize() 
    { return (sizeof(MqlProxySymbols)*MAX_SYMBOLS); }
};
#pragma pack(pop,r1) // restore memory alignment

//////////////////////////////////////////////////////////////////////////////
// Disable VisualC compiler deprication warnings
//////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#pragma warning(disable:4100)
#pragma warning(disable:4996)
#endif

#endif // __external_h__
