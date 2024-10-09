#ifndef MY_FUNC2_H
#define MY_FUNC2_H

#include "Functions.c"
#include <stdio.h>
#include <sys/select.h>

char *trimwhitespace(char *str);
char *strremove(char *str, char *sub);
void sendMessage(int socketFd, char *message, char socketName[]);
int containsUserName(char *searchedString, char *userNames[MAX_USERS]);
void getUserName(char socketName[], int socketFd, int *userCount,
                 char *userNames[MAX_USERS]);
void sendFile(char *filePath, int destinationSocket, char socketName[]);
int initializeSocket(char port[], char ip[], fd_set *master, int *listener,
                     int *fileDescriptorMax, char socketName[]);
int initializeClient(char ip[], char port[], char socketName[]);
void handleReceive(int i, Users *userData, FileData *fileData,
                   FileDescriptors *fileDescriptors, fd_set *master,
                   char socketName[]);
#endif
