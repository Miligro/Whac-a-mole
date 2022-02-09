#include "mygame.h"

#include <QApplication>
#include <QTcpServer>
#include <iostream>
int main(int argc, char *argv[])
{
    QTcpSocket *sck = new QTcpSocket;
    QApplication a(argc, argv);
    MyGame w(nullptr, sck);
    w.show();
    return a.exec();
}
