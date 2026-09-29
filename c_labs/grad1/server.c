#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h> 
#include <errno.h>
#include <netinet/udp.h> 
#include <netinet/ip.h>  
#include <arpa/inet.h>
#include <unistd.h>


#define PACKET_SIZE 8192
#define PORT 6000

struct record
{   
    uint32_t ip;
    int port; 
    int value; 
}; 

struct record records[100];
int total_records = 0; 


int main()
{
    int bytes;
    char *data;  
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

    struct sockaddr_in servAddr, clientAddr; 
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(PORT);

    char recv_buffer[PACKET_SIZE]; 
    char send_buffer[PACKET_SIZE];
    while(1)
    {
        int addrLen = sizeof(clientAddr);
        memset(recv_buffer,0,PACKET_SIZE);
        if ((bytes = recvfrom(raw_socket, recv_buffer, PACKET_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen)) < 0 )
        {
            perror("recvfrom");
            exit(1);
        }

        if(bytes < sizeof(struct iphdr) + sizeof(struct udphdr))
        {
            continue;
        }

        struct iphdr *iph = (struct iphdr *)recv_buffer;
        unsigned short iphdrlen = iph->ihl*4;
        struct udphdr *udph = (struct udphdr *)(recv_buffer + iphdrlen);

        if (ntohs(udph->dest) != PORT) {
                continue;
        }

        int header_size = iphdrlen + sizeof(struct udphdr);
        int data_len = bytes - header_size;

        if (data_len > 0)
        {
            printf("%.*s\n", data_len, recv_buffer + header_size);
            char client_ip[addrLen]; 

            u_int32_t source_ip = iph->saddr;
            int source_port = ntohs(udph->source);

            int found = 0;
            int number = 0;  
            for (int i = 0; i < total_records; i++)
            {
                if( records[i].ip == source_ip && records[i].port == source_port)
                {
                    if(data_len >= 4 && !strncmp(recv_buffer + header_size, "quit", 4))
                    {
                        records[i].value = 0;
                    }
                    else
                    {
                        records[i].value++;
                    }
                    number = records[i].value;
                    found = 1;
                    break;  
                }
            }

            if (!found)
            {
                    records[total_records].ip = source_ip;
                    records[total_records].port = source_port;
                    records[total_records].value = 1;
                    number = records[total_records].value;
                    total_records++; 
             
            }

            memset(send_buffer, 0, PACKET_SIZE);
            struct iphdr *ip_send = (struct iphdr *) send_buffer;
            struct udphdr *udp_send = (struct udphdr *) (send_buffer + sizeof(struct iphdr));
            data = send_buffer + sizeof(struct iphdr) + sizeof(struct udphdr);
            char temp[PACKET_SIZE];
            snprintf(temp, sizeof(temp), "%.*s %d", data_len, recv_buffer + header_size, number);
            strcpy(data, temp);

            ip_send->version  = 4;
            ip_send->ihl      = 5;
            ip_send->tos      = 16;
            ip_send->tot_len  = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data);
            ip_send->id       = htons(54321);
            ip_send->ttl      = 64;
            ip_send->protocol = 17;
            ip_send->check    = 0;
            ip_send->saddr = iph->daddr;
            ip_send->daddr = iph->saddr;
            
            udp_send->source = htons(PORT);
            udp_send->dest = htons(source_port);
            udp_send->len = htons(sizeof(struct udphdr) + strlen(data));
            udp_send->check = 0;

            struct sockaddr_in client; 
            client.sin_family = AF_INET;
            client.sin_port = udp_send->dest;
            client.sin_addr.s_addr = ip_send->daddr; 

            if (sendto(raw_socket, send_buffer, ntohs(ip_send->tot_len), 0, (struct sockaddr *)&client, sizeof(client)) < 0) {
                perror("sendto");
            }
        }
    }


    close(raw_socket);
    return 0; 
}