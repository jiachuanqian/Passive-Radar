#include "PassiveRadar.h"
#include <QtWidgets/QApplication>


ProcThread procThread;

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    PassiveRadar window;
    window.show();
    return app.exec();
}
