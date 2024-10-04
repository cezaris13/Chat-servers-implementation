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

typedef struct {
  char key[MAX_KEY_LENGTH];
  char value[MAX_VALUE_LENGTH];
} EnvVar;

char *strremove(char *str, const char *sub) {
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
  for (int i = 0; i < MAX_USERS; i++)
    if (userNames[i] != NULL && strcmp(searchedString, userNames[i]) == 0)
      return 1;
  return 0;
}

void getUserName(char socketName[], int socketFd, int *userCount,
                 char *userNames[MAX_USERS]) {
  char sendNameString[] = "SENDNAME\n\0";
  while (1) {
    char bufUserName[MAX_USERS];
    sendMessage(socketFd, sendNameString, socketName);
    if (recv(socketFd, bufUserName, sizeof bufUserName, 0) < 0) {
      printf("%s: recv error\n", socketName);
      continue;
    }

    bufUserName[strcspn(bufUserName, "\n")] = 0;
    if (strlen(bufUserName) == 0)
      break;

    if (!containsUserName(bufUserName, userNames) && *userCount <= MAX_USERS) {
      strcpy(userNames[socketFd], bufUserName);
      char nameOkString[] = "VK\n\0";
      (*userCount)++;
      sendMessage(socketFd, nameOkString, socketName);
      break;
    } else if (*userCount > MAX_USERS) {
      break;
    }
  }
}

void sendFile(char *filePath, int destinationSocket, char socketName[]) {
  FILE *file;
  file = fopen(filePath, "r");
  char *message = malloc(sizeof(char) * MAX_SIZE);
  strcpy(message, "");
  strcat(strcat(strcpy(message, "FILE"), filePath), "\n\0");
  sendMessage(destinationSocket, message, socketName);
  while (feof(file) != 1) {
    char *buffer = malloc(sizeof(char *) * MAX_SIZE);
    fgets(buffer, MAX_SIZE, file);
    printf("%s: %s\n", socketName, buffer);
    sendMessage(destinationSocket, buffer, socketName);
    free(buffer);
  }
  char *endOfFile = "%END%\n\0";
  sendMessage(destinationSocket, endOfFile, socketName);
  fclose(file);
  free(message);
}

int initializeSocket(char port[], char ip[], fd_set *master, int *listener,
                     int *fileDescriptorMax, char socketName[]) {
  struct addrinfo *servInfo, *currInfo, hints;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(ip, port, &hints, &servInfo) != 0) {
    printf("%s: getaddrinfo error\n", socketName);
    freeaddrinfo(servInfo);
    return -1;
  }

  for (currInfo = servInfo; currInfo != NULL; currInfo = currInfo->ai_next) {
    if ((*listener = socket(currInfo->ai_family, currInfo->ai_socktype,
                            currInfo->ai_protocol)) < 0) {
      printf("%s: could not create soctet\n", socketName);
      continue;
    }
    int yes = 1;
    if (setsockopt(*listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,
                   sizeof(yes))) {
      printf("%s: ", socketName);
      perror("setsockopt\n");
      return -1;
    }
    if (bind(*listener, currInfo->ai_addr, currInfo->ai_addrlen) < 0) {
      close(*listener);
      printf("%s: bind failed\n", socketName);
      continue;
    }
    break;
  }
  freeaddrinfo(servInfo);

  if (currInfo == NULL) {
    printf("%s: failed to bind\n", socketName);
    return -1;
  }

  if (listen(*listener, 10) < 0) {
    printf("%s: listen failure\n", socketName);
    return -1;
  }
  printf("%s: waiting for connections...\n", socketName);

  FD_SET(*listener, master);

  (*fileDescriptorMax) =
      ((*fileDescriptorMax) > (*listener) ? (*fileDescriptorMax) : (*listener));
  return 0;
}

int initializeClient(char ip[], char port[], char socketName[]) {
  int socketId;
  struct addrinfo *servInfo, *currInfo, hints;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(ip, port, &hints, &servInfo) != 0) {
    freeaddrinfo(servInfo);
    printf("%s: getaddrinfo error\n", socketName);
    return -1;
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

  if (currInfo == NULL) {
    printf("%s: client failed to connect\n", socketName);
    return -1;
  }

  freeaddrinfo(servInfo);
  return socketId;
}

void handleReceive(int i, int *userCount, char *userNames[MAX_SIZE],
                   int secondarySocketFileDescriptorActive, int *fileSending,
                   int *fileReceiving, FILE **fp,
                   int secondarySocketFileDescriptor,
                   int primarySocketFileDescriptor, int fileDescriptorMax,
                   fd_set *master, char **fileName, char socketName[]) {
  int nbytes;
  char buf[MAX_SIZE];
  if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
    if (nbytes == 0)
      printf("%s: socket %d hung up\n", socketName, i);
    else
      printf("%s: recv error\n", socketName);

    close(i);
    FD_CLR(i, master);
    (*userCount)--;
    userNames[i][0] = '\0';
    return;
  }

  printf("%s: got bits:%s\n", socketName, buf);
  if (strstr(buf, "#get")) {
    char *file = buf + 4;
    printf("%s: %s\n", socketName, file);
    sendFile(trimwhitespace(file), i, socketName);
  } else if (strstr(buf, "@")) {
    char *file = buf + 8; // remove @ and socket name @Socket1 file.txt
    char *ff = trimwhitespace(file); // check if contains newline
    char *message = malloc(sizeof(char) * MAX_SIZE);
    strcpy(message, "");
    strcat(strcat(strcpy(message, "FILE"), ff), "\n\0");
    if (strstr(buf, socketName)) {
      (*fileReceiving) = 1;
      printf("%s: %s\n", socketName, file);
      strcpy(*fileName, file);
      (*fp) = fopen(ff, "w");
    } else {
      sendMessage(secondarySocketFileDescriptorActive, message, socketName);
      (*fileSending) = 1;
    }
    free(message);
    message = malloc(sizeof(char) * MAX_SIZE);
    strcpy(message, "");
    strcat(strcat(strcpy(message, "RECEIVEFILE"), ff), "\n\0");
    sendMessage(i, message, socketName);
    free(message);
  } else if (strstr(buf, "FILE")) {
    (*fileReceiving) = 1;
    char *file = buf + 4;
    printf("%s: %s\n", socketName, file);

    char *file1 = malloc(sizeof(char) * MAX_SIZE);
    strcpy(file1, file);
    file1[strcspn(file, "\n")] = 0;
    printf("%s: file 1: %s\n", socketName, file1);
    strcpy(*fileName, file1);
    (*fp) = fopen(trimwhitespace(*fileName), "w");
    printf("%s: %lu\n", socketName, strlen(file) - strlen(file1));
    if (strlen(file) != strlen(file1) && strlen(file) - strlen(file1) != 1) {
      char *fileContent = file + strlen(file1);
      char *noEnd = strremove(fileContent, "%END%");
      fputs(noEnd, (*fp));
    }
  } else if ((*fileReceiving) == 1) {
    char *result = (char *)malloc(MAX_SIZE);
    strcpy(result, buf);
    char *noEnd = strremove(result, "%END%");
    fputs(noEnd, (*fp));
    printf("%s: receiving %s", socketName, noEnd);
    free(result);
    if (!strstr(buf, "%END%"))
      return;

    (*fileReceiving) = 0;
    fclose(*fp);
    for (int j = 0; j <= fileDescriptorMax; j++) {
      if (!FD_ISSET(j, master))
        continue;

      if (j == primarySocketFileDescriptor ||
          j == secondarySocketFileDescriptor)
        continue;

      char *message = malloc(sizeof(char) * MAX_SIZE);
      strcat(
          strcat(strcpy(message, "MESSAGEFile has been received: "), *fileName),
          "\n\0");
      printf("%s", message);
      sendMessage(j, message, socketName);
      free(message);
    }

    for (int i = 0; i < MAX_SIZE; i++)
      (*fileName)[i] = '\0';
  } else if (*fileSending) {
    char *result = (char *)malloc(MAX_SIZE);
    strcpy(result, buf);
    printf("%s: sending %s\n", socketName, result);
    sendMessage(secondarySocketFileDescriptorActive, result, socketName);
    free(result);
    if (strstr(buf, "%END%"))
      *fileSending = 0;
  } else if (strstr(buf, "MESSAGE") == NULL && (*fileReceiving) == 0 &&
             (*fileSending) == 0) {
    buf[strcspn(buf, "\n")] = 0;
    for (int j = 0; j <= fileDescriptorMax; j++) {
      if (!FD_ISSET(j, master))
        continue;

      if (j == primarySocketFileDescriptor ||
          j == secondarySocketFileDescriptor)
        continue;

      printf("%s %d\n", socketName, j);
      char *message =
          malloc(sizeof(char) * MAX_SIZE); // problem with memory 32 crashes
      strcat(
          strcat(strcat(strcat(strcpy(message, "MESSAGE"), userNames[i]), ": "),
                 buf),
          "\n\0");
      printf("%s: %s", socketName, message);
      sendMessage(j, message, socketName);
      free(message);
    }
  }
  for (int i = 0; i < MAX_SIZE; i++)
    buf[i] = '\0';
}
