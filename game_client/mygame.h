#ifndef MYGAME_H
#define MYGAME_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>
#include "menuwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MyGame; }
QT_END_NAMESPACE

class MyGame : public QDialog
{
    Q_OBJECT

public:
    MyGame(QWidget *parent = nullptr, QTcpSocket *sck = nullptr);
    ~MyGame();

protected:
    QTcpSocket *sck;
    QTimer * connTimeoutTimer;
    void connectToServer();
    void socketConnected();
    void confirmUsername();
    void readData();
    void errorOcc();
    QByteArray readMessage();
    bool send_username = true;
    bool error_occ = false;

private:
    Ui::MyGame *ui;
    MenuWindow *menu;
};
#endif // MYGAME_H
