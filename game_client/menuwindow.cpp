#include "menuwindow.h"
#include "mygame.h"
#include "ui_menuwindow.h"
#include <QTcpServer>
#include <QMessageBox>
#include <QtEndian>
#include <iostream>

MenuWindow::MenuWindow(QDialog *parent, QTcpSocket *sck) :
    QDialog(parent),
    ui(new Ui::MenuWindow)
{
    this->sck = sck;

    ui->setupUi(this);
    connect(ui->players2Btn, &QPushButton::clicked, this, &MenuWindow::twoPlayers);
    connect(ui->players3Btn, &QPushButton::clicked, this, &MenuWindow::threePlayers);
    connect(ui->players4Btn, &QPushButton::clicked, this, &MenuWindow::fourPlayers);
    connect(this, &QDialog::finished, this, &MenuWindow::closeWindow);
    connect(sck, &QTcpSocket::errorOccurred, this, &MenuWindow::errorOcc);
}

MenuWindow::~MenuWindow()
{
    delete ui;
}

void MenuWindow::closeWindow(){
    server_closed = false;
}

void MenuWindow::twoPlayers(){
    QString txt = "two";
    QByteArray gamemode = (txt).toUtf8();

    char buf[255];
    uint16_t len = sprintf(buf+2, "%s", gamemode.data());
    *(uint16_t*)buf = qToBigEndian(len);
    sck->write(buf, 2+len);
    gameboard = new GameBoard(this, sck);
    gameboard->exec();
    gameboard->deleteLater();
}

void MenuWindow::threePlayers(){
    QString txt = "three";
    QByteArray gamemode = (txt).toUtf8();

    char buf[255];
    uint16_t len = sprintf(buf+2, "%s", gamemode.data());
    *(uint16_t*)buf = qToBigEndian(len);
    sck->write(buf, 2+len);
    gameboard = new GameBoard(this, sck);
    gameboard->exec();
    gameboard->deleteLater();
}

void MenuWindow::fourPlayers(){
    QString txt = "four";
    QByteArray gamemode = (txt).toUtf8();

    char buf[255];
    uint16_t len = sprintf(buf+2, "%s", gamemode.data());
    *(uint16_t*)buf = qToBigEndian(len);
    sck->write(buf, 2+len);
    gameboard = new GameBoard(this, sck);
    gameboard->exec();
    gameboard->deleteLater();
}

void MenuWindow::errorOcc(){
    if(server_closed){
        QMessageBox::critical(this, "Error", "Utracono połączenie z serwerem");
    }
    this->close();
}

