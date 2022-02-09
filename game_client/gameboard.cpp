#include "gameboard.h"
#include "ui_gameboard.h"
#include <iostream>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <algorithm>
#include <QtEndian>

GameBoard::GameBoard(QDialog *parent, QTcpSocket *sck) :
    QDialog(parent),
    ui(new Ui::GameBoard)
{
    this->sck = sck;
    ui->setupUi(this);
    connect(sck, &QTcpSocket::readyRead, this, &GameBoard::readData);
    connect(this, &QDialog::finished, this, &GameBoard::closedWindow);
    time_wait = new QTime(0, 0, 5);
    time_game = new QTime(0, 3, 0);
    timer = new QTimer(this);
    counter = 5;
    part = 1;
    connect(timer, &QTimer::timeout, [&]{
        counter--;
        time_wait->setHMS(0, counter/60, counter%60);
        ui->my_timer->display(time_wait->toString("mm:ss"));
        if(counter == 0){
            timer->stop();
            if(part == 1){
                counter=play_time + 1;
                timer->start(1000);
                part = 2;
            }
        }
    });
    labelArrays();
}

GameBoard::~GameBoard()
{
    delete ui;
}

QByteArray GameBoard::readMessage(){
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

void GameBoard::closedWindow(){
    QString txt = "end";
    QByteArray msg = (txt).toUtf8();
    char buf[255];
    uint16_t len = sprintf(buf+2, "%s", msg.data());
    *(uint16_t*)buf = qToBigEndian(len);
    sck->write(buf, 2+len);
}

void GameBoard::readData(){
    QByteArray msg = readMessage();
    QString msg_string = QString::fromUtf8(msg).trimmed();
    ui->info_label->setText("");
    QStringList msg_list = msg_string.split(" ");
    if(msg_list[0] == "nicks"){
        int max = msg_list[1].toInt();
        for(int i = 0; i < max; i++){
            nick_labels[i]->setText(msg_list[i+2]);
            points_labels[i]->setText(QString::number(0));
        }
        play_time = msg_list[msg_list.size()-1].toInt();
        ui->my_timer->display(time_wait->toString("mm:ss"));
        timer->start(1000);
    }
    else if(msg_list[0] == "click"){
        int index = msg_list[1].toInt();
        btns_labels[index]->setStyleSheet("border-radius : 25; border : 1px solid black; background-color: cyan;");
    }
    else if(msg_list[0] == "unclick"){
        int unclick_index = msg_list[1].toInt();
        int click_index = msg_list[3].toInt();
        for(int i=0; i<4; i++){
            if(nick_labels[i]->text() == msg_list[4]){
                points[i] += 1;
                points_labels[i]->setText(QString::number(points[i]));
            }
        }
        btns_labels[unclick_index]->setStyleSheet("border-radius : 25; border : 1px solid black; background-color: red;");
        btns_labels[click_index]->setStyleSheet("border-radius : 25; border : 1px solid black; background-color: cyan;");
    }
    else if(msg_list[0] == "win"){
        QString winner;
        if(msg_list.size() > 2) winner = "<p align='center'>Zwycięzcami zostają:<br>";
        else winner = "<p align='center'>Zwycięzcą zostaje:<br>";
        for(int i = 1; i < msg_list.size(); i++){
            if(i < msg_list.size() - 1){
                winner = winner + msg_list[i] + "<br>";
            }
            else{
                winner = winner + msg_list[i] + "</p>";
            }
        }
        timer->stop();
        QMessageBox::information(this, "Koniec gry", winner);
        this->close();
       }
}

void GameBoard::btnClicked(){
    QString txt = sender()->objectName().trimmed();
    QByteArray btn = (txt).toUtf8();

    char buf[255];
    uint16_t len = sprintf(buf+2, "%s", btn.data());
    *(uint16_t*)buf = qToBigEndian(len);
    sck->write(buf, 2+len);
}

void GameBoard::labelArrays(){
    points[0] = 0;
    points[1] = 0;
    points[2] = 0;
    points[3] = 0;
    nick_labels[0] = ui->nickLabel1;
    nick_labels[1] = ui->nickLabel2;
    nick_labels[2] = ui->nickLabel3;
    nick_labels[3] = ui->nickLabel4;
    points_labels[0] = ui->pointsLabel1;
    points_labels[1] = ui->pointsLabel2;
    points_labels[2] = ui->pointsLabel3;
    points_labels[3] = ui->pointsLabel4;
    btns_labels[0] = ui->click1;
    btns_labels[1] = ui->click2;
    btns_labels[2] = ui->click3;
    btns_labels[3] = ui->click4;
    btns_labels[4] = ui->click5;
    btns_labels[5] = ui->click6;
    btns_labels[6] = ui->click7;
    btns_labels[7] = ui->click8;
    btns_labels[8] = ui->click9;
    btns_labels[9] = ui->click10;
    btns_labels[10] = ui->click11;
    btns_labels[11] = ui->click12;
    btns_labels[12] = ui->click13;
    btns_labels[13] = ui->click14;
    btns_labels[14] = ui->click15;
    btns_labels[15] = ui->click16;
    btns_labels[16] = ui->click17;
    btns_labels[17] = ui->click18;
    btns_labels[18] = ui->click19;
    btns_labels[19] = ui->click20;
    btns_labels[20] = ui->click21;
    btns_labels[21] = ui->click22;
    btns_labels[22] = ui->click23;
    btns_labels[23] = ui->click24;
    btns_labels[24] = ui->click25;
    btns_labels[25] = ui->click26;
    btns_labels[26] = ui->click27;
    btns_labels[27] = ui->click28;
    btns_labels[28] = ui->click29;
    btns_labels[29] = ui->click30;
    for(int i = 0; i < 30; i++){
        connect(btns_labels[i], &QPushButton::clicked, this, &GameBoard::btnClicked);
    }
}
