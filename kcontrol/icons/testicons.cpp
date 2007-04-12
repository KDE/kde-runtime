/* Test program for icons setup module. */

#include <QtGui/QApplication>
#include <kcomponentdata.h>
#include "icons.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv, "testicons");
    KComponentData componentData("testicons");
    KIconConfig *w = new KIconConfig(componentData, 0L);
    w->show();
    return app.exec();
}
