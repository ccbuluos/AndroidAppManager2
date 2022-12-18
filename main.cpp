#include "mainwindow.h"

#include <QApplication>
#include "commander.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Commander cmd;
    MainWindow w;
    w.show();
    return a.exec();
}
