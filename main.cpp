#include <QCoreApplication>
#include "core/CoreManager.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);

    CoreManager core;
    core.initialize();

    return a.exec();
}
