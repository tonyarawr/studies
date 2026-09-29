#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <fstream> 
#include <thread>
#include "game.hpp"

#define BUFLEN 100
#define MSG_BUF 1024

std::string msg;
struct player_info
{
    bool is_host; 
    char name[BUFLEN];
    char address[16];
    int port;  
};

void recv_msg(int gameSock)
{
    char msg[MSG_BUF];
    while (true){
        int str_len = recv(gameSock, &msg, MSG_BUF + 1, 0);
        if (str_len == -1){
            exit(0);
        }
        std::cout << msg << "\n";
    }
}

void chat(int gameSock, std::string name)
{
    std::thread recv_thread(recv_msg, gameSock);
    recv_thread.detach();

    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        if (msg == "exit") break;

        std::string full_msg = name + ": " + msg;
        send(gameSock, full_msg.c_str(), full_msg.length(), 0);
    }
    return;
}

void updateRatings(std::string name)
{
    std::vector<std::string> lines; 
    std::string line;  
    std::ifstream file("logfile.txt");

    if (!file.is_open()) return;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i] == name) {
            i = i + 1;
            int pos = lines[i].find(":");
            if (pos != std::string::npos)
            {
                int wins = stoi(lines[i].substr(pos + 2)) + 1;
                lines[i] = "Количество побед: " + std::to_string(wins); 
            }
            break;
        }
    }

    std::ofstream outFile("logfile.txt");
    for (const auto& l : lines) {
        outFile << l << "\n";
    }
}


int startGame(int gameSock, struct player_info &data, struct player_info &enemy)
{

    std::string myName (data.name);
    std::string enemyName (enemy.name);
    int winner;
    if (!enemy.is_host) {
        Game game('x', data.name, 'o', enemy.name, gameSock,true);
        winner = game.play();
    } else {
        Game game('x', enemy.name, 'o', data.name, gameSock,false);
        winner = game.play();
    }

    if (winner != 2)
    {
        if (winner == 1 && enemy.is_host || (winner == 0 && !enemy.is_host)) { updateRatings(myName);}   
    }
    return 0; 
}

int send_and_get(int sockServer, struct player_info &enemy, struct player_info &data)
{
    std::string nickname;
    std::cout << "Введите никнейм: "; 
    std::cin >> nickname; 
    strncpy(data.name, nickname.c_str(), BUFLEN - 1);
    data.name[BUFLEN - 1] = '\0';

    struct sockaddr_in myAddr;
    int sockListen;

    if ( (sockListen = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        std::cerr << "Сокет для обмена клиентов не получен";
        exit(EXIT_FAILURE);
    }
    
    std::memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = 0;

    if (bind(sockListen, (struct sockaddr*)&myAddr, sizeof(myAddr)))
    {
        std::cerr << "Проблема с bind в клиенте";
        exit(EXIT_FAILURE);
    }

    unsigned int addrLen = sizeof(myAddr);
    getsockname(sockListen, (struct sockaddr*)&myAddr, &addrLen);
    strcpy(data.address, inet_ntoa(myAddr.sin_addr));
    data.port = ntohs(myAddr.sin_port); 
    send(sockServer, &data, sizeof(data), 0);

    if (recv(sockServer, &enemy,sizeof(enemy),0) < 0) { exit(0);}
    return sockListen;
}


int main(int argc, char* argv[])
{
    int sockServer; 
    struct sockaddr_in servAddr; 
    struct hostent *hp;

    if (argc < 3)
    {
        std::cerr << "Введите имя хоста и порт";
        exit(EXIT_FAILURE);
    }

    if ( (sockServer = socket(AF_INET,SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Сокет не получен";
        exit(EXIT_FAILURE);
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    memmove(hp->h_addr_list[0], &(servAddr.sin_addr.s_addr), hp->h_length);
    servAddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockServer,(struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
    {
        std::cerr << "Не удалось подключиться к серверу";
        exit(EXIT_FAILURE);
    }

    struct player_info data;
    struct player_info enemy;
    int sockListen = send_and_get(sockServer, enemy, data);
    int gameSock; 
    close(sockServer);

    if(enemy.is_host)
    {
        // я -- второй игрок
        sleep(1);
        gameSock = socket(AF_INET,SOCK_STREAM, 0);
        struct sockaddr_in hostAddr;
        memset(&hostAddr, 0, sizeof(hostAddr));
        hostAddr.sin_family = AF_INET;
        hostAddr.sin_port = htons(enemy.port);
        hostAddr.sin_addr.s_addr = inet_addr(enemy.address);
        if (connect(gameSock, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0) {
            std::cerr << "Ошибка подключения к хосту";
            exit(EXIT_FAILURE);
        }
        close(sockListen);
    }
    else
    {
        // я -- первый игрок 
        listen(sockListen,1);
        gameSock = accept(sockListen,0,0);
    }

    char regime;
    do {
    std::cout << "Выберите режим: 1 - игра, 2 - чат\n";
    std::cin >> regime;
    switch(regime)
    {
        case '1':
            startGame(gameSock, data, enemy);
            break;
        case '2':
        {
            std::string name (data.name);
            chat(gameSock, name);
        }
        default:
            std::cout << "Вариантов немного :( \n";
            break;
    }    
    } while (regime != 0);
    close(gameSock);
    return 0;
}