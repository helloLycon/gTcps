#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct  {
    int fd;
    struct sockaddr_in addr;
} CliMsg;


int curr_fd(void);

int createTcpServer(unsigned short  listenPort, int cliNum, void * (*start_routine) (void *));

#endif

