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
#define MAX_KEY_LENGTH 128
#define MAX_VALUE_LENGTH 128
#define MAX_ENV_VARS 100
#define MAX_LINE_LENGTH 256

const char *envCommandsLocation = "../../commands";
typedef struct {
  char key[MAX_KEY_LENGTH];
  char value[MAX_VALUE_LENGTH];
} EnvVar;

typedef struct {
  char **names;
  int count;
} Users;

typedef struct {
  char *fileName;
  FILE *fp;
  int isReceiving;
  int isSending;
} FileData;

typedef struct {
  int primarySocketFileDescriptor;
  int secondarySocketFileDescriptor;
  int fileDescriptorMax;
} FileDescriptors;

EnvVar env_vars[MAX_ENV_VARS];

char *strremove(char *str, char *sub) {
  size_t len = strlen(sub);
  if (len > 0) {
    char *p = str;
    while ((p = strstr(p, sub)) != NULL)
      memmove(p, p + len, strlen(p + len) + 1);
  }
  return str;
}

char *trimwhitespace(char *str) {
  char *end;
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

// Function to load the .env file and parse key-value pairs
int load_env(const char *filename, EnvVar env_vars[], int max_vars) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening file");
    return -1;
  }

  char line[MAX_LINE_LENGTH];
  int env_count = 0;

  // Read the file line by line
  while (fgets(line, sizeof(line), file) != NULL && env_count < max_vars) {
    // Skip empty lines and lines starting with '#'
    if (line[0] == '\n' || line[0] == '#') {
      continue;
    }

    // Find the position of the '=' character
    char *equal_sign = strchr(line, '=');
    if (equal_sign != NULL) {
      // Split the line into key and value
      *equal_sign = '\0'; // Replace '=' with null terminator
      char *key = trimwhitespace(line);
      char *value = trimwhitespace(equal_sign + 1);

      // Remove newline from the value if present
      value[strcspn(value, "\n")] = 0;

      // Store the key and value in the env_vars array
      strncpy(env_vars[env_count].key, key, MAX_KEY_LENGTH);
      strncpy(env_vars[env_count].value, value, MAX_VALUE_LENGTH);
      env_count++;
    }
  }
  fclose(file);
  return env_count;
}

// Function to get the value of an environment variable by key
char *get_env_value(const char *key, EnvVar env_vars[], int env_count) {
  for (int i = 0; i < env_count; i++) {
    if (strcmp(env_vars[i].key, key) == 0) {
      return env_vars[i].value;
    }
  }
  return NULL;
}

void sendMessage(int socketFileDescriptor, char *message, char socketName[]) {
  if (send(socketFileDescriptor, message, strlen(message), 0) == -1)
    printf("%s: send error\n", socketName);
}

int containsUserName(char *searchedString, char *userNames[MAX_USERS]) {
  for (int i = 0; i < MAX_USERS; i++) {
    if (userNames[i] == NULL)
      break;

    if (strcmp(searchedString, userNames[i]) == 0)
      return 1;
  }
  return 0;
}

void getUserName(char socketName[], int socketFd, int *userCount,
                 char *userNames[MAX_USERS]) {
  if (*userCount >= MAX_USERS)
    return; // No need to proceed if the user count exceeds the maximum

  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *sendNameCommand = malloc(sizeof(char) * MAX_SIZE);
  char *nameOkString = malloc(sizeof(char) * MAX_SIZE);

  sprintf(sendNameCommand, "%s\n",
          get_env_value("SendName", env_vars, env_count));
  sprintf(nameOkString, "%s\n", get_env_value("NameOk", env_vars, env_count));

  while (1) {
    char bufUserName[MAX_USERS];
    sendMessage(socketFd, sendNameCommand, socketName);
    if (recv(socketFd, bufUserName, sizeof bufUserName, 0) < 0) {
      printf("%s: recv error\n", socketName);
      continue;
    }

    char *newlinePos = strchr(bufUserName, '\n');
    if (newlinePos != NULL)
      *newlinePos = '\0'; // Replace '\n' with null terminator

    if (strlen(bufUserName) == 0)
      break;

    if (!containsUserName(bufUserName, userNames) && *userCount <= MAX_USERS) {
      strcpy(userNames[socketFd], bufUserName);
      (*userCount)++;
      sendMessage(socketFd, nameOkString, socketName);
      break;
    }
  }

  free(sendNameCommand);
  free(nameOkString);
}

void sendFile(char *filePath, int destinationSocket, char socketName[]) {
  FILE *file = fopen(filePath, "r");
  if (file == NULL) {
    printf("%s: Failed to open file %s\n", socketName, filePath);
    return;
  }

  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *message = malloc(sizeof(char) * MAX_SIZE);
  char *endOfFile = malloc(sizeof(char) * MAX_SIZE);

  sprintf(message, "%s%s\n", get_env_value("File", env_vars, env_count),
          filePath);
  sprintf(endOfFile, "%s\n", get_env_value("End", env_vars, env_count));

  sendMessage(destinationSocket, message, socketName);
  // Buffer for reading the file
  char *buffer = malloc(sizeof(char) * MAX_SIZE);

  while (fgets(buffer, sizeof(buffer), file) != NULL) {
    printf("%s: %s\n", socketName, buffer);
    sendMessage(destinationSocket, buffer, socketName);
  }
  sendMessage(destinationSocket, endOfFile, socketName);
  printf("%s: file sending to %d socket has finished\n", socketName,
         destinationSocket);
  fclose(file);
  free(message);
  free(buffer);
  free(endOfFile);
}

int initializeSocket(char port[], char ip[], fd_set *master, int *listener,
                     int *fileDescriptorMax, char socketName[]) {
  struct addrinfo hints, *servInfo, *currInfo;
  int reuseAddrOpt = 1; // More descriptive name for the option flag

  // Zero out the 'hints' struct and fill in necessary fields
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // Use IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  hints.ai_flags = AI_PASSIVE;     // For wildcard IP address

  int rv;
  if ((rv = getaddrinfo(ip, port, &hints, &servInfo)) != 0) {
    printf("%s: getaddrinfo error: %s\n", socketName, gai_strerror(rv));
    return -1;
  }

  for (currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
    if ((*listener = socket(currInfo->ai_family, currInfo->ai_socktype,
                            currInfo->ai_protocol)) < 0) {
      printf("%s: could not create soctet\n", socketName);
      continue;
    }

    if (setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &reuseAddrOpt, sizeof(reuseAddrOpt))) {
      printf("%s: setSockopt error: ", socketName);
      perror("setsockopt");
      continue;
    }

    if (bind(*listener, currInfo->ai_addr, currInfo->ai_addrlen) < 0) {
      printf("%s: bind failed trying next\n", socketName);
      close(*listener);
      continue;
    }

    break;
  }

  // Free the address info structure after done using it
  freeaddrinfo(servInfo);

  if (currInfo == NULL) {
    printf("%s: failed to bind to any address\n", socketName);
    return -1;
  }

  if (listen(*listener, 10) < 0) {
    printf("%s: listen failure\n", socketName);
    close(*listener); // Close the socket if listen fails
    return -1;
  }

  printf("%s: waiting for connections...\n", socketName);

  FD_SET(*listener, master);

  // Update the maximum file descriptor value
  if (*listener > *fileDescriptorMax)
    *fileDescriptorMax = *listener;

  return 0;
}

int initializeClient(char ip[], char port[], char socketName[]) {
  int socketId;
  struct addrinfo *servInfo, *currInfo, hints;

  // Zero out the 'hints' struct and fill in necessary fields
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // Use IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

  // Get address information
  int rv;
  if ((rv = getaddrinfo(ip, port, &hints, &servInfo)) != 0) {
    printf("%s: getaddrinfo error: %s\n", socketName, gai_strerror(rv));
    return -1; // No need to free servInfo as it is NULL
  }

  for (currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
    if ((socketId = socket(currInfo->ai_family, currInfo->ai_socktype,
                           currInfo->ai_protocol)) < 0) {
      printf("%s: could not create soctet\n", socketName);
      continue;
    }
    if (connect(socketId, currInfo->ai_addr, currInfo->ai_addrlen) < 0) {
      close(socketId);
      printf("%s: bind failed\n", socketName);
      continue;
    }
    break;
  }

  freeaddrinfo(servInfo);

  if (currInfo == NULL) {
    printf("%s: client failed to connect\n", socketName);
    return -1;
  }

  return socketId;
}

void handleGetCommand(char *command, char *buf, int clientSocket,
                      char socketName[]) {
  char *fileName = buf + strlen(command);
  printf("%s: %s\n", socketName, fileName);
  sendFile(trimwhitespace(fileName), clientSocket, socketName);
}

void handleAtCommand(char *buf, int clientSocket,
                     int secondarySocketFileDescriptor, FileData *fileData,
                     char socketName[]) {
  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *fileCommand = get_env_value("File", env_vars, env_count);
  char *receiveFileCommand = get_env_value("ReceiveFile", env_vars, env_count);

  char *message = malloc(sizeof(char) * MAX_SIZE);

  // handling the case when theres no socket name
  char *file = strchr(buf, ' ');
  char *ff = trimwhitespace(file); // check if contains newline
  sprintf(message, "%s%s\n", fileCommand, ff);
  printf("secondary socket file descriptor: %d\n",
         secondarySocketFileDescriptor);
  printf("ClientSocket: %d\n", clientSocket);
  if (strstr(buf, socketName)) {
    fileData->isReceiving = 1;
    printf("%s: %s\n", socketName, file);
    strcpy(fileData->fileName, file);
    fileData->fp = fopen(ff, "w");
  } else {
    sendMessage(secondarySocketFileDescriptor, message, socketName);
    fileData->isSending = 1;
  }
  free(message);
  message = malloc(sizeof(char) * MAX_SIZE);
  sprintf(message, "%s%s\n", receiveFileCommand, ff);
  sendMessage(clientSocket, message, socketName);
  free(message);
}

void handleFileCommand(char *buf, FileData *fileData, char socketName[]) {
  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *fileCommand = get_env_value("File", env_vars, env_count);
  char *endOfFile = get_env_value("End", env_vars, env_count);

  fileData->isReceiving = 1;
  char *file = buf + strlen(fileCommand);
  printf("%s: %s\n", socketName, file);

  char *file1 = strdup(file);
  char *newlinePos = strchr(file1, '\n');
  if (newlinePos != NULL)
    *newlinePos = '\0'; // Replace '\n' with null terminator

  strcpy(fileData->fileName, file1);

  fileData->fp = fopen(trimwhitespace(fileData->fileName), "w");
  if (fileData->fp == NULL) {
    perror("Failed to open file for writing");
    free(file1);
    return;
  }

  if (strlen(file) != strlen(file1) && strlen(file) - strlen(file1) != 1) {
    char *fileContent = file + strlen(file1);
    char *noEnd = strremove(fileContent, endOfFile);

    if (noEnd != NULL) {
      fputs(noEnd, fileData->fp); // Write content to the file
      free(noEnd); // Free the content buffer if it was dynamically allocated
    }
  }
  free(file1);
}

void broadcastMessage(char *buf, char *userName, fd_set *master,
                      FileDescriptors *fileDescriptors, char socketName[]) {
  char *newlinePos = strchr(buf, '\n');
  if (newlinePos != NULL)
    *newlinePos = '\0'; // Replace '\n' with null terminator

  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *messagePrefix = get_env_value("Message", env_vars, env_count);

  char *message = malloc(sizeof(char) * MAX_SIZE);
  sprintf(message, "%s%s: %s\n", messagePrefix, userName, buf);

  for (int i = 0; i <= fileDescriptors->fileDescriptorMax; i++) {
    if (!FD_ISSET(i, master))
      continue;
    if (i == fileDescriptors->primarySocketFileDescriptor ||
        i == fileDescriptors->secondarySocketFileDescriptor)
      continue;

    printf("%s: %s", socketName, message);
    sendMessage(i, message, socketName);
  }

  free(message);
}

void handleFileSending(char *buf, int *fileSending,
                       int secondarySocketFileDescriptor, char socketName[]) {
  char *result = strdup(buf); // Duplicate the buffer into a new string
  if (result == NULL) {
    perror("Failed to allocate memory for file sending");
    return; // Handle allocation failure
  }

  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *endOfFile = get_env_value("End", env_vars, env_count);
  printf("%s: sending %s\n", socketName, result);

  printf("%d\n", secondarySocketFileDescriptor);
  sendMessage(secondarySocketFileDescriptor, result, socketName);

  if (strstr(buf, endOfFile)) {
    *fileSending = 0; // End file sending when %END% is detected
    printf("%s: File transmission completed.\n", socketName);
  }

  free(result);
}

void handleFileReceiving(char *buf, fd_set *master, FileData *fileData,
                         FileDescriptors *fileDescriptors, char socketName[]) {
  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *messageCommand = get_env_value("Message", env_vars, env_count);
  char *endOfFile = get_env_value("End", env_vars, env_count);

  char *result = strdup(buf);
  char *noEnd = strremove(result, endOfFile);

  // Write the content to the file
  if (fputs(noEnd, fileData->fp) == EOF) {
    printf("Failed to write to a file\n");
    free(noEnd);
    return;
  }
  printf("%s: receiving %s\n", socketName, noEnd);

  if (!strstr(buf, endOfFile)) {
    // If the file content does not contain %END%, return and continue receiving
    free(noEnd);
    return;
  }

  // End file receiving and notify clients
  fileData->isReceiving = 0;
  printf("file receiving: %s: is receiving has ended\n", socketName);
  if (fclose(fileData->fp) == EOF) {
    perror("Failed to close the file");
  }

  printf("%s: File receiving completed for %s.\n", socketName,
         fileData->fileName);

  // Notify all clients about the received file
  for (int j = 0; j <= fileDescriptors->fileDescriptorMax; j++) {
    if (!FD_ISSET(j, master))
      continue;
    if (j == fileDescriptors->primarySocketFileDescriptor ||
        j == fileDescriptors->secondarySocketFileDescriptor)
      continue;

    char *message = malloc(sizeof(char) * MAX_SIZE);
    sprintf(message, "%sFile has been received: %s\n", messageCommand,
            fileData->fileName);
    sendMessage(j, message, socketName);
    free(message);
  }

  // Clear the fileName after the file is received
  memset(fileData->fileName, 0, MAX_SIZE);
  free(noEnd);
}

void handleReceive(int i, Users *userData, FileData *fileData,
                   FileDescriptors *fileDescriptors, fd_set *master,
                   char socketName[]) {

  EnvVar env_vars[MAX_ENV_VARS];
  int env_count = load_env(envCommandsLocation, env_vars, MAX_ENV_VARS);
  char *getCommand = get_env_value("GetFile", env_vars, env_count);
  char *messageCommand = get_env_value("Message", env_vars, env_count);
  char *fileCommand = get_env_value("File", env_vars, env_count);
  char *sendFileCommand = get_env_value("SendFile", env_vars, env_count);
  if (env_count == -1) {
    printf("Failed to load .env file\n");
    return;
  }

  int nbytes;
  char buf[MAX_SIZE];
  if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
    if (nbytes == 0)
      printf("%s: socket %d hung up\n", socketName, i);
    else
      printf("%s: recv error: %d\n", socketName, nbytes);

    close(i);
    FD_CLR(i, master);

    userData->count--;
    userData->names[i][0] = '\0';
    return;
  }

  printf("%s: got bits:%s\n", socketName, buf);

  if (strstr(buf, getCommand)) {
    handleGetCommand(getCommand, buf, i, socketName);
  } else if (strstr(buf, sendFileCommand)) {
    handleAtCommand(buf, i, fileDescriptors->secondarySocketFileDescriptor,
                    fileData, socketName);
  } else if (strstr(buf, fileCommand)) {
    handleFileCommand(buf, fileData, socketName);
  } else if (fileData->isReceiving) {
    handleFileReceiving(buf, master, fileData, fileDescriptors, socketName);
  } else if (fileData->isSending) {
    handleFileSending(buf, &(fileData->isSending),
                      fileDescriptors->secondarySocketFileDescriptor,
                      socketName);
  } else if (strstr(buf, messageCommand) == NULL && !(fileData->isReceiving) &&
             !(fileData->isSending)) {
    broadcastMessage(buf, userData->names[i], master, fileDescriptors,
                     socketName);
  }

  memset(buf, 0, sizeof(buf));
}
