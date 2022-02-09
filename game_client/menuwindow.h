#ifndef MENUWINDOW_H
#define MENUWINDOW_H

#include <QDialog>
#include <QTcpServer>
#include "gameboard.h"

namespace Ui {
class MenuWindow;
}

class MenuWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MenuWindow(QDialog *parent = nullptr, QTcpSocket *sck = nullptr);
    ~MenuWindow();

protected:
    QTcpSocket *sck;
    bool server_closed = true;
    void twoPlayers();
    void threePlayers();
    void fourPlayers();
    void closeWindow();
    void socketDisconnected();
    void errorOcc();

private:
    Ui::MenuWindow *ui;
    GameBoard *gameboard;
};

#endif // MENUWINDOW_H
