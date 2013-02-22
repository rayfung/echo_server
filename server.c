#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <strings.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <netdb.h>

#define BUFFER_SIZE 256

void handler(int num)
{
    exit(0);
}

void setup_sigaction(void)
{
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int udp_server(const char *host, const char *serv)
{
    int n, sockfd;
    struct addrinfo hints, *res, *saved;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    n = getaddrinfo(host, serv, &hints, &res);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }
    saved = res;
    while(res)
    {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(sockfd >= 0)
        {
            if(bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break;
        }
        res = res->ai_next;
    }
    if(res == NULL)
    {
        perror("udp_server");
        sockfd = -1;
    }
    freeaddrinfo(saved);
    return sockfd;
}

void print_address(struct sockaddr *addr, socklen_t len)
{
    int n;
    char ip[NI_MAXHOST], port[NI_MAXSERV];

    n = getnameinfo(addr, len, ip, sizeof(ip), port, sizeof(port),
            NI_DGRAM | NI_NUMERICHOST | NI_NUMERICSERV);
    if(n != 0)
    {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(n));
        return;
    }
    printf("===== [%s]:%s =====\n", ip, port);
}

int main(int argc, char *argv[])
{
    int sockfd;
    ssize_t count;
    char buffer[BUFFER_SIZE];
    char *host, *serv;

    host = "::";
    serv = "webcache";
    switch(argc)
    {
        case 3:
            serv = argv[2];
        case 2:
            host = argv[1];
        case 1:
            break;
        default:
            fprintf(stderr, "usage: %s [<host> [<service>]]\n", argv[0]);
            return 1;
    }

    setup_sigaction();
    sockfd = udp_server(host, serv);
    if(sockfd < 0)
        exit(1);

    while(1)
    {
        struct sockaddr_storage addr;
        socklen_t len;

        len = sizeof(addr);
        count = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&addr, &len);
        if(count < 0)
        {
            perror("recvfrom");
            exit(1);
        }
        else if(count == 0)
            printf("[warning] received nothing.\n");
        else
        {
            print_address((struct sockaddr *)&addr, len);
            printf("[log] receive %d byte(s).\n", (int)count);
            count = sendto(sockfd, buffer, count, 0, (struct sockaddr *)&addr, len);
            printf("[log] send %d byte(s).\n", (int)count);
        }
        printf("\n");
    }
    exit(0);
}
