#include <QCoreApplication>
#include <QApplication>
#include <QtGui>
#include "bufferInheritedfromQIODevice.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    bufferPlayback w;

    return a.exec();
}
