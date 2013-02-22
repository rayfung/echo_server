#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#define BUFFER_SIZE 256

int udp_connect(const char *host, const char *serv)
{
    struct addrinfo hints, *res, *saved;
    int n, sockfd;

    bzero(&hints, sizeof(hints));
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
            if(connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break;
        }
        res = res->ai_next;
    }
    if(res == NULL)
    {
        perror("udp_connect");
        sockfd = -1;
    }
    freeaddrinfo(saved);
    return sockfd;
}

void input_echo(int sockfd)
{
    ssize_t count;
    char buf[BUFFER_SIZE + 1]; /* "+1" for null byte */

    while(fgets(buf, BUFFER_SIZE, stdin) != NULL)
    {
        count = write(sockfd, buf, strlen(buf));
        if(count < 0)
        {
            perror("write");
            exit(1);
        }
        count = read(sockfd, buf, BUFFER_SIZE);
        if(count < 0)
        {
            perror("read");
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
    char *host, *serv;

    host = "127.0.0.1";
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
            fprintf(stderr, "usage: %s <host> <service>\n", argv[0]);
            return 1;
    }

    sockfd = udp_connect(host, serv);
    if(sockfd < 0)
        exit(1);
    input_echo(sockfd);
    exit(0);
}
