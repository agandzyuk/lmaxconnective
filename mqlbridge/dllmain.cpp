#include "mqlbridge.h"
#include <QString>
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

DLLEXPORT(double) __getBid(const wchar_t* symbol)
{
    BridgeOrZero;
    return bridge->getBid(QString::fromWCharArray(symbol).toLocal8Bit());
}

DLLEXPORT(double) __getAsk(const wchar_t* symbol)
{
    BridgeOrZero;
    return bridge->getAsk(QString::fromWCharArray(symbol).toLocal8Bit());
}

DLLEXPORT(void) __setBid(const wchar_t* symbol, double value)
{
    BridgeOrReturn;
    bridge->setBid(QString::fromWCharArray(symbol).toLocal8Bit(), value);
}

DLLEXPORT(void) __setAsk(const wchar_t* symbol, double value)
{
    BridgeOrReturn;
    bridge->setAsk(QString::fromWCharArray(symbol).toLocal8Bit(), value);
}

#ifdef __cplusplus
}
#endif
