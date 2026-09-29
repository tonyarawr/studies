#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <errno.h>
#include <netinet/udp.h> 
#include <netinet/ip.h>  
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h> 

#define PACKET_SIZE 8192 

volatile sig_atomic_t quit = 0;
void send_packet(int raw_socket, struct sockaddr_in *sin, 
                 u_int32_t src_addr, u_int32_t dst_addr, 
                 u_int16_t src_port, u_int16_t dst_port, 
                 char *message);

void sighandler(int signo)
{
	if(signo == SIGINT)
		quit=1;
}

int main(int argc, char *argv[])
{

    if (argc != 5) 
    {
        printf("source hostname/IP source port dest hostname/IP dest port");
        printf("Пример: sudo ./client.out 127.0.0.1 5000 (source) 127.0.0.1 6000\n");
        exit(1);
    }

    u_int16_t src_port, dst_port;
    u_int32_t src_addr, dst_addr;
    src_addr = inet_addr(argv[1]);
    dst_addr = inet_addr(argv[3]);
    src_port = atoi(argv[2]);
    dst_port = atoi(argv[4]);

	struct sigaction act;
    act.sa_handler = &sighandler;

	if (-1 == sigaction(SIGINT, &act, 0)) {
		perror("sigaction");
		exit(1);
	}
    

    int raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if(raw_socket < 0)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in sin;
    int one = 1;
    const int *val = &one;

    if(setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) 
    {
        perror("setsockopt() error");
        exit(2);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(dst_port);
    sin.sin_addr.s_addr = dst_addr;

    while(!quit)
    {
        char temp[100];
        memset(temp, 0, sizeof(temp));
        scanf("%99s", temp);

        send_packet(raw_socket, &sin, src_addr, dst_addr, src_port, dst_port, temp);

        char recv_buff[PACKET_SIZE];
        while(!quit)
        {
            memset(recv_buff, 0, PACKET_SIZE);
            int bytes_recv = recvfrom(raw_socket, recv_buff, PACKET_SIZE, 0, NULL, NULL);
            if(bytes_recv == -1)
            {
                if(errno == EINTR)
                {

                    continue; 
                }
                perror("recvfrom");
                continue;
            }

            if(bytes_recv < sizeof(struct iphdr) + sizeof(struct udphdr))
            {
                continue;
            }
     
            struct iphdr *iph = (struct iphdr *)recv_buff;
            if (iph->protocol != 17 || iph->saddr != dst_addr) {
                continue; 
            }

            unsigned short iphdrlen = iph->ihl*4;
            struct udphdr *udph = (struct udphdr *)(recv_buff + iphdrlen);
        
            if (ntohs(udph->source) != dst_port || ntohs(udph->dest) != src_port) {
                continue;
            }

            int header_size = iphdrlen + sizeof(struct udphdr);
            int data_len = bytes_recv - header_size;

            if (data_len > 0)
            {
                printf("Ответ от сервера: ");
                printf("%.*s\n", data_len, recv_buff + header_size);
                break;
            }
        }
    }

    
    send_packet(raw_socket, &sin, src_addr, dst_addr, src_port, dst_port, "quit");
    close(raw_socket);
    return 0;
}

void send_packet(int raw_socket, struct sockaddr_in *sin,
                 u_int32_t src_addr, u_int32_t dst_addr, 
                 u_int16_t src_port, u_int16_t dst_port, 
                 char *message)
{
    char buffer[PACKET_SIZE];
    char *data;
    memset(buffer, 0, PACKET_SIZE);
    struct iphdr *ip = (struct iphdr *) buffer;
    struct udphdr *udp = (struct udphdr *) (buffer + sizeof(struct iphdr));
    data = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

    strcpy(data, message);
    ip->version  = 4;
    ip->ihl      = 5;
    ip->tos      = 16;
    ip->tot_len  = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
    ip->id       = htons(54321);
    ip->ttl      = 64;
    ip->protocol = 17;
    ip->check    = 0;
    ip->saddr = src_addr;
    ip->daddr = dst_addr;

    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons(sizeof(struct udphdr) + strlen(data));
    udp->check = 0; 

        
    if (sendto(raw_socket, buffer, ip->tot_len, 0,
            (struct sockaddr *)sin, sizeof(struct sockaddr_in)) < 0)
    {
        perror("sendto");
        exit(1);
    }
}