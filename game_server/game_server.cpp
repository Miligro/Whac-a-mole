#include <thread>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <vector>
#include <algorithm>
#include <map>
#include <iostream>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <set>
#include <fstream>
#include <random>
#include <chrono>
#include <sstream>
#include <sys/eventfd.h>
#include <mutex>

std::map<int, std::string> users;
std::map<int, std::set<int>> playersGame;
int sck_server;
int epollfd_set_username = epoll_create1(0);
int epollfd_game_mode = epoll_create1(0);
int epollfd_waiting = epoll_create1(0);
epoll_event event_waiting;
epoll_event event_set_username;
epoll_event event_game_mode;
int stopUsername, stopWaiting, stopGameMode, stopAccept;
int port;
int seconds;
bool isRuning = true;

std::mutex usersLock;
std::mutex playersGameLock;

ssize_t readData(int socket, char* buf){
    uint16_t msgSize;
    if (recv(socket, &msgSize, 2, MSG_WAITALL) != 2)
        return 0;

    msgSize = ntohs(msgSize);
    if (recv(socket, buf, msgSize, MSG_WAITALL) != msgSize)
        return 0;

    return msgSize;
}

void close_server(int signal){
    std::cout<<"Close server, signal: "<<signal<<std::endl;
    isRuning = false;
    eventfd_write(stopUsername, 1);
    eventfd_write(stopWaiting, 1);
    eventfd_write(stopGameMode, 1);
    eventfd_write(stopAccept, 1);
}

void changeEpoll(epoll_event del_event, epoll_event add_event, int del_fd, int add_fd, int socket){
    del_event.events = EPOLLIN|EPOLLET;
    del_event.data.fd = socket;
    epoll_ctl(del_fd, EPOLL_CTL_DEL, socket, &del_event);
    add_event.events = EPOLLIN|EPOLLET;
    add_event.data.fd = socket;
    epoll_ctl(add_fd, EPOLL_CTL_ADD, socket, &add_event);
}

void client_disconnected(int client_socket){
    users.erase(client_socket);
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
}

void acceptClients(){
    int epollfd_accept = epoll_create1(0);
    epoll_event event_accept;
    event_accept.events = EPOLLIN|EPOLLET;
    event_accept.data.fd = sck_server;
    epoll_ctl(epollfd_accept, EPOLL_CTL_ADD, sck_server, &event_accept);

    stopAccept = eventfd(0, 0);
    if(stopAccept == -1) perror("stopAccept");
    event_accept.events = EPOLLIN|EPOLLET;
    event_accept.data.fd = stopAccept;
    epoll_ctl(epollfd_accept, EPOLL_CTL_ADD, stopAccept, &event_accept);

    if(listen(sck_server, 10) < 0){
        perror("listen");
        close_server(9);
    }

    while(isRuning){
        if(epoll_wait(epollfd_accept, &event_accept, 1, -1) < 0){
            perror("epollfd_accept");
            break;
        }
        if(event_accept.events & EPOLLIN && event_accept.data.fd == sck_server){
            int sock_in;
            sock_in = accept(sck_server, NULL, NULL);
            if (sock_in != -1){
                users[sock_in] = "";
                event_set_username.events = EPOLLIN|EPOLLET;
                event_set_username.data.fd = sock_in;
                epoll_ctl(epollfd_set_username, EPOLL_CTL_ADD, sock_in, &event_set_username);
            }
            else{
                perror("Accept returned -1");
            }
        }
        else{
            std::cout<<"Unexpected event on epoll accept: "<<event_accept.events<<" on socket: "<<event_accept.data.fd<<std::endl;
        }
    }
}

void removeFromGame(int sck){
    playersGame[2].erase(sck);
    playersGame[3].erase(sck);
    playersGame[4].erase(sck);
}

void waitingForOpponent(){
    int sck;
    ssize_t received;
    while(isRuning){
        if(epoll_wait(epollfd_waiting, &event_waiting, 1, -1) < 0){
            perror("epollfd_waiting");
            break;
        }
        if(event_waiting.events & EPOLLIN){
            sck = event_waiting.data.fd;
            if(stopWaiting == sck){
                break;
            }
            char buf2[255]{};
            received = readData(sck, buf2);
            if(received == 0){
                std::unique_lock<std::mutex> lock(playersGameLock);
                removeFromGame(sck);
                std::unique_lock<std::mutex> lock1(usersLock);
                client_disconnected(sck);
            }
            else if(strcmp(buf2, "end") == 0){
                std::unique_lock<std::mutex> lock(playersGameLock);
                removeFromGame(sck);
                changeEpoll(event_waiting, event_game_mode, epollfd_waiting, epollfd_game_mode, sck);
            }
            else{
                std::cout<<"Message in waiting room from user "<<users[sck]<<" : "<<buf2<<std::endl;
            }
        }
        else{
            std::cout<<"Unexpected event on epoll waiting: "<<event_waiting.events<<" on socket: "<<event_waiting.data.fd<<std::endl;
        }
    }
}

void readDataUsername(){
    while(isRuning){
        if(epoll_wait(epollfd_set_username, &event_set_username, 1, -1)<0){
            perror("epollfd_set_username");
            break;
        }
        if(event_set_username.events & EPOLLIN){
            bool contain = false;
            int sck = event_set_username.data.fd;
            if(stopUsername == sck){
                break;
            }
            ssize_t received;
            uint16_t len;
            char buf[255]{};
            char write_buf[255]{};
            received = readData(sck, buf);
            if(received == 0) {
                std::unique_lock<std::mutex> lock(usersLock);
                client_disconnected(sck);
            }

            contain = (users.end() != std::find_if(users.cbegin(), users.cend(), [buf](auto kv){
                return !(bool)strcmp(kv.second.c_str(), buf);}));

            if(!contain){
                users[sck] = buf;

                len = sprintf(write_buf+2, "true");
                *(uint16_t*)write_buf = htons(len);
                if (2+len != write(sck, write_buf, 2+len)){
                    std::unique_lock<std::mutex> lock(usersLock);
                    client_disconnected(sck);
                }
                changeEpoll(event_set_username, event_game_mode, epollfd_set_username, epollfd_game_mode, sck);
            }
            else{
                len = sprintf(write_buf+2, "false");
                *(uint16_t*)write_buf = htons(len);
                if (2+len != write(sck, write_buf, 2+len)) {
                    std::unique_lock<std::mutex> lock(usersLock);
                    client_disconnected(sck);
                }
            }
        }
        else{
            std::cout<<"Unexpected event on epoll set username: "<<event_set_username.events<<" on socket: "<<event_set_username.data.fd<<std::endl;
        }
    }
}

int lastPlayerWin(int sck){
    char buf[255];
    uint16_t len = sprintf(buf+2, "win %s", users[sck].c_str());
    *(uint16_t*)buf = htons(len);
    if (2+len != write(sck, buf, 2+len)){
        std::unique_lock<std::mutex> lock(usersLock);
        client_disconnected(sck);
        return 1;
    }
    return 0;
}

void gameFunction(std::set<int> sockets, int playersNum){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0,29);
    std::map<int, int> points;
    int epollfd_game = epoll_create1(0);
    std::set<int> sockets_d;
    sockets_d = sockets;
    epoll_event event_game;
    int id;
    int id2;
    int sck;
    char buf_write[255]{};
    char buf_win[512]{};
    ssize_t received;
    uint16_t len;
    for (auto value: sockets){
        changeEpoll(event_waiting, event_game, epollfd_waiting, epollfd_game, value);
    }
    
    if (playersNum == 2){
        len = sprintf(buf_write+2, "nicks 2 %s %s %d", users[*std::next(sockets.begin(), 0)].c_str(), users[*std::next(sockets.begin(), 1)].c_str(), seconds);
        points[*std::next(sockets.begin(), 0)] = 0;
        points[*std::next(sockets.begin(), 1)] = 0;
    }
    else if(playersNum == 3){
        len = sprintf(buf_write+2, "nicks 3 %s %s %s %d", users[*std::next(sockets.begin(), 0)].c_str(), users[*std::next(sockets.begin(), 1)].c_str(), users[*std::next(sockets.begin(), 2)].c_str(), seconds);
        points[*std::next(sockets.begin(), 0)] = 0;
        points[*std::next(sockets.begin(), 1)] = 0;
        points[*std::next(sockets.begin(), 2)] = 0;
    }
    else{
        len = sprintf(buf_write+2, "nicks 4 %s %s %s %s %d", users[*std::next(sockets.begin(), 0)].c_str(), users[*std::next(sockets.begin(), 1)].c_str(), users[*std::next(sockets.begin(), 2)].c_str(), users[*std::next(sockets.begin(), 3)].c_str(), seconds); 
        points[*std::next(sockets.begin(), 0)] = 0;
        points[*std::next(sockets.begin(), 1)] = 0;
        points[*std::next(sockets.begin(), 2)] = 0;
        points[*std::next(sockets.begin(), 3)] = 0;
    }

    *(uint16_t*)buf_write = htons(len);
    for (auto value: sockets){
        if (2+len != write(value, buf_write, 2+len)){
            std::unique_lock<std::mutex> lock(usersLock);
            client_disconnected(sck);
            sockets_d.erase(sck);
            if(sockets_d.size() == 1){
                sck = *std::next(sockets_d.begin(), 0);
                if(lastPlayerWin(sck) == 0)
                    changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                close(epollfd_game);
                return;
            }
        }
    }
    auto start = std::chrono::steady_clock::now();
    while(isRuning){
        if(epoll_wait(epollfd_game, &event_game, 1, (6000-(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()))) < 0){
            perror("epollfd_game");
            break;
        }
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start).count() >= 6) {
            break;
        }
        if(event_game.events & EPOLLIN){
            sck = event_game.data.fd;
            char buf2[255]{};
            received = readData(sck, buf2);
            if(received == 0){
                std::unique_lock<std::mutex> lock(usersLock);
                client_disconnected(sck);
                sockets_d.erase(sck);
                if(sockets_d.size() == 1){
                    sck = *std::next(sockets_d.begin(), 0);
                    if(lastPlayerWin(sck) == 0)
                        changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                    close(epollfd_game);
                    return;
                }
            }else if(strcmp(buf2, "end") == 0){
                changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                sockets_d.erase(sck);
                if(sockets_d.size() == 1){
                    sck = *std::next(sockets_d.begin(), 0);
                    if(lastPlayerWin(sck) == 0)
                        changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                    close(epollfd_game);
                    return;
                }
            }
        }
        else{
            std::cout<<"Unexpected event on epoll game: "<<event_game.events<<" on socket: "<<event_game.data.fd<<std::endl;
        }
    }
    id = dist(gen);
    len = sprintf(buf_write+2, "click %d", id);
     *(uint16_t*)buf_write = htons(len);
    for (auto value: sockets_d){
        if (2+len != write(value, buf_write, 2+len)){
            std::unique_lock<std::mutex> lock(usersLock);
            client_disconnected(sck);
            sockets_d.erase(sck);
            if(sockets_d.size() == 1){
                sck = *std::next(sockets_d.begin(), 0);
                if(lastPlayerWin(sck) == 0)
                    changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                close(epollfd_game);
                return;
            }
        }
    }
    start = std::chrono::steady_clock::now();
    while(isRuning){
        if(epoll_wait(epollfd_game, &event_game, 1, ((seconds+1)*1000-(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()))) < 0){
            perror("epollfd_game");
            break;
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count() >= seconds*1000) {
            break;
        }
        snprintf(buf_write, 255, "click%d", id + 1);
        if(event_game.events & EPOLLIN){
            sck = event_game.data.fd;
            char buff[255]{};
            received = readData(sck, buff);
            if(received == 0){
                std::unique_lock<std::mutex> lock(usersLock);
                client_disconnected(sck);
                sockets_d.erase(sck);
                if(sockets_d.size() == 1){
                    sck = *std::next(sockets_d.begin(), 0);
                    if(lastPlayerWin(sck) == 0)
                        changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                    close(epollfd_game);
                    return;
                }
            }
            if(strcmp(buff, buf_write) == 0){
                id2 = dist(gen);
                len = sprintf(buf_write+2, "unclick %d click %d %s", id, id2, users[sck].c_str());
                points[sck] += 1;
                id = id2;
                *(uint16_t*)buf_write = htons(len);
                for (auto value: sockets_d){
                    if (2+len != write(value, buf_write, 2+len)){
                        std::unique_lock<std::mutex> lock(usersLock);
                        client_disconnected(sck);
                        sockets_d.erase(sck);
                        if(sockets_d.size() == 1){
                            sck = *std::next(sockets_d.begin(), 0);
                            if(lastPlayerWin(sck) == 0)
                                changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                            close(epollfd_game);
                            return;
                        }
                    }
                }
            }else if(strcmp(buff, "end") == 0){
                changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                sockets_d.erase(sck);
                if(sockets_d.size() == 1){
                    sck = *std::next(sockets_d.begin(), 0);
                    if(lastPlayerWin(sck) == 0)
                        changeEpoll(event_game, event_game_mode, epollfd_game, epollfd_game_mode, sck);
                    close(epollfd_game);
                    return;
                }
            }
        }
        else{
            std::cout<<"Unexpected event on epoll game: "<<event_game.events<<" on socket: "<<event_game.data.fd<<std::endl;
        }
    }

    auto x = std::max_element(points.begin(), points.end(),
    [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
        return p1.second < p2.second; })->second;

    sprintf(buf_write, "win");   
    for(auto v : points){
        if(v.second == x){
            strcat(buf_write, " ");
            strcat(buf_write, users[v.first].c_str());
        }
    }
    len = sprintf(buf_win+2, "%s", buf_write);
    *(uint16_t*)buf_win = htons(len);
    for (auto value: sockets_d){
        if (2+len != write(value, buf_win, 2+len)){
            std::unique_lock<std::mutex> lock(usersLock);
            client_disconnected(sck);
            sockets_d.erase(sck);
        }
    }

    for (auto value: sockets_d){
        event_game_mode.events = EPOLLIN|EPOLLET;
        event_game_mode.data.fd = value;
        epoll_ctl(epollfd_game_mode, EPOLL_CTL_ADD, value, &event_game_mode);
    }
    close(epollfd_game);
}

void addPlayer(long unsigned int playersNum, int socket){
    std::set<int> game = playersGame[playersNum];
    game.insert(socket);
    playersGame[playersNum] = game;
    changeEpoll(event_game_mode, event_waiting, epollfd_game_mode, epollfd_waiting, socket);
    if(game.size() == playersNum){
        std::thread(gameFunction, game, playersNum).detach();
        game.clear();
        playersGame[playersNum] = game;
    }
}

void readDataGameMode(){
    while(isRuning){
        if(epoll_wait(epollfd_game_mode, &event_game_mode, 1, -1) < 0){
            perror("epollfd_game_mode");
            break;
        }
        if(event_game_mode.events & EPOLLIN){
            int sck = event_game_mode.data.fd;
            if(stopGameMode == sck){
                break;
            }
            ssize_t received;
            char buf[255]{};
            received = readData(sck, buf);
            if(received == 0){
                std::unique_lock<std::mutex> lock(usersLock);
                client_disconnected(sck);
            }
            if(strcmp(buf, "two") == 0){
                addPlayer(2, sck);
            }
            else if(strcmp(buf, "three") == 0){
                addPlayer(3, sck);
            }
            else if(strcmp(buf, "four") == 0){
                addPlayer(4, sck);
            }
        }
        else{
            std::cout<<"Unexpected event on epoll game mode: "<<event_game_mode.events<<" on socket: "<<event_game_mode.data.fd<<std::endl;
        }
    }
}

void readConfig(){
    std::ifstream configFile("config.txt");
    if(configFile.is_open()){
        std::string line;
        while(getline(configFile, line)){
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            if(strcmp(name.c_str(), "port") == 0){
                port = std::stoi(value);
            }
            else if(strcmp(name.c_str(), "time_per_game") == 0){
                seconds = std::stoi(value);
            }
        }
    }
}

int main(){
    readConfig();
    signal(SIGINT, close_server);
    sck_server = socket(AF_INET, SOCK_STREAM, 0);
    if(sck_server == -1){
        error(1, errno, "socket failed");
    }
    
    sockaddr_in serv;
    serv.sin_family=AF_INET;
    serv.sin_addr.s_addr=htonl(INADDR_ANY);
    serv.sin_port=htons(port);

    int fd = bind(sck_server, (sockaddr*)&serv, sizeof(serv));
    if(fd){
        perror("bind");
        return 1;
    }

    stopWaiting = eventfd(0, 0);
    if(stopWaiting == -1) perror("stopWaiting");
    event_waiting.events = EPOLLIN|EPOLLET;
    event_waiting.data.fd = stopWaiting;
    epoll_ctl(epollfd_waiting, EPOLL_CTL_ADD, stopWaiting, &event_waiting);

    stopUsername = eventfd(0, 0);
    if(stopUsername == -1) perror("stopUsername");
    event_set_username.events = EPOLLIN|EPOLLET;
    event_set_username.data.fd = stopUsername;
    epoll_ctl(epollfd_set_username, EPOLL_CTL_ADD, stopUsername, &event_set_username);

    stopGameMode= eventfd(0, 0);
    if(stopGameMode == -1) perror("stopGameMode");
    event_game_mode.events = EPOLLIN|EPOLLET;
    event_game_mode.data.fd = stopGameMode;
    epoll_ctl(epollfd_game_mode, EPOLL_CTL_ADD, stopGameMode, &event_game_mode);

    std::thread acceptfun(acceptClients);
    std::thread waitingfun(waitingForOpponent);
    std::thread readfun(readDataUsername);
    std::thread readgamemode(readDataGameMode);

    acceptfun.join();
    readfun.join();
    waitingfun.join();
    readgamemode.join();

    for (auto const& x : users){
        shutdown(x.first, SHUT_RDWR);
        close(x.first);
    }
    close(epollfd_game_mode);
    close(epollfd_set_username);
    close(epollfd_waiting);
    close(stopUsername);
    close(stopWaiting);
    close(stopGameMode);
    close(stopAccept);
    close(sck_server);
    return 0;
}
