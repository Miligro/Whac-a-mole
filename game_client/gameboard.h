#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include <QDialog>
#include <QLabel>
#include <QTcpSocket>

namespace Ui {
class GameBoard;
}

class GameBoard : public QDialog
{
    Q_OBJECT

public:
    explicit GameBoard(QDialog *parent = nullptr, QTcpSocket *sck = nullptr);
    ~GameBoard();

protected:
    int counter;
    int play_time;
    int part;
    int points[4];
    QLabel* nick_labels[4];
    QLabel* points_labels[4];
    QPushButton* btns_labels[30];
    QTcpSocket *sck;
    QTimer* timer;
    QTime* time_wait;
    QTime* time_game;
    void readData();
    void labelArrays();
    void btnClicked();
    void closedWindow();
    QByteArray readMessage();

private:
    Ui::GameBoard *ui;
};

#endif // GAMEBOARD_H
