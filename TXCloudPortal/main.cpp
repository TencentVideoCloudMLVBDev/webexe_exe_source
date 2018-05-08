#include <QtWidgets/QApplication>

#include "Application.h"
#include "crashdump.h"

int main(int argc, char *argv[])
{
    CrashDump dump;

    return Application::instance().run(argc, argv);
}
