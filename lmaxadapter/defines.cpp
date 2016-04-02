#include "defines.h"
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
QPixmap* Global::pxToRefresh;
QPixmap* Global::pxViewFIX;
QPixmap* Global::pxEditFIX;
QFile*   Global::infoLogFile = NULL;

void Global::init()
{
    infoLogFile = new CLogFile();
    std::string info = "Session started at " + timestamp();
    CDebug(false) << info.c_str() << "\n";

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

    /*
    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxAddOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON3), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxRemoveOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON6), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxEditOrigin = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON4), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxToAdd = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON5), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxToRemove = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON7), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxToRefresh = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON8), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxViewFIX = new QPixmap( qt_pixmapFromWinHICON(hIco) );

    hIco = (HICON)::LoadImage(hInst, 
                            MAKEINTRESOURCE(IDI_ICON9), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    pxEditFIX = new QPixmap( qt_pixmapFromWinHICON(hIco) );
    */
}

qint32 Global::time()
{
    return ::GetTickCount();
}

std::string Global::timestamp()
{
    char out[timestamp_ms_size];
    SYSTEMTIME t;
    ::GetSystemTime(&t);
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

