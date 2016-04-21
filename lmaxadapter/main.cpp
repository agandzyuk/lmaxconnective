#include "globals.h"
#include "maindialog.h"

#include <QApplication>
#include <QMessageBox>
#include <windows.h>

using namespace std;

////////////////////////////////////////////////////////////////////////
// App main
////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    int code = 0;
    try {
        QApplication app(argc, argv);
        MainDialog lmxDlg;
        lmxDlg.show();
        app.exec();
    }
    catch(const exception& ex) 
    {
        ::MessageBoxA(NULL, ex.what(), "Application error", MB_OK|MB_ICONSTOP);
        code = -1; 
    }

    if(code == 0) {
        string info = "Session closed at " + Global::timestamp();
        CDebug(false) << info.c_str() << "\n\n";
    }
    return code;
}
