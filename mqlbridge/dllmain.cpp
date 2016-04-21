#include "mqlbridge.h"
#include <Windows.h>

///////////////////////////////////////////////////////////////////////
/// Defines
#define DLLEXPORT(retType) __declspec(dllexport) retType __stdcall

#define BridgeOrZero                                \
    MqlBridge* bridge = spMqlBridge();              \
    if( NULL == bridge || !bridge->isAttached() )   \
        return 0.0f

#define BridgeOrReturn                              \
    MqlBridge* bridge = spMqlBridge();              \
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
        if( NULL == spMqlBridge() )
            dllRet = FALSE;
        spMqlBridge()->attach();
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

DLLEXPORT(double) __getBid(const char* symbol)
{
    BridgeOrZero;
    return bridge->getBid(symbol);
}

DLLEXPORT(double) __getAsk(const char* symbol)
{
    BridgeOrZero;
    return bridge->getAsk(symbol);
}

DLLEXPORT(void) __setBid(const char* symbol, double value)
{
    BridgeOrReturn;
    bridge->setBid(symbol, value);
}

DLLEXPORT(void) __setAsk(const char* symbol, double value)
{
    BridgeOrReturn;
    bridge->setAsk(symbol, value);
}

#ifdef __cplusplus
}
#endif
