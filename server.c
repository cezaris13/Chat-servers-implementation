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

void sendMessage(int socketFd,char* message){
    if (send(socketFd, message, strlen(message), 0) == -1){
        printf("send error\n");
    }
}

int containsUserName(char *searchedString, char *userNames[MAX_USERS]){
    for(int i=0;i<MAX_USERS;i++){
        if(userNames[i]!=NULL&&strcmp(searchedString,userNames[i])==0){
            return 1;
        }
    }
    return 0;
}

void getUserName(int socketFd, int *userCount, char *userNames[MAX_USERS]){
    char atsiuskVarda[]="ATSIUSKVARDA\n\0";
    while(1){
        char bufUserName[MAX_USERS];
        sendMessage(socketFd,atsiuskVarda);
        if (recv(socketFd, bufUserName, sizeof bufUserName, 0) < 0){
            printf("recv error\n");
        }
        else{
            bufUserName[strcspn(bufUserName, "\n")] = 0;
            if(strlen(bufUserName)==0){
                break;
            }
            if(!containsUserName(bufUserName, userNames)&& *userCount <= MAX_USERS){
                /* userNames[socketFd] = malloc(sizeof(char)*(MAX_USERNAME_SIZE+1)); */
                strcpy(userNames[socketFd],bufUserName);
                char vardasOk[] = "VK\n\0";
                (*userCount)++;
                sendMessage(socketFd,vardasOk);
                break;
            }
            else if(*userCount > MAX_USERS){
                break;
            }
        }
    }
}

int startServer(char port[],char ip[]){
    int fdmax, listener, newfd, nbytes;
    char *userNames[MAX_USERS];
    int userCount=0;
    fd_set read_fds, master;
    for(int i=0;i<MAX_USERS;i++){
        userNames[i] = malloc(sizeof(char)*(MAX_USERNAME_SIZE+1));
    }
    struct addrinfo *servInfo, *currInfo, hints;
    struct sockaddr_storage remoteaddr;
    char buf[MAX_SIZE];

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &servInfo) != 0){
        printf("getaddrinfo error\n");
        return -1;
    }

	for(currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next){
        if((listener = socket(currInfo->ai_family, currInfo->ai_socktype, currInfo->ai_protocol)) < 0){
            printf("could not create soctet\n");
			continue;
        }
        int yes=1;
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes))){
            perror("setsockopt\n");
            return -1;
        }
        if(bind(listener,currInfo->ai_addr, currInfo->ai_addrlen) < 0){
			close(listener);
            printf("bind failed\n");
            continue;
        }
        break;
    }
	freeaddrinfo(servInfo);

	if (currInfo == NULL) {
		printf("server: failed to bind\n");
        return -1;
	}

    if(listen(listener, 3) < 0){
        printf("listen failure\n");
        return -1;
    }
	printf("server: waiting for connections...\n");

    FD_SET(listener, &master);

    fdmax = listener;

    while(1){
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
            printf("select error\n");
            return -1;
        }

        for(int i = 0; i <= fdmax; i++){
            if (FD_ISSET(i, &read_fds)){
                if (i == listener){
                    socklen_t addrlen = sizeof remoteaddr;
                    if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1){
                        printf("accept error\n");
                    }
                    else{
                        FD_SET(newfd, &master);
                        if (newfd > fdmax){
                            fdmax = newfd;
                        }
                        getUserName(newfd,&userCount,userNames);
                    }
                }
                else{
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0){
                        if (nbytes == 0){
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else{
                            printf("recv error\n");
                        }
                        close(i);
                        FD_CLR(i, &master);
                        userCount--;
                        /* free(userNames[i]); */
                    }
                    else{
                        for(int j = 0; j <= fdmax; j++){
                            if (FD_ISSET(j, &master)){
                                if (j != listener){
                                    char message[MAX_SIZE]="PRANESIMAS";
                                    strcat(strcat(strcat(strcat(message,userNames[i]),": "),buf),"\0");
                                    printf("%s",message);
                                    sendMessage(j,message);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
	return 0;
}

int main(){
    startServer("20000","127.0.0.1");
}
