#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#define BUFFER_SIZE 256

void input_echo(int sockfd, struct sockaddr_in serv_addr)
{
    ssize_t count;
    char buf[BUFFER_SIZE + 1]; /* "+1" for null byte */
    socklen_t len;

    len = sizeof(serv_addr);
    if(connect(sockfd, (struct sockaddr *)&serv_addr, len) == -1)
    {
        perror("connect");
        exit(1);
    }
    while(fgets(buf, BUFFER_SIZE, stdin) != NULL)
    {
        count = sendto(sockfd, buf, strlen(buf), 0,
                (struct sockaddr *)&serv_addr, len);
        if(count < 0)
        {
            perror("sendto");
            exit(1);
        }
        count = recvfrom(sockfd, buf, BUFFER_SIZE, 0, NULL, NULL);
        if(count < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        else if(count == 0)
            printf("[warning] 0 byte received.\n");
        else
        {
            buf[count] = '\0';
            printf("%s");
        }
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in addr; /* server address */
    char *ip;
    int port;

    ip = "127.0.0.1";
    port = 8080;
    switch(argc)
    {
        case 3:
            port = atoi(argv[2]);
        case 2:
            ip = argv[1];
        case 1:
            break;
        default:
            fprintf(stderr, "usage: %s <server_ip> <port>\n", argv[0]);
            return 1;
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        fprintf(stderr, "ip address is not valid\n");
        exit(1);
    }
    input_echo(sockfd, addr);
    exit(0);
}
