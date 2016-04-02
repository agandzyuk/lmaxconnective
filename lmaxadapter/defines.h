#ifndef __defines_h__
#define __defines_h__

#include "logger.h"

#include <QSize>
#include <QFont>
#include <QPixmap>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define MAKE_VERSION_STRING(major,minor) QString("%1.").arg(major) + QString("%1").arg(minor)

#define DEF_INI_NAME            "./lmax.ini"
#define MESSAGES_LOGGING_FILE   "./lmax_messages.txt"
#define INFO_LOGGING_FILE       "./lmax_debug.log"
#define MAX_LOG_MESSAGE_SIZE    (1024*1024*20)
#define MAX_LOG_INFO_SIZE       (1024*1024*30)

#define SYMBOLS_UPDATE_PROVIDER_PERIOD      500 // msecs

QT_BEGIN_NAMESPACE
class QFont;
class QPixmap;
QT_END_NAMESPACE

////////////////////////////////////////////////////////////////////////
// Global objects and constants
////////////////////////////////////////////////////////////////////////

#define BTN_RED         (QColor(0x99,0x33,0x40))
#define BTN_YELLOW      (QColor(0xDD,0xDD,0x00))
#define BTN_GREEN       (QColor(0x20,0xF0,0x20))

#define TEXT_GREEN      (QColor(0x00,0x99,0x22))
#define TEXT_RED        (QColor(0xAA,0x33,0x33))
#define TEXT_LIME       (QColor(0x55,0x99,0x00))
#define TEXT_BROWN      (QColor(0x66,0x66,0x33))
#define TEXT_YELLOW     (QColor(0x99,0x88,0x00))
#define TEXT_DARKGRAY   (QColor(0x33,0x33,0x33))

struct Global {
    static QSize    desktop;
    static QFont*   compact;
    static QFont*   native;
    static QFont*   nativeBold;
    static QFont*   buttons;
    static QFont*   huge;
    static QPixmap* pxSettings;
    static QPixmap* pxAddOrigin;
    static QPixmap* pxRemoveOrigin;
    static QPixmap* pxEditOrigin;
    static QPixmap* pxToAdd;
    static QPixmap* pxToRemove;
    static QPixmap* pxToRefresh;
    static QPixmap* pxViewFIX;
    static QPixmap* pxEditFIX;
    static QFile*   infoLogFile;

    static void init();
    static std::string timestamp();
    static qint32   time();
    static void truncateMbFromLog(const char* filename, quint32 sizeLimit);
};

//////////////////////////////////////////////////////////////////////////////
// Disable VisualC compiler deprication warnings
//////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#pragma warning(disable:4100)
#pragma warning(disable:4996)
#endif

#endif /* __defines_h__ */
