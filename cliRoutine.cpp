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
#include <set>
#include "imsi-catcher.h"

using namespace std;

pthread_mutex_t fdMutex = PTHREAD_MUTEX_INITIALIZER;

void * clientRoutine(void *arg) {
    extern int quiet;
    const CliMsg *pcli = (const CliMsg *)arg;
    CliMsg cli = *pcli;
    //FILE *fp = fdopen(cli.fd, "r+");
    char line[1024];


#if  0
    fprintf(fp, "hello~\r\n");
    for(cli.msgCount=0; fgets(line, sizeof line, fp) ;) {
        if( !strncmp("quit", line, 4) ) {
            fprintf(fp, "\r\nSee you~\r\n");
            goto quit;
        }
        cli.msgCount++;
        printf("%s:%d - [%d]%s", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, cli.msgCount, line);
        if(!quiet) {
            fprintf(fp, "You Said: %s", line);
        }
    }
#endif
    imsi_catcher_routine(cli.fd, "460023587111111");

quit:
    pthread_mutex_lock(&fdMutex);
    printf("%s:%d disconnected!\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port);
    pthread_mutex_unlock(&fdMutex);
    close(cli.fd);
    pthread_exit(NULL);
}

