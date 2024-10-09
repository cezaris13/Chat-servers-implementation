#include <arpa/inet.h>
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

int startServer(char ip[], char primarySocketPort[], char secondarySocketPort[],
                char socketId[]) {
  char socketName[7] = "Socket";
  strncat(socketName, &socketId[0], 1);
  int newFileDescriptor;
  Users users;
  users.names = (char **)malloc(MAX_USERS * sizeof(char *));
  users.count = 0;
  fd_set readFileDescriptor, master;
  struct sockaddr_storage remoteAddress;

  for (int i = 0; i < MAX_USERS; i++)
    users.names[i] = malloc(sizeof(char) * (MAX_USERNAME_SIZE + 1));

  FileData fileData;
  fileData.fp = NULL;
  fileData.fileName = malloc(sizeof(char) * MAX_SIZE);
  fileData.isReceiving = 0;
  fileData.isSending = 0;

  FileDescriptors fileDescriptors;
  fileDescriptors.fileDescriptorMax = -1;


  initializeSocket(primarySocketPort, ip, &master, &(fileDescriptors.primarySocketFileDescriptor),
                   &(fileDescriptors.fileDescriptorMax), socketName);
  int isFirstTimeConnection = 1;

  int isSecondaryServerOnline = 0;
  while (1) {
    readFileDescriptor = master;

    // Run do while to search for secondary socket if it exists.
    if (!isSecondaryServerOnline) {
      do {
        fileDescriptors.secondarySocketFileDescriptor =
            initializeClient(ip, secondarySocketPort, socketName);
        printf("%s: searching for other socket\n", socketName);
        sleep(1);
      } while (fileDescriptors.secondarySocketFileDescriptor == -1);

      isSecondaryServerOnline = 1;
    }

    if (select(fileDescriptors.fileDescriptorMax + 1, &readFileDescriptor, NULL, NULL, NULL) ==
        -1) {
      printf("%s: select error\n", socketName);
      return -1;
    }

    for (int i = 0; i <= fileDescriptors.fileDescriptorMax; i++) {
      if (!FD_ISSET(i, &readFileDescriptor))
        continue;

      if (fileDescriptors.primarySocketFileDescriptor != i) {
        handleReceive(i, &users, &fileData, &fileDescriptors, &master, socketName);
        continue;
      }

      socklen_t addressLength = sizeof remoteAddress;
      if ((newFileDescriptor = accept(fileDescriptors.primarySocketFileDescriptor,
                                      (struct sockaddr *)&remoteAddress,
                                      &addressLength)) == -1) {
        printf("%s: accept error\n", socketName);
        continue;
      }

      // If the connection
      if (!isFirstTimeConnection) {
        FD_SET(newFileDescriptor, &master);
        if (newFileDescriptor > fileDescriptors.fileDescriptorMax)
          fileDescriptors.fileDescriptorMax = newFileDescriptor;

        getUserName(socketName, newFileDescriptor, &(users.count), users.names);
        continue;
      }

      isFirstTimeConnection = 0;
      fileDescriptors.secondarySocketFileDescriptorActive = newFileDescriptor;
      FD_SET(newFileDescriptor, &master);

      if (newFileDescriptor > fileDescriptors.fileDescriptorMax)
        fileDescriptors.fileDescriptorMax = newFileDescriptor;
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 5)
    startServer(argv[1], argv[2], argv[3], argv[4]);
  else
    printf("Not all parameters are given. Should enter: primaryServerPort, "
           "secondaryServerPort, socketId");
}
