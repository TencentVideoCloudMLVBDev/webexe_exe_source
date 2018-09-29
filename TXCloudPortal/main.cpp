#include <QtWidgets/QApplication>

#include "Application.h"
#include "crashdump.h"

#ifdef ENABLE_QT_STATIC_PACKET
#include <QtCore/QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

int main(int argc, char *argv[])
{
	//QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // DPI support

    CrashDump dump;

    return Application::instance().run(argc, argv);
}
