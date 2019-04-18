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

void * clientRoutine(void *arg) {
    static int cliNum;
    const CliMsg *pcli = (const CliMsg *)arg;
    CliMsg cli = *pcli;
    FILE *fp = fdopen(cli.fd, "r+");
    char line[1024];
    
    cliNum++;
    printf("%s:%d connected! (%d)\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, cliNum);
    fprintf(fp, "hello~\r\n");
    for(; fgets(line, sizeof line, fp) ;) {
        printf("%s:%d - %s", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, line);
        fprintf(fp, "You Said: %s", line);
    }
    cliNum--;
    printf("%s:%d diconneted! (%d)\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, cliNum);
    pthread_exit(NULL);
}

