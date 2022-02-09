#include "mygame.h"
#include "./ui_mygame.h"
#include "menuwindow.h"
#include <QMessageBox>
#include <QtEndian>
#include <iostream>


MyGame::MyGame(QWidget *parent, QTcpSocket *sck)
    : QDialog(parent)
    , ui(new Ui::MyGame)
{
    this->sck = sck;
    ui->setupUi(this);
    connect(ui->confirmBtn, &QPushButton::clicked, this, &MyGame::confirmUsername);
    connect(ui->connectBtn, &QPushButton::clicked, this, &MyGame::connectToServer);
    connect(sck, &QTcpSocket::errorOccurred, this, &MyGame::errorOcc);
}

MyGame::~MyGame()
{
    if(sck != nullptr){
        sck->close();
    }
    delete ui;
}

void MyGame::errorOcc(){
    error_occ = true;
    sck->abort();
    ui->connectBtn->setEnabled(true);
    QMessageBox::critical(this, "Error", "Wystąpił błąd");
}

void MyGame::connectToServer(){
    error_occ = false;
    QString serverAddress = ui->serverAddress->text();
    int serverPort = ui->serverPort->value();
    ui->connectBtn->setEnabled(false);
    connect(sck, &QTcpSocket::connected, this, &MyGame::socketConnected);
    connect(sck, &QTcpSocket::readyRead, this, &MyGame::readData);
    sck->connectToHost(serverAddress, serverPort);
    connTimeoutTimer = new QTimer(this);
    connTimeoutTimer->setSingleShot(true);
    connect(connTimeoutTimer, &QTimer::timeout, [&]{
        sck->abort();
        if(!error_occ){
            QMessageBox::critical(this, "Error", "Przekroczono limit czasu połączenia");
            ui->connectBtn->setEnabled(true);
        }
        connTimeoutTimer->deleteLater();
    });
    connTimeoutTimer->start(2000);
}

void MyGame::socketConnected(){
    ui->connectStatus->setText("Status połączenia: połączono");
    connTimeoutTimer->stop();
    connTimeoutTimer->deleteLater();
    ui->connectBtn->setEnabled(false);
}

void MyGame::confirmUsername(){
    if(sck->state() == QAbstractSocket::ConnectedState && send_username){
        send_username = false;
        QString txt = ui->username->text().trimmed();
        if(txt.size()>0){
            QByteArray username = (txt).toUtf8();
            char buf[255];
            uint16_t len = sprintf(buf+2, "%s", username.data());
            *(uint16_t*)buf = qToBigEndian(len);
            sck->write(buf, 2+len);
        }
        else{
            QMessageBox::information(this, "Informacja","Podaj conajmniej jeden znak");
        }
    }
}

QByteArray MyGame::readMessage(){
    quint16 msgSize;
    QByteArray msg = sck->read(2);
    if(msg.size()<2){
        sck->close();
    }
    QDataStream dataStream(msg);
    dataStream >> msgSize;
    msg = sck->read(msgSize);
    return msg;
}

void MyGame::readData(){
    QByteArray msg;
    msg = readMessage();
    if(msg == "true"){
        sck->disconnect();
        hide();
        menu = new MenuWindow(this, sck);
        menu->show();
    }
    else if(msg == "false"){
        QMessageBox::information(this, "Informacja", "Użytkownik już istnieje");
        send_username = true;
    }
}
