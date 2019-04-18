#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcpServer.h"
#include "cliRoutine.h"


int main(int argc, char **argv) {
    if(argc < 2) {
        printf("usage: %s <port>\n", argv[0]);
        return 0;
    }

    createTcpServer(atoi(argv[1]), 16, clientRoutine);
    return 0;
}

