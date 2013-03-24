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
#include <sys/select.h>
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

int tcp_listen(const char *host, const char *serv, socklen_t *len)
{
    struct addrinfo *res, *saved, hints;
    int n, listenfd;
    const int on = 1;

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    n = getaddrinfo(host, serv, &hints, &res);
    if(n != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(n));
        return -1;
    }
    saved = res;
    while(res)
    {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(listenfd >= 0)
        {
            if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
                perror("setsockopt");
            if(bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            {
                if(listen(listenfd, 128) == 0)
                    break;
            }
            close(listenfd);
        }
        res = res->ai_next;
    }
    if(res == NULL)
    {
        perror("tcp_listen");
        freeaddrinfo(saved);
        return -1;
    }
    else
    {
        if(len)
            *len = res->ai_addrlen;
        freeaddrinfo(saved);
        return listenfd;
    }
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
            NI_NUMERICHOST | NI_NUMERICSERV);
    if(n != 0)
    {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(n));
        return;
    }
    printf("Client [%s]:%s\n", ip, port);
}

int max(int fd1, int fd2)
{
    return (fd1 > fd2 ? fd1 : fd2);
}

int main(int argc, char *argv[])
{
    int tcpfd, udpfd;
    ssize_t count;
    char buffer[BUFFER_SIZE];
    socklen_t len;
    fd_set rset;
    fd_set allset;
    int maxfd;
    char *host, *serv;

    host = "::";
    serv = "8080";
    switch(argc)
    {
        case 3:
            serv = argv[2];
        case 2:
            host = argv[1];
        case 1:
            break;
        default:
            fprintf(stderr, "usage: %s <host_name> <service_name>\n", argv[0]);
            exit(1);
    }

    setup_sigaction();

    tcpfd = tcp_listen(host, serv, NULL);
    if(tcpfd < 0)
        exit(1);

    udpfd = udp_server(host, serv);
    if(udpfd < 0)
        exit(1);

    FD_ZERO(&allset);
    FD_SET(tcpfd, &allset);
    FD_SET(udpfd, &allset);
    maxfd = max(tcpfd, udpfd);

    while(1)
    {
        int n, i;
        struct sockaddr_storage addr;

        rset = allset;
        n = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if(n <= 0)
            continue;

        if(FD_ISSET(udpfd, &rset))
        {
            FD_CLR(udpfd, &rset);
            --n;
            len = sizeof(addr);
            count = recvfrom(udpfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&addr, &len);
            if(count < 0)
                perror("recvfrom");
            else if(count == 0)
                printf("[warning] received nothing.\n");
            else
            {
                printf("[log] receive %d byte(s).\n", (int)count);
                count = sendto(udpfd, buffer, count, 0, (struct sockaddr *)&addr, len);
                printf("[log] send %d byte(s).\n", (int)count);
            }
            printf("\n");
        }
        if(FD_ISSET(tcpfd, &rset))
        {
            int connfd;

            FD_CLR(tcpfd, &rset);
            --n;
            len = sizeof(addr);
            connfd = accept(tcpfd, (struct sockaddr *)&addr, &len);
            if(connfd < 0)
			{
				if(errno != EINTR)
					perror("accept");
			}
            else
            {
                FD_SET(connfd, &allset);
                maxfd = max(maxfd, connfd);
                printf("TCP client #%d accpeted.\n", connfd);
            }
        }
        for(i = 0; i <= maxfd && n > 0; ++i)
        {
            if(FD_ISSET(i, &rset))
            {
                --n;
                count = read(i, buffer, BUFFER_SIZE);
                if(count <= 0)
                {
                    close(i);
                    FD_CLR(i, &allset);
                    printf("#%d closed.\n", i);
                }
                else
                {
                    printf("read %d byte(s)\n", (int)count);
                    count = write(i, buffer, count);
                    printf("write %d byte(s)\n\n", (int)count);
                }
            }
        }
    }
    exit(0);
}
