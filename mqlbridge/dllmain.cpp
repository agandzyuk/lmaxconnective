#include "mqlbridge.h"

#include <memory>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////
/// Defines
#define DLLEXPORT __declspec(dllexport)

#define BridgeOrFalse                               \
    MQLBridge* bridge = spMqlBridge();              \
    if( NULL == bridge || !bridge->isAttached() )   \
        return FALSE

#define BridgeOrZero                                \
    MQLBridge* bridge = spMqlBridge();              \
    if( NULL == bridge || !bridge->isAttached() )   \
        return 0.0f

#define BridgeOrReturn                              \
    MQLBridge* bridge = spMqlBridge();              \
    if( NULL == bridge || !bridge->isAttached() )   \
        return

///////////////////////////////////////////////////////////////////////
/// Main
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    BOOL dllRet = TRUE;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        if( NULL == spMqlBridge() || !spMqlBridge()->attach() )
            dllRet = FALSE;
        break;
	case DLL_THREAD_ATTACH:
        break;
	case DLL_THREAD_DETACH:
        break;
	case DLL_PROCESS_DETACH:
        if( spMqlBridge() )
            spMqlBridge()->detach();
		break;
	}
	return dllRet;
}


///////////////////////////////////////////////////////////////////////
/// Exports
#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DLLEXPORT bool __getSymbols(std::vector<std::string>& symbols)
{
    BridgeOrFalse;
	return bridge->getSymbols(symbols);
}

DLLEXPORT double __getBid(const char* symbol)
{
    BridgeOrZero;
    return bridge->getBid(symbol);
}

DLLEXPORT double __getAsk(const char* symbol)
{
    BridgeOrZero;
    return bridge->getAsk(symbol);
}

DLLEXPORT void __setBid(const char* symbol, double value)
{
    BridgeOrReturn;
    bridge->setBid(symbol, value);
}

DLLEXPORT void __setAsk(const char* symbol, double value)
{
    BridgeOrReturn;
    bridge->setAsk(symbol, value);
}

#ifdef __cplusplus
}
#endif