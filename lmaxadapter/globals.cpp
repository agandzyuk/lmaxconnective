#include "globals.h"
#include "resource.h"

#include <QDesktopWidget>
#include <QFont>
#include <QPixmap>
#include <QTime>

#include <string>
#include <windows.h>

namespace {
    typedef unsigned char  u8;
    typedef unsigned short u16;

    const int   timestamp_ms_size  = sizeof("YYYYMMDD-HH:MM:SS.sss");
    u8 sprintfTimeStamp(u16 year, u8 month, u8 day, u8 hour, u8 minute, u8 second, char* buf);
    u8 sprintfTimeStampWithMSec(u16 year, u8 month, u8 day, u8 hour, u8 minute, u16 second, u16 ms, char* buf);
}

/* external routine */
QPixmap qt_pixmapFromWinHICON(HICON icon);

const int   compactPx    = 9;
const float nativePx     = 12;
const float buttonsPx    = 13;

#if !defined(MQL_LOGGING_ENABLED) || !(MQL_LOGGING_ENABLED)
bool Global::logging_ = false;
#elif defined(MQL_LOGGING_ENABLED)
bool Global::logging_ = true;
#endif

QSize    Global::desktop;
QFont*   Global::compact;
QFont*   Global::native;
QFont*   Global::nativeBold;
QFont*   Global::buttons;
QFont*   Global::huge;
QPixmap* Global::pxSettings;
QPixmap* Global::pxAddOrigin;
QPixmap* Global::pxRemoveOrigin;
QPixmap* Global::pxEditOrigin;
QPixmap* Global::pxToAdd;
QPixmap* Global::pxToRemove;
QPixmap* Global::pxViewFIX;
QPixmap* Global::pxEditFIX;
QPixmap* Global::pxFirst;
QPixmap* Global::pxPrev;
QPixmap* Global::pxNext;
QPixmap* Global::pxLast;
QPixmap* Global::pxInstNoChange;
QPixmap* Global::pxInstMoved;
QPixmap* Global::pxInstNewNoChange;
QPixmap* Global::pxInstNewMoved;
QPixmap* Global::pxInstEdtNoChange;
QPixmap* Global::pxInstEdtMoved;
QPixmap* Global::pxSubscribe;
QPixmap* Global::pxUnSubscribe;
QPixmap* Global::pxRemoveRow;


QFile*   Global::infoLogFile = NULL;

QString Global::organizationName()
{ return ORGANIZATION_NAME; }

QString Global::productFullName()
{ return PRODUCT_NAME + QString(" v.%1.%2").arg(VERSION_MAJOR).arg(VERSION_MINOR); }

void Global::setDebugLog(bool on)
{
    if( on )
        infoLogFile = new CLogFile();
    else if (infoLogFile) {
        delete infoLogFile;
        infoLogFile = NULL;
    }
}

void Global::init()
{
    if( logging_ ) setDebugLog(true);

    QDesktopWidget desk;
    QRect rcScreen( desk.screenGeometry() );
    desktop.setHeight( rcScreen.height()+1 );
    desktop.setWidth( rcScreen.width()+1 );

    compact = new QFont();
    compact->setPixelSize( compactPx );

    native = new QFont();
    native->setPixelSize( nativePx );

    nativeBold = new QFont();
    nativeBold->setPixelSize( nativePx );
    nativeBold->setBold(true);

    buttons = new QFont();
    buttons->setPixelSize( buttonsPx );

    huge = new QFont();
    huge->setPixelSize( buttonsPx );
    huge->setBold(true);


    HINSTANCE hInst = ::GetModuleHandle(NULL);
    HICON hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_SETTINGS), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxSettings = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ADDORIGIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxAddOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_REMORIGIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxRemoveOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_EDITORIGIN), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxEditOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ADDTOVIEW), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxToAdd = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_RMFROMVIEW), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxToRemove = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_VIEWFIX), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxViewFIX = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_EDITFIX), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxEditFIX = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_FIRST), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxFirst = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_PREV), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxPrev = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxNext = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_LAST), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxLast = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INST_NOCHANGE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstNoChange = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INST_MOVED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstMoved = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INSTNEW_NOCHANGE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstNewNoChange = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INSTNEW_MOVED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstNewMoved = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INSTEDT_NOCHANGE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstEdtNoChange = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_INSTEDT_MOVED), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxInstEdtMoved = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_SUBSCRIBE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxSubscribe = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_UNSUBSCRIBE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxUnSubscribe = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_REMOVEROW), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxRemoveRow = new QPixmap( qt_pixmapFromWinHICON(hIco) );
}

qint32 Global::time()
{
    return ::GetTickCount();
}

qint64 Global::systemtime()
{
    SYSTEMTIME st;
    ::GetSystemTime(&st);
    FILETIME ft;
    ::SystemTimeToFileTime(&st, &ft);
    qint64 tm64 = ft.dwHighDateTime;
    tm64 <<= 32;
    tm64 |= ft.dwLowDateTime;
    return tm64;
}

std::string Global::timestamp()
{
    char out[timestamp_ms_size];
    SYSTEMTIME t;
    ::GetSystemTime(&t);
	sprintfTimeStampWithMSec(t.wYear, t.wMonth , t.wDay, t.wHour , t.wMinute , t.wSecond, t.wMilliseconds, out);
    return out;
}

std::string Global::timestamp(qint64 timet)
{
    FILETIME ft;
    ft.dwHighDateTime = (DWORD)((timet&0xFFFFFFFF00000000)>>32);
    ft.dwLowDateTime = (DWORD)((timet&0xFFFFFFFF));
    SYSTEMTIME t;
    ::FileTimeToSystemTime(&ft, &t);

    char out[timestamp_ms_size];
	sprintfTimeStampWithMSec(t.wYear, t.wMonth , t.wDay, t.wHour , t.wMinute , t.wSecond, t.wMilliseconds, out);
    return out;
}

namespace 
{
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
                                u16 second, 
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
}


void Global::truncateMbFromLog(const char* filename, quint32 sizeLimit)
{
    QFile file(filename);
    if( !file.exists() )
        return;

    if( file.open(QIODevice::ReadWrite|QIODevice::Append) )
    {
        if( file.size() > sizeLimit )
        {
            // truncate a data from head up to 300 Kb from a tail
            int szlv = file.size()-0x50000-1;
            if( file.seek(szlv) ) {
                QByteArray datalv = file.read(10*1024);
                if( datalv.size() >= 0x50000-2 )
                {
                    file.close();
                    if( file.open(QIODevice::ReadWrite|QIODevice::Truncate) )
                    {
                        file.write(datalv);
                        file.close();
                    }
                }
            }
        }
    }
}

