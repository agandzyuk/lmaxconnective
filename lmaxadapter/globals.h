#ifndef __globals_h__
#define __globals_h__

#include "logger.h"

#include <QSize>
#include <QFont>
#include <QPixmap>

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
    static bool     logging_;
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
    static QPixmap* pxViewFIX;
    static QPixmap* pxEditFIX;
    static QPixmap* pxFirst;
    static QPixmap* pxPrev;
    static QPixmap* pxNext;
    static QPixmap* pxLast;
    static QPixmap* pxInstNoChange;
    static QPixmap* pxInstMoved;
    static QPixmap* pxInstNewNoChange;
    static QPixmap* pxInstNewMoved;
    static QPixmap* pxInstEdtNoChange;
    static QPixmap* pxInstEdtMoved;
    static QPixmap* pxSubscribe;
    static QPixmap* pxUnSubscribe;
    static QPixmap* pxRemoveRow;
    static QFile*   infoLogFile;

    static void init();
    static void setDebugLog(bool on);
    static std::string timestamp();
    static std::string timestamp(qint64 timet);
    static qint32 time();
    static qint64 systemtime();
    static void truncateMbFromLog(const char* filename, quint32 sizeLimit);
    static QString organizationName();
    static QString productFullName();

    // Helper: adjusts to 5 digits of real accuracy
    inline static void reinterpretDouble(const char* sznum, double* convert) {
        *convert = atol(sznum);
        const char* dot = strchr(sznum,'.');
        double c = 0.1;
        while(dot && *(++dot) ) {
            if( c < 1e-005 ) break;
            *convert += c * (*dot - 48); 
            c *= 0.1;
        }
    }
};

#endif // __globals_h__
