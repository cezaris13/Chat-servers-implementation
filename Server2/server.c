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

char *trimwhitespace(char *str){
  char *end;
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)
      return str;

  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  end[1] = '\0';

  return str;
}

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

/* void receiveFile(int socketFd, char *newFile){ */
/*     FILE *fp; */
/*     fp = fopen(newFile, "w"); */
/*     char *message = malloc(sizeof(char)*MAX_SIZE); */
/*     strcat(strcat(strcpy(message,"GAUTIFAILA"),newFile),"\n\0"); */
/*     sendMessage(socketFd, message); */

/*     char buf[MAX_SIZE]; */
/*     recv(socketFd, buf, sizeof buf, 0); */
/*     while(strstr(buf,"%END%\0")==NULL){ */
/*         printf("%s\n",buf); */
/*         fputs(buf,fp); */
/*         recv(socketFd, buf, sizeof buf, 0); */
/*     } */
/*     fclose(fp); */
/*     free(message); */
/* } */
/* void receiveFileFromServer(int socketFd, char *newFile){ */
/*     FILE *fp; */
/*     fp = fopen(newFile, "w"); */
/*     char buf[MAX_SIZE]; */
/*     recv(socketFd, buf, sizeof buf, 0); */
/*     printf("rev"); */
/*     while(strstr(buf,"%END%\0")==NULL){ */
/*         printf("%s\n",buf); */
/*         fputs(buf,fp); */
/*         recv(socketFd, buf, sizeof buf, 0); */
/*     } */
/*     fclose(fp); */
/* } */

void sendFile(char* filePath, int destSocket){
    FILE *fp;
    fp = fopen(filePath, "r");
    char *message = malloc(sizeof(char)*MAX_SIZE);
    strcpy(message,"");
    strcat(strcat(strcpy(message,"FAILAS"),filePath),"\n\0");
    sendMessage(destSocket, message);
    while(feof(fp) != 1){
        char* buffer = malloc(sizeof(char*) * MAX_SIZE);
        fgets(buffer, MAX_SIZE, fp);
        sendMessage(destSocket, buffer);
        free(buffer);
    }
    char *endOfFile = "%END%\n\0";
    sendMessage(destSocket, endOfFile);
    fclose(fp);
    free(message);
}

void sendFileToServer(char* filePath, int destSocket,int sourceSocket){
    char *message = malloc(sizeof(char)*MAX_SIZE);
    strcpy(message,"");
    strcat(strcat(strcpy(message,"FAILAS"),filePath),"\n\0");
    sendMessage(destSocket, message);
    free(message);
    message = malloc(sizeof(char)*MAX_SIZE);
    strcpy(message,"");
    strcat(strcat(strcpy(message,"GAUTIFAILA"),filePath),"\n\0");
    sendMessage(sourceSocket, message);
    free(message);
}

int initializeSocket(char port[],char ip[], fd_set *master,int *listener,int *fdmax){
    struct addrinfo *servInfo, *currInfo, hints;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &servInfo) != 0){
        printf("getaddrinfo error\n");
        return -1;
    }

	for(currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next){
        if((*listener = socket(currInfo->ai_family, currInfo->ai_socktype, currInfo->ai_protocol)) < 0){
            printf("could not create soctet\n");
			continue;
        }
        int yes=1;
        if (setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes))){
            perror("setsockopt\n");
            return -1;
        }
        if(bind(*listener,currInfo->ai_addr, currInfo->ai_addrlen) < 0){
			close(*listener);
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

    if(listen(*listener, 10) < 0){
        printf("listen failure\n");
        return -1;
    }
	printf("server: waiting for connections...\n");

    FD_SET(*listener, master);

    (*fdmax) =((*fdmax)>(*listener)?(*fdmax):(*listener));
    return 0;
}

int initializeClient(char ip[],char port[]){
    int socketId;
    struct addrinfo *servInfo, *currInfo, hints;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &servInfo) != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }
	for(currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
        if((socketId = socket(currInfo->ai_family, currInfo->ai_socktype, currInfo->ai_protocol)) < 0){
            printf("could not create soctet\n");
            continue;
        }
        if(connect(socketId, currInfo->ai_addr, currInfo->ai_addrlen) < 0){
			close(socketId);
            printf("bind failed\n");
            continue;
        }
        break;
    }

    if (currInfo == NULL) {
		printf("client failed to connect\n");
		return -1;
	}

	freeaddrinfo(servInfo);
    return socketId;
}

int startServer(char ip[],char thisPort[],char otherPort[]){
    int fdmax=-1, listener, newfd, nbytes;
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
    char sth[MAX_SIZE];
    printf("waiting for the other server...");
    scanf("%99[^\n]%*c", sth);
    otherFd = initializeClient(ip,otherPort);
    FD_SET(otherFd, &master);
    if (otherFd > fdmax){
        fdmax = otherFd;
    }
    int firstTime = 1;
    int fileReceiving = 0;
    int fileSending = 0;
    FILE *fp;
    while(1){
    char buf[MAX_SIZE];
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
                        if(firstTime == 1){
                            firstTime = 0;
                            otherFdActive = newfd;
                            FD_SET(newfd, &master);
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
                        userNames[i][0]='\0';
                    }
                    else{
                        if(strstr(buf,"#get")){
                            char *file = buf+4;
                            printf("%s\n",file);
                            sendFile(trimwhitespace(file),i);
                        }
                        else if(strstr(buf,"@")){// minor fixed there and we done
                            char *file = buf+4;
                            char *ff = trimwhitespace(file);
                            /* sendFileToServer(trimwhitespace(file),otherFdActive,i); */
                            char *message = malloc(sizeof(char)*MAX_SIZE);
                            strcpy(message,"");
                            strcat(strcat(strcpy(message,"FAILAS"),ff),"\n\0");
                            sendMessage(otherFdActive, message);
                            free(message);
                            message = malloc(sizeof(char)*MAX_SIZE);
                            strcpy(message,"");
                            strcat(strcat(strcpy(message,"GAUTIFAILA"),ff),"\n\0");
                            sendMessage(i, message);
                            free(message);
                            fileSending=1;
                        }
                        else if(strstr(buf,"FAILAS")){
                            fileReceiving = 1;
                            char *file = buf+6;
                            printf("%s\n",file);
                            fp = fopen(trimwhitespace(file), "w");
                        }
                         else if(strstr(buf,"%END%") && fileReceiving == 1){
                            fileReceiving = 0;
                            fclose(fp);
                            for(int j = 0; j <= fdmax; j++){
                                if (FD_ISSET(j, &master)){
                                    if (j != ourFd && j != otherFd){
                                        char *message = malloc(sizeof(char)*MAX_SIZE);
                                        strcat(strcat(strcpy(message,"Gautas failas: "),""),"\n\0");
                                        printf("%s",message);
                                        sendMessage(j,message);
                                        free(message);
                                    }
                                }
                            }
                        }
                        else if(fileReceiving == 1){
                            /* buf[strcspn(buf, "\n")] = 0; */
                            char *result = (char *)malloc(MAX_SIZE);
                            strcpy(result,buf);

                            fputs(result,fp);
                            printf("receiving %s",result);
                            free(result);
                        }
                        else if(fileSending == 1){
                            /* buf[strcspn(buf, "\n")] = 0; */
                            char *result = (char *)malloc(MAX_SIZE);
                            strcpy(result,buf);
                            printf("sending %s\n",result);
                            sendMessage(otherFdActive,result);
                            free(result);
                        }
                        else if(strstr(buf,"%END%") && fileSending == 1){
                            fileSending = 0;
                        }
                        else if(strstr(buf,"PRANESIMAS") == NULL && fileReceiving == 0 && fileSending == 0){
                            buf[strcspn(buf, "\n")] = 0;
                            for(int j = 0; j <= fdmax; j++){
                                if (FD_ISSET(j, &master)){
                                    if (j != ourFd && j!= otherFd){
                                        printf("%d\n",j);
                                        char *message = malloc(sizeof(char)*MAX_SIZE);// problem with memory 32 crashes
                                        strcat(strcat(strcat(strcat(strcpy(message,"PRANESIMAS"),userNames[i]),": "),buf),"\n\0");
                                        printf("%s",message);
                                        sendMessage(j,message);
                                        free(message);
                                    }
                                }
                            }
                        }
                        buf[0]='\0';
                    }
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
