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
#include "imsi-catcher.h"

using namespace std;

#define ERROR_STRING "=> error: "

void *stdin_routine(void *arg) {
    extern pthread_mutex_t fdMutex;
    const char *imsi_cmd = "imsi=";

    char line[1024];
    for(; fgets(line, sizeof line, stdin) ;) {
#if  0
        pthread_mutex_lock(&fdMutex);
        for(set<int>::iterator it=fdSet.begin(); it!=fdSet.end(); it++) {
            //printf("fd = %d\n", *it);
            dprintf(*it, "%s\r", line);
        }
        pthread_mutex_unlock(&fdMutex);
#endif
        *strchr(line, '\n') = '\0';
        if(!strncmp(line, imsi_cmd, strlen(imsi_cmd))) {
            /* imsi command */
            pthread_mutex_lock(&imsiMutex);
            char tmp[16];
            strcpy(tmp, imsi_to_catch);
            pthread_mutex_unlock(&imsiMutex);
            if(tmp[0] == '4') {
                fprintf(stderr, ERROR_STRING "busy\n");
            } else if(is_imsi(line + strlen(imsi_cmd))) {
                pthread_mutex_lock(&imsiMutex);
                strcpy(imsi_to_catch, line + strlen(imsi_cmd));
                printf("OK (imsi=%s)\n", imsi_to_catch);
                pthread_mutex_unlock(&imsiMutex);
            } else {
                fprintf(stderr, ERROR_STRING "invalid imsi\n");
            }
        } else {
            fprintf(stderr, ERROR_STRING "invalid command\n");
        }
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

    pthread_t tid2;
    pthread_create(&tid2, NULL, imsi_catcher_thread, NULL);
    pthread_detach(tid2);
    pthread_t tid;
    pthread_create(&tid, NULL, stdin_routine, NULL);
    pthread_detach(tid);
    createTcpServer(atoi(argv[argc-1]), 16, clientRoutine);
    return 0;
}

