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

int init_socket()
{
    struct sockaddr_in addr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        perror("socket");
        exit(1);
    }
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(1);
    }
    return sockfd;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in addr;
    int sockfd;
    ssize_t count;
    char buffer[BUFFER_SIZE];
    socklen_t len;

    setup_sigaction();
    sockfd = init_socket();

    while(1)
    {
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
            printf("===== %s:%d =====\n", inet_ntoa(addr.sin_addr),
                    (int)ntohs(addr.sin_port));
            printf("[log] receive %d byte(s).\n", (int)count);
            len = sizeof(addr);
            count = sendto(sockfd, buffer, count, 0, (struct sockaddr *)&addr, len);
            printf("[log] send %d byte(s).\n", (int)count);
        }
        printf("\n");
    }
    exit(0);
}
