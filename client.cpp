#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
// #define DEBUG

#define MaxDataSize 1024
#define MaxBuff 1024

void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void printAllMessages(char* msg, char *nickname){

    char *search = (char *)"MSG ";
    int len = strlen(msg);
    int count = 0;
    int *positions = (int*) malloc(sizeof(int) * len);
    char messages[20][300] = {0};
    // Search for all occurrences of "MSG" in the string
    char *pos = strstr(msg, search);
    while (pos != NULL) {
        positions[count] = pos - msg;
        count++;
        pos = strstr(pos + 1, search);
    }
    positions[count] = strlen(msg);
    //  printf("Found %d occurrences of '%s':\n", count, search);
    // Print the positions of all occurrences of "MSG"
    for (int i = 0; i < count; i++) {
        messages[i][300] = {0};
        // printf("position %d\n", positions[i]);
        strncpy(messages[i], msg + positions[i], positions[i+1]-positions[i]);
        // printf("mmmmsg:%s\n", messages[i]);
        if (strlen(messages[i]) > 255 + 4)
        {
            printf("Recived message is ignored because it is longer than 255 charachter.\n");
        } else {
          char tempMsg[259];
          strcpy(tempMsg, messages[i]);
          char *recvNickname;
          recvNickname = strtok(tempMsg + 4, " ");
          // printf("nickname:%s\n", recvNickname);

          if (strcmp(nickname, recvNickname) == 0){
            // self message, do nothing
          } else {
            char messageEscaped[259] = {0};
            strcpy(messageEscaped, messages[i] + 4);
            printf("%s", messageEscaped);
          }
        }
    }
    
    // Free memory
    free(positions);

    if ( count == 0 && strlen(msg) > 0) {
       if (strstr(msg, "ERROR") != NULL) {
            perror(msg);
          } else {
            printf("incorrect Message recv\n");
            printf("%s\n", msg);
          }
    }
};

int main(int argc, char *argv[])
{
  // Seperating IP and port
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport = strtok(NULL, delim);

  /* Do magic chage string to int*/
  int port = atoi(Destport);
  printf("Host %s, and port %d.\n", Desthost, port);

  int fdmax, i;
  int socket_desc = 0, n = 0;
  char recvBuff[5200];
  char result[100];
  int status;
  char myIP[16];
  char s[INET6_ADDRSTRLEN];
  char buf[MaxDataSize];
  int numbytes;
  socklen_t len;
  char nick[50];
  strcpy(nick, argv[2]);

  if (strlen(nick) > 12)
  {
    printf("The name  must be lower that 12\n");
    exit(0);
  }

  // create regex+++(A-Za-z0-9\_) handling
  regex_t reegex;
  int value;
  // Function call to create regex
  value = regcomp(&reegex, "^[A-Za-z0-9\\_]*$", 0);
  value = regexec(&reegex, nick, 0, NULL, 0);
  // printf("%i",value);

  if (value != 0)
  {
    printf("The nickname must be alphanumeric \n");
    exit(0);
  }

  struct addrinfo serv_addr;
  struct addrinfo hints, *res, *p;
  struct sockaddr_in sin;

  fd_set master; // create a master set
  fd_set readFd; // create set for clients

  //   Check number of argument

  if (argc != 3)
  {
    fprintf(stderr, "usage: showip hostname\n");
    return 1;
  }

  // Clearing hints
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(Desthost, Destport, &hints, &res)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    printf("status %d", status);
    return 2;
  }
  for (p = res; p != NULL; p = p->ai_next)
  {
    // Create the socket
    if ((socket_desc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("socket creation failed");
      continue;
    }

    break;
  }

  if (p == NULL)
  {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }

  freeaddrinfo(res);

  memcpy(&serv_addr, res->ai_addr, sizeof(serv_addr));

  if (connect(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\n Error : Connect Failed \n");
    return 1;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
  //  printf("client: connecting to %s\n", s);

  memset(recvBuff, 0, sizeof(recvBuff));
  // buf[n]=0;
  if ((numbytes = recv(socket_desc, recvBuff, sizeof(recvBuff), 0)) == -1)
  {
    perror("recv");
    exit(1);
  }

  printf("client: received %s\n", recvBuff);

  if (strcmp(recvBuff, "HELLO 1\n") == 0)
  {
    printf("the protocol is supported\n");

    strcpy(nick, "NICK ");
    strcat(nick, argv[2]);
    strcat(nick,"\n");

    if (numbytes = send(socket_desc, nick, strlen(nick), 0) == -1)
    {
      perror("send problem:");
      exit(1);
    }

    memset(recvBuff, 0, sizeof(recvBuff));

    if ((numbytes = recv(socket_desc, recvBuff, sizeof(recvBuff), 0)) == -1)
    {
      perror("recv problem");
      exit(1);
    }

    printf("client received: %s \n", recvBuff);
    char *ret = strstr(recvBuff, "ERR");
    if (ret != NULL)
    {
      exit(0);
    }
  }
  else
  {
    printf("Version is incorrect\n");
  }

  FD_ZERO(&master);
  FD_ZERO(&readFd);
  FD_SET(0, &master);
  FD_SET(socket_desc, &master);
  fdmax = socket_desc;

  while (1)
  {
    readFd = master;
    if (select(fdmax + 1, &readFd, NULL, NULL, NULL) == -1)
    {
      perror("select");
      exit(0);
    }

    for (i = 0; i <= fdmax; i++)
    {
      if (FD_ISSET(i, &readFd))
      {

        char sendBuffer[MaxBuff];
        char recvBuffer[MaxBuff];
        int nbyteRecv;
        char message[259];

        if (i == 0)
        {
          fgets(sendBuffer, MaxBuff, stdin);

          strcpy(message, "MSG ");
          strcat(message, sendBuffer);
          //strcat(message,"\n");

          if (strlen(message) > 255 + 4)
          {
           printf("YOU are Not allowed to write more than 255 character\n");
             continue;
          }

          if (strcmp(sendBuffer, "quit\n") == 0)
          {
            exit(0);
          }
          else
          {
            int sendres = send(socket_desc, message, strlen(message), 0);
            //printf("send Meessage:%s",message);
            if (sendres < 0)
            {
              printf("Mesage sanding faild");

              continue;
            }
          }
        }
        else
        {
          memset(recvBuffer, 0, sizeof(recvBuffer));
          nbyteRecv = recv(socket_desc, recvBuffer, MaxBuff, 0);
          char message[5200] = {0};
          memcpy(message, recvBuffer, nbyteRecv);
          // printf("recvBuffer: %s\n", recvBuffer);
          // printf("message from: %s\n", message);
          printAllMessages(message, argv[2]);
          // printAllMessages((char *)"MSG negin message1\nMSG behzad message2\nMSG behrouz message3\n", argv[2]);

          fflush(stdout);
        }
      }
    }
  }
  printf("client-quited\n");

  close(socket_desc);
  return 0;
}
