#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "./Lib/Functions.h"

#define MAX_SIZE 1024
#define MAX_USERNAME_SIZE 50
#define MAX_USERS 50

int startServer(char ip[], char serverPort[], char otherServerPort[], int isFirstServer) {
    int fdmax, listener, newfd;
    char* userNames[MAX_USERS];
    int userCount = 0;
    fd_set read_fds, master;
    struct sockaddr_storage remoteaddr;

    for (int i = 0; i < MAX_USERS; i++)
        userNames[i] = malloc(sizeof(char) * (MAX_USERNAME_SIZE + 1));

    char* fileName = malloc(sizeof(char) * (MAX_SIZE));
    FILE* fp = NULL;

    initializeSocket(serverPort, ip, &master, &listener, &fdmax);
    int ourFd = listener;
    int otherFd;
    int otherFdActive;
    int firstTime = 1;
    int fileReceiving = 0;
    int fileSending = 0;

    int clientFound = 0;
    while(1){
        read_fds = master;
        if (!clientFound){
            do {
                otherFd = initializeClient(ip, otherServerPort);
                printf("searching for other server\n");
                usleep(1000000);
            } while(otherFd == -1);
            clientFound = 1;
        }

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            printf("select error\n");
            return -1;
        }

        printf("fdmax %d\n", fdmax);
        printf("listener %d\n", listener);
        printf("master %s\n", master);
        for (int i = 0; i <= fdmax; i++) {
            if (!FD_ISSET(i, &read_fds))
                continue;

            if (i != ourFd) {
                char serverId = isFirstServer +'0';
                char serverName[3]= "s";
                strncat(serverName, &serverId, 1);
                handleReceive(i, &userCount, userNames, otherFdActive, &fileSending,
                    &fileReceiving, &fp, otherFd, ourFd, fdmax, &master,
                    &fileName, serverName);
                continue;
            }
            socklen_t addrlen = sizeof remoteaddr;
            if ((newfd = accept(ourFd, (struct sockaddr*)&remoteaddr, &addrlen)) == -1) {
                printf("accept error\n");
                continue;
            }

            if (firstTime != 1) {
                FD_SET(newfd, &master);
                if (newfd > fdmax)
                    fdmax = newfd;

                getUserName(newfd, &userCount, userNames);
                continue;
            }

            firstTime = 0;
            otherFdActive = newfd;
            FD_SET(newfd, &master);

            if (newfd > fdmax)
                fdmax = newfd;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc == 4) {
        char* p;
        long num = strtol(argv[3],&p, 10);
        startServer("127.0.0.1", argv[1], argv[2],num);
    }
    else
        printf("not all parameters are given");
}
