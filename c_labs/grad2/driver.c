#include <stdio.h> 
#include <stdlib.h>
#include <time.h> 
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>


#define MAX_DRIVERS 100

enum STATUS
{
    GET_STATUS,
    AVAILABLE,
    BUSY
};

struct driver
{
    int driver_id; 
    int current_stat;
    int fd;
};

struct driver drivers[MAX_DRIVERS];
int drivers_count = 0;
int get_status(pid_t target);

void printHelp()
{
    printf("Допустимые команды:\n");
    printf("create_driver\n");
    printf("get_status ID\n");
    printf("send_task ID time\n");
    printf("get_drivers\n");
    printf("exit\n");
}


void perform_task(int socket)
{
    int myID = drivers_count; 
    int task; 
    
    while(1)
    {
        int bytes_read = read(socket, &task, sizeof(task));
        if (bytes_read < 0)
        {
            perror("CHILD:read");
            return; 
        }

        if(task == 0)
        {
            int status = drivers[myID].current_stat;
            write(socket, &status, sizeof(status));
        }
        else
        {
            drivers[myID].current_stat = BUSY;
            fd_set readfd;

            while(task > 0)
            {
                FD_ZERO(&readfd);
                FD_SET(socket, &readfd);

                struct timeval timeout;
                timeout.tv_sec = 1; 
                timeout.tv_usec = 0;

                int ready = select(socket + 1, &readfd, NULL, NULL, &timeout);
                if( ready > 0 && FD_ISSET(socket, &readfd))
                {
                    int req;
                    int nread = read(socket, &req, sizeof(req));
                    if(req == 0)
                    {
                        int status = drivers[myID].current_stat;
                        write(socket, &status, sizeof(status));
                    }
                    continue;
                }

                task--; 
            }
        }

        int done; 
        drivers[myID].current_stat = AVAILABLE;
        write(socket, &done, sizeof(done)); 
    }
}

void create_driver()
{
    int sockets[2]; 
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        perror("socketpair");
        return;
    }

    pid_t proc = fork();
    if (proc < 0)
    {
        perror("fork");
        close(sockets[0]);
        close(sockets[1]);
        return; 
    }
    else if(proc == 0)
    {
        close(sockets[1]);
        drivers[drivers_count].current_stat = AVAILABLE;
        perform_task(sockets[0]); 
        exit(0);
    }
    else
    {
        close(sockets[0]);
        drivers[drivers_count].driver_id = proc;
        drivers[drivers_count].current_stat = AVAILABLE;
        drivers[drivers_count].fd = sockets[1]; 
        drivers_count++;
    }
}

void send_task(pid_t target, int time)
{
    int status = get_status(target);
    for (int i = 0; i < drivers_count; i++) 
    { 
        if(drivers[i].driver_id == target)
        {
            if(status == AVAILABLE)
            {
                write(drivers[i].fd, &time, sizeof(time)); 
            }
            else
            {
                printf("Водитель сейчас занят\n");
                return;
            }
        }

    }
}

void get_drivers()
{
    printf("\n");
    for (int i = 0; i < drivers_count; i++) 
    {
        int status = get_status(drivers[i].driver_id);
        if(status < 0)
        {
            return; 
        }
        printf("  Водитель %d | ID: %d | Статус: %s\n", 
               i, drivers[i].driver_id, 
            status == AVAILABLE ? "AVAILABLE" : "BUSY");
    }
    printf("\n");
}

int get_status(pid_t target)
{
    for (int i = 0; i < drivers_count; i++) 
    { 
        if(drivers[i].driver_id == target)
        {
            int command = GET_STATUS;  
            write(drivers[i].fd, &command, sizeof(command));
            int response; 
            read(drivers[i].fd, &response, sizeof(response));
            return response;
        }
    }

    printf("Водитель не найден\n");
    return -1; 
}

int main()
{
    char choice[50];
    printHelp();
    fd_set mainfd;
    int maxfd;  
    
    while(1)
    {
        FD_ZERO(&mainfd);
        FD_SET(fileno(stdin),&mainfd);
        maxfd = fileno(stdin);
        for (int i = 0; i < drivers_count; i++)
        {
            FD_SET(drivers[i].fd, &mainfd);
            if (drivers[i].fd > maxfd)
            {
                maxfd = drivers[i].fd;
            } 
        }

        if (select(maxfd + 1, &mainfd, NULL, NULL, NULL) < 0)
        {
            perror("select");
            break; 
        }

        if(FD_ISSET(fileno(stdin),&mainfd))
        {
            if (fgets(choice, sizeof(choice), stdin) == NULL)
            {
                perror("PARENT:fgets");
                break; 
            }

            choice[strcspn(choice, "\r\n")] = 0;
            if (strlen(choice) == 0) {continue;}

            char command[16];
            memset(command, 0, sizeof(command));
            pid_t target; 
            int time; 
            
            sscanf(choice, "%s %d %d", command, &target, &time);
            if(!strcmp(command,"create_driver"))
            {
                create_driver();
            }
            else if(!strcmp(command,"send_task"))
            {
                send_task(target, time);
            }
            else if(!strcmp(command, "get_status"))
            {
                int respond = get_status(target);
                printf("Текущий статус: ");
                printf("%s\n", respond == AVAILABLE ? "AVAILABLE" : "BUSY");
            }
            else if(!strcmp(command, "get_drivers"))
            {
                get_drivers();
            }
            else if(!strcmp(command, "exit"))
            {
                for(int i = 0; i < drivers_count; i++)
                {
                    kill(drivers[i].driver_id, SIGKILL);
                }
                return 0; 
            }
            else
            {
                printf("Неверная команда\n"); 
                printHelp(); 
            }
        }

        for (int n = 0; n < drivers_count; n++)
        {
            if(FD_ISSET(drivers[n].fd, &mainfd))
            {
                int response;
                int bytes = read(drivers[n].fd, &response, sizeof(response));
                if (bytes <= 0)
                {
                    perror("PARENT:read");
                    return 0; 
                }

                
                drivers[n].current_stat = response; 
            }
        }
    }

    return 0; 
}