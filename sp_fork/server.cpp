#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <fstream> 

#define BUFLEN 100

struct player_info
{
    bool is_host; 
    char name[BUFLEN];
    char address[16];
    int port;  
};

void addPlayer(char* name) 
{   
    std::string line; 
    bool found = false; 
    std::ifstream file("logfile.txt");
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line == name) {
                found = true;
                break;
            }
        }
        file.close();
    }

    if(!found)
    {
        std::ofstream log("logfile.txt", std::ios_base::app);
        if (log.is_open()) {
            log << name << "\n";
            log << "Количество побед: 0\n";
            log << "+---+---+---+---+---+\n";
        }
    }
    std::cout << "Игрок " << name << " подключился\n";
}

void reaper(int sig) {
    int status;
    while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

int connectPlayers(int sockP1, int sockP2)
{
    struct player_info player1, player2;
    
    if (recv(sockP1, &player1,sizeof(player1),0) < 0) { exit(0);}
    addPlayer(player1.name);
    if (recv(sockP2, &player2,sizeof(player2),0) < 0) { exit(0);}
    addPlayer(player2.name);

    player1.is_host = true;
    player2.is_host = false;

    send(sockP1, &player2,sizeof(player2),0);
    send(sockP2, &player1,sizeof(player1),0);

    close(sockP1);
    close(sockP2);
    exit(0);
}

int main()
{
    int sockMain, sockClient;
    struct sockaddr_in servAddr; 

    if ( (sockMain = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        std::cerr << "Сервер не может открыть главный сокет";
        exit(EXIT_FAILURE);
    }

    std::memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = 0;

    if (bind(sockMain, (struct sockaddr*)&servAddr, sizeof(servAddr)))
    {
        std::cerr << "Связывание сервера неудачно";
        exit(EXIT_FAILURE);
    }

    unsigned int addrLen = sizeof(servAddr);
    if (getsockname(sockMain, (struct sockaddr*)&servAddr, &addrLen)) 
    {
        std::cerr << "Вызов getsockname неудачен";
        exit(EXIT_FAILURE);
    }

    std::cout << "SERVER: номер порта:" << ntohs(servAddr.sin_port) << "\n";
    listen(sockMain,5);

    signal(SIGCHLD, reaper);

    int sockPlayer1;
    bool is_waiting = false; 
    while(true)
    {
        if( (sockClient = accept(sockMain, 0,0)) < 0)
        {
            std::cerr << "Неверный сокет для клиента";
            exit(EXIT_FAILURE);
        }

        if(!is_waiting)
        {
            sockPlayer1 = sockClient;
            is_waiting = true; 
        }
        else
        {
            int sockPlayer2 = sockClient;
            std::cout << "Пара найдена! Соединяем игроков..." << "\n";    
            if (fork() == 0)
            {
                close(sockMain);
                connectPlayers(sockPlayer1,sockPlayer2);
            }   
            close(sockPlayer1);
            close(sockPlayer2);
            is_waiting = false;      
        }
    } 
}