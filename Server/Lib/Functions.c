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

#define MAX_USERS 50
#define MAX_SIZE 1024

char* strremove(char* str, const char* sub)
{
    size_t len = strlen(sub);
    if (len > 0) {
        char* p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

char* trimwhitespace(char* str)
{
    char* end;
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return str;
}

void sendMessage(int socketFd, char* message)
{
    if (send(socketFd, message, strlen(message), 0) == -1) {
        printf("send error\n");
    }
}

int containsUserName(char* searchedString, char* userNames[MAX_USERS])
{
    for (int i = 0; i < MAX_USERS; i++) {
        if (userNames[i] != NULL && strcmp(searchedString, userNames[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void getUserName(int socketFd, int* userCount, char* userNames[MAX_USERS])
{
    char atsiuskVarda[] = "SENDNAME\n\0";
    while (1) {
        char bufUserName[MAX_USERS];
        sendMessage(socketFd, atsiuskVarda);
        if (recv(socketFd, bufUserName, sizeof bufUserName, 0) < 0) {
            printf("recv error\n");
        } else {
            bufUserName[strcspn(bufUserName, "\n")] = 0;
            if (strlen(bufUserName) == 0) {
                break;
            }
            if (!containsUserName(bufUserName, userNames) && *userCount <= MAX_USERS) {
                strcpy(userNames[socketFd], bufUserName);
                char vardasOk[] = "VK\n\0";
                (*userCount)++;
                sendMessage(socketFd, vardasOk);
                break;
            } else if (*userCount > MAX_USERS) {
                break;
            }
        }
    }
}

void sendFile(char* filePath, int destSocket)
{
    FILE* fp;
    fp = fopen(filePath, "r");
    char* message = malloc(sizeof(char) * MAX_SIZE);
    strcpy(message, "");
    strcat(strcat(strcpy(message, "FILE"), filePath), "\n\0");
    sendMessage(destSocket, message);
    while (feof(fp) != 1) {
        char* buffer = malloc(sizeof(char*) * MAX_SIZE);
        fgets(buffer, MAX_SIZE, fp);
        sendMessage(destSocket, buffer);
        free(buffer);
    }
    char* endOfFile = "%END%\n\0";
    sendMessage(destSocket, endOfFile);
    fclose(fp);
    free(message);
}

int initializeSocket(char port[], char ip[], fd_set* master, int* listener,
    int* fdmax)
{
    struct addrinfo *servInfo, *currInfo, hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(ip, port, &hints, &servInfo) != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }

    for (currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
        if ((*listener = socket(currInfo->ai_family, currInfo->ai_socktype,
                 currInfo->ai_protocol))
            < 0) {
            printf("could not create soctet\n");
            continue;
        }
        int yes = 1;
        if (setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,
                sizeof(yes))) {
            perror("setsockopt\n");
            return -1;
        }
        if (bind(*listener, currInfo->ai_addr, currInfo->ai_addrlen) < 0) {
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

    if (listen(*listener, 10) < 0) {
        printf("listen failure\n");
        return -1;
    }
    printf("server: waiting for connections...\n");

    FD_SET(*listener, master);

    (*fdmax) = ((*fdmax) > (*listener) ? (*fdmax) : (*listener));
    return 0;
}

int initializeClient(char ip[], char port[])
{
    int socketId;
    struct addrinfo *servInfo, *currInfo, hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &servInfo) != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }
    for (currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
        if ((socketId = socket(currInfo->ai_family, currInfo->ai_socktype,
                 currInfo->ai_protocol))
            < 0) {
            printf("could not create soctet\n");
            continue;
        }
        if (connect(socketId, currInfo->ai_addr, currInfo->ai_addrlen) < 0) {
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

void handleReceive(int i, int* userCount, char* userNames[MAX_SIZE],
    int otherServerFd, int* fileSending, int* fileReceiving,
    FILE** fp, int otherFd, int ourFd, int fdmax, fd_set* master,
    char** fileName, char* ourServerName)
{
    int nbytes;
    char buf[MAX_SIZE];
    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
        if (nbytes == 0) {
            printf("selectserver: socket %d hung up\n", i);
        } else {
            printf("recv error\n");
        }
        close(i);
        FD_CLR(i, master);
        (*userCount)--;
        userNames[i][0] = '\0';
    } else {
        printf("got bits:%s\n", buf);
        if (strstr(buf, "#get")) {
            char* file = buf + 4;
            printf("%s\n", file);
            sendFile(trimwhitespace(file), i);
        } else if (strstr(buf, "@")) {
            char* file = buf + 4;
            char* ff = trimwhitespace(file); // check if contains newline, then
            char* message = malloc(sizeof(char) * MAX_SIZE);
            strcpy(message, "");
            strcat(strcat(strcpy(message, "FILE"), ff), "\n\0");
            if (strstr(buf, ourServerName)) {
                (*fileReceiving) = 1;
                printf("%s\n", file);
                strcpy(*fileName, file);
                (*fp) = fopen(trimwhitespace(file), "w");
            } else {
                sendMessage(otherServerFd, message);
                (*fileSending) = 1;
            }
            free(message);
            message = malloc(sizeof(char) * MAX_SIZE);
            strcpy(message, "");
            strcat(strcat(strcpy(message, "RECEIVEFILE"), ff), "\n\0");
            sendMessage(i, message);
            free(message);
        } else if (strstr(buf, "FILE")) {
            (*fileReceiving) = 1;
            char* file = buf + 6;
            printf("%s\n", file);

            char* file1 = malloc(sizeof(char) * MAX_SIZE);
            strcpy(file1, file);
            file1[strcspn(file, "\n")] = 0;
            printf("file 1: %s\n", file1);
            strcpy(*fileName, file1);
            (*fp) = fopen(trimwhitespace(*fileName), "w");
            printf("%lu\n", strlen(file) - strlen(file1));
            if (strlen(file) != strlen(file1) && strlen(file) - strlen(file1) != 1) {
                char* fileContent = file + strlen(file1);
                char* noEnd = strremove(fileContent, "%END%");
                fputs(noEnd, (*fp));
            }
        } else if ((*fileReceiving) == 1) {
            char* result = (char*)malloc(MAX_SIZE);
            strcpy(result, buf);
            char* noEnd = strremove(result, "%END%");
            fputs(noEnd, (*fp));
            printf("receiving %s", noEnd);
            free(result);
            if (strstr(buf, "%END%")) {
                (*fileReceiving) = 0;
                fclose(*fp);
                for (int j = 0; j <= fdmax; j++) {
                    if (FD_ISSET(j, master)) {
                        if (j != ourFd && j != otherFd) {
                            char* message = malloc(sizeof(char) * MAX_SIZE);
                            strcat(
                                strcat(strcpy(message, "MESSAGEGautas failas: "), *fileName),
                                "\n\0");
                            printf("%s", message);
                            sendMessage(j, message);
                            free(message);
                        }
                    }
                }
                for (int i = 0; i < MAX_SIZE; i++) {
                    (*fileName)[i] = '\0';
                }
            }
        } else if ((*fileSending) == 1) {
            char* result = (char*)malloc(MAX_SIZE);
            strcpy(result, buf);
            printf("sending %s\n", result);
            sendMessage(otherServerFd, result);
            free(result);
            if (strstr(buf, "%END%")) {
                (*fileSending) = 0;
            }
        } else if (strstr(buf, "MESSAGE") == NULL && (*fileReceiving) == 0 && (*fileSending) == 0) {
            buf[strcspn(buf, "\n")] = 0;
            for (int j = 0; j <= fdmax; j++) {
                if (FD_ISSET(j, master)) {
                    if (j != ourFd && j != otherFd) {
                        printf("%d\n", j);
                        char* message = malloc(sizeof(char) * MAX_SIZE); // problem with memory 32 crashes
                        strcat(
                            strcat(strcat(strcat(strcpy(message, "MESSAGE"), userNames[i]),
                                       ": "),
                                buf),
                            "\n\0");
                        printf("%s", message);
                        sendMessage(j, message);
                        free(message);
                    }
                }
            }
        }
        for (int i = 0; i < MAX_SIZE; i++) {
            buf[i] = '\0';
        }
    }
}
