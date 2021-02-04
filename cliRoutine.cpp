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

using namespace std;

set<int> fdSet;
pthread_mutex_t fdMutex = PTHREAD_MUTEX_INITIALIZER;

void * clientRoutine(void *arg) {
    static long long packetCnter = 0;
    extern int quiet;
    const CliMsg *pcli = (const CliMsg *)arg;
    CliMsg cli = *pcli;
    FILE *fp = fdopen(cli.fd, "r+");
    char line[10240];

    pthread_mutex_lock(&fdMutex);
    fdSet.insert(cli.fd);
    printf("%s:%d connected! (%d)\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, fdSet.size());
    pthread_mutex_unlock(&fdMutex);
    fprintf(fp, "hello~\r\n");
    for(cli.msgCount=0; fread(line, sizeof line,1, fp) ;) {
        cli.msgCount++;
        packetCnter++;
        if(0 == (packetCnter % 10000)) {
            printf("%ld received\n", packetCnter);
        }
        /*printf("%s:%d - [%d]%s", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, cli.msgCount, line);
        if(!quiet) {
            fprintf(fp, "You Said: %s", line);
        }*/
    }

quit:
    pthread_mutex_lock(&fdMutex);
    fdSet.erase(cli.fd);
    printf("%s:%d disconnected! (%d)\n", inet_ntoa(cli.addr.sin_addr), cli.addr.sin_port, fdSet.size());
    pthread_mutex_unlock(&fdMutex);
    fclose(fp);
    pthread_exit(NULL);
}

