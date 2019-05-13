#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcpServer.h"
#include "cliRoutine.h"
#include <set>
#include <pthread.h>

using namespace std;

void *stdin_routine(void *arg) {
    extern set<int> fdSet;
    extern pthread_mutex_t fdMutex;

    char line[1024];
    for(; fgets(line, sizeof line, stdin) ;) {
        pthread_mutex_lock(&fdMutex);
        for(set<int>::iterator it=fdSet.begin(); it!=fdSet.end(); it++) {
            //printf("fd = %d\n", *it);
            dprintf(*it, "%s\r", line);
        }
        pthread_mutex_unlock(&fdMutex);
    }
    return NULL;
}

int main(int argc, char **argv) {
    extern int quiet;
    if(argc < 2) {
        printf("usage: %s [-q] <port>\n", argv[0]);
        return 0;
    }

    if(strcmp("-q", argv[1])) {
        quiet = 0;
    } else {
        quiet = 1;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, stdin_routine, NULL);
    pthread_detach(tid);
    createTcpServer(atoi(argv[argc-1]), 16, clientRoutine);
    return 0;
}

