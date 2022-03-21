#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#define MAX_SIZE 1024
#define MAX_USERNAME_SIZE 50
#define MAX_USERS 50

#include "../Lib/Functions.h"

int startServer(char ip[],char thisPort[],char otherPort[]){
    int fdmax=-1, listener, newfd;
    char *userNames[MAX_USERS];
    int userCount=0;
    fd_set read_fds, master;
    for(int i=0;i<MAX_USERS;i++){
        userNames[i] = malloc(sizeof(char)*(MAX_USERNAME_SIZE+1));
    }
    struct sockaddr_storage remoteaddr;

    initializeSocket(thisPort,ip,&master,&listener,&fdmax);
    int ourFd = listener;
    int otherFd;
    int otherFdActive;
    int firstTime=1;
    int fileReceiving = 0;
    int fileSending = 0;
    FILE *fp = NULL;
    while(1){
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
            printf("select error\n");
            return -1;
        }

        for(int i = 0; i <= fdmax; i++){
            if (FD_ISSET(i, &read_fds)){
                if (i == ourFd) {
                    socklen_t addrlen = sizeof remoteaddr;
                    if ((newfd = accept(ourFd, (struct sockaddr *)&remoteaddr, &addrlen)) == -1){
                        printf("accept error\n");
                    }
                    else{
                        if(firstTime==1){
                            char sth[MAX_SIZE];
                            firstTime = 0;
                            printf("waiting for the other server...");
                            scanf("%99[^\n]%*c", sth);
                            otherFd = initializeClient(ip,otherPort);
                            FD_SET(newfd, &master);
                            FD_SET(otherFd, &master);
                            otherFdActive  = newfd;
                            if (otherFd > fdmax){
                                fdmax = otherFd;
                            }
                            if (newfd > fdmax){
                                fdmax = newfd;
                            }

                        }
                        else{
                            FD_SET(newfd, &master);
                            if (newfd > fdmax){
                                fdmax = newfd;
                            }
                            getUserName(newfd,&userCount,userNames);
                        }
                    }
                }
                else{
                    HandleReceive(i,&userCount,userNames,otherFdActive,&fileSending,&fileReceiving,&fp,otherFd,ourFd,fdmax, &master);
                }
            }
        }
    }
	return 0;
}

int main(int argc, char *argv[]){
    if(argc == 3){
        startServer("127.0.0.1",argv[1],argv[2]);
    }
    else{
        printf("not all parameters are given");
    }
}
