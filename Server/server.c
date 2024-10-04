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
  char socketName[7] = "Socket ";
  strncat(socketName, &socketId[0], 1);
  int fileDescriptorMax = -1, primarySocketFileDescriptor, newFileDescriptor;
  char *userNames[MAX_USERS];
  int userCount = 0;
  fd_set readFileDescriptor, master;
  struct sockaddr_storage remoteAddress;

  for (int i = 0; i < MAX_USERS; i++)
    userNames[i] = malloc(sizeof(char) * (MAX_USERNAME_SIZE + 1));

  char *fileName = malloc(sizeof(char) * (MAX_SIZE));
  FILE *file = NULL;

  initializeSocket(primarySocketPort, ip, &master, &primarySocketFileDescriptor,
                   &fileDescriptorMax, socketName);
  int secondarySocketFileDescriptor;
  int secondarySocketFileDescriptorActive;
  int isFirstTimeConnection = 1;
  int fileReceiving = 0;
  int fileSending = 0;

  int isSecondaryServerOnline = 0;
  while (1) {
    readFileDescriptor = master;

    // Run do while to search for secondary socket if it exists.
    if (!isSecondaryServerOnline) {
      do {
        secondarySocketFileDescriptor =
            initializeClient(ip, secondarySocketPort, socketName);
        printf("%s: searching for other socket\n", socketName);
        sleep(1);
      } while (secondarySocketFileDescriptor == -1);

      isSecondaryServerOnline = 1;
    }

    if (select(fileDescriptorMax + 1, &readFileDescriptor, NULL, NULL, NULL) ==
        -1) {
      printf("%s: select error\n", socketName);
      return -1;
    }

    for (int i = 0; i <= fileDescriptorMax; i++) {
      if (!FD_ISSET(i, &readFileDescriptor))
        continue;

      if (primarySocketFileDescriptor != i) {
        handleReceive(i, &userCount, userNames,
                      secondarySocketFileDescriptorActive, &fileSending,
                      &fileReceiving, &file, secondarySocketFileDescriptor,
                      primarySocketFileDescriptor, fileDescriptorMax, &master,
                      &fileName, socketName);
        continue;
      }

      socklen_t addressLength = sizeof remoteAddress;
      if ((newFileDescriptor = accept(primarySocketFileDescriptor,
                                      (struct sockaddr *)&remoteAddress,
                                      &addressLength)) == -1) {
        printf("%s: accept error\n", socketName);
        continue;
      }

      // If the connection
      if (!isFirstTimeConnection) {
        FD_SET(newFileDescriptor, &master);
        if (newFileDescriptor > fileDescriptorMax)
          fileDescriptorMax = newFileDescriptor;

        getUserName(socketName, newFileDescriptor, &userCount, userNames);
        continue;
      }

      isFirstTimeConnection = 0;
      secondarySocketFileDescriptorActive = newFileDescriptor;
      FD_SET(newFileDescriptor, &master);

      if (newFileDescriptor > fileDescriptorMax)
        fileDescriptorMax = newFileDescriptor;
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
    /*     EnvVar env_vars[MAX_ENV_VARS]; */

    /* // Load the .env file */
    /* int env_count = load_env("../commands", env_vars, MAX_ENV_VARS); */

    /* if (env_count == -1) { */
    /*     printf("Failed to load .env file\n"); */
    /*     return 1; */
    /* } */

    /* // Get specific environment variables by key */
    /* char *sendName = get_env_value("SendName", env_vars, env_count); */
    /* printf("%s\n",sendName); */

}
