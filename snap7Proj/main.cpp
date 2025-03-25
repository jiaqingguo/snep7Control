#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  //  ShowWindow(GetConsoleWindow(), SW_HIDE);
    QApplication a(argc, argv);
    MainWindow w;
   // w.show();
    return a.exec();
}
