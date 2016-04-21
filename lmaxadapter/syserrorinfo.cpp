#pragma warning(disable:4996)

#include "syserrorinfo.h"
#include <windows.h>

using namespace std;

SysErrorInfo::SysErrorInfo(const string& msg, int errNo)
    : info_(msg)
{
    char buf[512];
    sprintf( buf, "%d", errNo );

    HLOCAL hlocal = NULL; /* message buffer handle */
    BOOL fOk = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	                           NULL, errNo, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                               (LPSTR)&hlocal, 0, NULL);

    if( !fOk )
    {
        // Is it a network-related error? */
        HMODULE hDll = LoadLibraryExA("netmsg.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
        if( hDll != NULL ) 
        {
            fOk = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
                                hDll, errNo, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                (LPSTR)&hlocal, 0, NULL);
            FreeLibrary(hDll);
        }
    }

    if( fOk && (NULL != hlocal) )
    {
        if(fOk > 2 && ('\r' == static_cast<const char*>(hlocal)[fOk-2]) )
		    fOk -= 2;

        info_ += " : " + string((LPSTR)hlocal, fOk) + " (" + buf + ")";
    }
    else
    {
	    info_ += string(" (Error code = ") + buf + ")";
    }

    LocalFree(hlocal);
}
