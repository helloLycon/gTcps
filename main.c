#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcpServer.h"
#include "cliRoutine.h"


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
    createTcpServer(atoi(argv[argc-1]), 16, clientRoutine);
    return 0;
}

