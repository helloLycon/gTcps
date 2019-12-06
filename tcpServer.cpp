#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <pthread.h>
#include "tcpServer.h"

int quiet;
int servSockFd;

int createTcpServer(unsigned short listenPort, int cliNum, void * (*start_routine) (void *)) {
    servSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(servSockFd < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(listenPort);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(servSockFd, (struct sockaddr *) &servAddr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("receive messages on %d ...\n", listenPort);
    for(;;) {
        socklen_t cliAddrLen = sizeof (struct sockaddr_in);
        CliMsg cli;
        char buf[1024];
        ssize_t sz = recvfrom(servSockFd, buf, sizeof(buf)-1, 0, (struct sockaddr *)&cli.addr, &cliAddrLen);
        if(sz > 0) {
            buf[sz] = '\0';
            printf("%s:%d - %s", inet_ntoa(cli.addr.sin_addr), ntohs(cli.addr.sin_port), buf);
            int end = sz>3?3:sz;
            buf[end] = '\r';
            buf[end+1] = '\n';
            buf[end+2] = 0;
            sendto(servSockFd, buf, strlen(buf), 0, (struct sockaddr *)&cli.addr, sizeof(cli.addr));
        }
    }
}

