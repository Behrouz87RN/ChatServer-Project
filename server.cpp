#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <regex.h>
#include <math.h>
// #define DEBUG
#include <pthread.h>
#define MaxBuff 1024
#define MaxNumClients 100
typedef uint32_t socklen_t;

pthread_mutex_t mutex;
// int clients[20];
int n = 0;
// char *nicknames[20][12];
struct client
{
    int socket;
    char nickname[12];
};
struct client clients[MaxNumClients];

void sendtoall(char *msg, int curr, char *nickname)
{
    int i;
    char *ret = strstr(msg, "MSG ");
    char mainMessage[500] = {0};
    strncpy(mainMessage, msg + 3, strlen(msg)-3);

    pthread_mutex_lock(&mutex);

    for (i = 0; i < n; i++)
    {
        // if (clients[i].socket != curr) // IS not current client
        // {
        char se_message4[1024] = {0};
        // strcpy(se_message4, &clients[i].nickname);
        strcat(se_message4, "MSG ");
        strcat(se_message4, nickname);
        // strcat(se_message4, " ");
        strcat(se_message4, mainMessage);
        
        if (send(clients[i].socket, se_message4, strlen(se_message4), 0) < 0)
        {
            printf("sending failure \n");
            exit(0);
        }
        // }
    }
    pthread_mutex_unlock(&mutex);
}

void distributeAllMessages(char* msg, int curr, char *nickname){

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
    
    // Print the positions of all occurrences of "MSG"
    for (int i = 0; i < count; i++) {
        messages[i][300] = {0};
        strncpy(messages[i], msg + positions[i], positions[i+1]-positions[i]);
        if (strlen(messages[i]) > 255 + 4)
        {
            printf("Recived message is ignored because it is longer than 255 charachter.\n");
        } else {
            sendtoall(messages[i], curr, nickname);
        }
    }
    
    // Free memory
    free(positions);

    if ( count == 0) {
        char se_message3[5200];
        strcpy(se_message3, "ERROR ");
        strcat(se_message3, msg);
        int sendres = send(curr, se_message3, strlen(se_message3), 0);
        if (sendres < 0)
        {
            printf("Mesage sanding faild");
        }
        printf("incorrect Message recv\n");
    }
};

void *recvmg(void *clientIndex)
{
    int currentClientId = (uintptr_t)clientIndex;
    int sock = clients[currentClientId].socket;
    char nickname[12];
    strcpy(nickname, clients[currentClientId].nickname);
    char msg[5200];
    int len, i;
    while ((len = recv(sock, msg, 5200, 0)) > 0)
    {
        msg[len] = '\0';
        distributeAllMessages(msg, sock, nickname);
    }

    printf("Connection to clinet %s is closed.\n", nickname);

    // find this client index (could be shifted when other clients get closed)
    int currentClientIndex;
    for (i = 0; i < n; i++)
    {
        // printf("clients[i].socket == sock: %d %d \n", clients[i].socket, sock);
        // printf("clients[i].nick: %s \n", clients[i].nickname);
        if (clients[i].socket == sock)
        {
            // printf("found client index: %d \n", i);
            currentClientIndex = i;
            break;
        }
    }

    // remove client from the list of online clients.
    for (i = currentClientIndex; i < n; i++)
    {
        memcpy(&clients[i], &clients[i + 1], sizeof(client));
    }
    n--;

    // print all clients
    // for ( i = 0; i < n; i++) {
    //     printf("%s %d  \n", clients[i].nickname, i);
    // }
    return 0 ;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    char delim[] = ":";
    char *Desthost = strtok(argv[1], delim);
    char *Destport = strtok(NULL, delim);
    int port = atoi(Destport);
    pthread_t recvt;
    int rc, socket_desc, clientsockt, sendres, readres;
    char buffer[1024];
    socklen_t len2;

   struct addrinfo *p, hints,clientaddr, *res = NULL;
    memset(&hints, 0x00, sizeof(hints));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rc = getaddrinfo(Desthost, Destport, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = res; p != NULL; p = p->ai_next) {
		if ((socket_desc = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
    printf("Socket created successfully\n");


    // Binding newly created socket to given IP and verification
    if (rc = bind(socket_desc, res->ai_addr, res->ai_addrlen) != 0)
    {
        printf("socket bind failed...\n");
        return -1;
    }
    else

        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(socket_desc, 100)) != 0)
    {
        perror("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len2 = sizeof(clientaddr);

    while (1)
    {
        // Accept the data packet from client and verification

        clientsockt = accept(socket_desc, (struct sockaddr *)&clientaddr, &len2);
        if (clientsockt < 0)
        {
            printf("server accept failed...\n");

            continue;
        }
        else
        {
            if (n == MaxNumClients)
            {
                char bussy_message[40];
                strcpy(bussy_message, "Chat server is bussy. try again later.\n");
                memset(buffer, '\0', sizeof(buffer));
                sendres = send(clientsockt, bussy_message, strlen(bussy_message), 0);
                if (sendres < 0)
                {
                    printf("Mesage sanding faild");
                    printf("ERROR TO\n");

                    continue;
                }
                continue;
            }

            printf("server accept the client...\n");

            char se_message[20];
            strcpy(se_message, "HELLO 1\n");
            memset(buffer, '\0', sizeof(buffer));

            sendres = send(clientsockt, se_message, strlen(se_message), 0);
            if (sendres < 0)
            {
                printf("Mesage sanding faild");
                printf("ERROR TO\n");

                continue;
            }

            memset(buffer, '\0', MaxBuff);

            readres = read(clientsockt, buffer, MaxBuff);
            if (readres < 0)
            {
                printf("ERROR TO\n");

                continue;
            }
            else
            {

                char *ret = strstr(buffer, "NICK");

                if (ret != NULL)
                {

                    char nickname[50] = {0};
                    strncpy(nickname, buffer + 5, strlen(buffer)-5);
                    nickname[strlen(nickname)-1]='\0';

                    // regex++++(A-Za-z0-9\_) handling
                    regex_t reegex;
                    int value;
                    // Function call to create regex
                    value = regcomp(&reegex, "^[A-Za-z0-9\\_]*$", 0);
                    value = regexec(&reegex, nickname, 0, NULL, 0);
                    // printf("%i",value);

                    if (value != 0 || strlen(nickname) > 12)
                    {
                        printf("invalid nickname, client ignored!\n");

                        char se_message[100] = {0};
                        strcpy(se_message, "ERROR\n The nickname must be alphanumeric and not longer than 12\n");

                        sendres = send(clientsockt, se_message, strlen(se_message), 0);
                        if (sendres < 0)
                        {
                            printf("Mesage sanding faild\n");
                            printf("ERROR TO\n");

                            continue;
                        }
                        continue;
                    }

                    strcpy(clients[n].nickname, nickname);

                    printf("nickname:%s\n", clients[n].nickname);
                    char se_message[20];
                    strcpy(se_message, "OK\n");
                    memset(buffer, '\0', sizeof(buffer));

                    sendres = send(clientsockt, se_message, strlen(se_message), 0);
                    if (sendres < 0)
                    {
                        printf("Mesage sanding faild");
                        printf("ERROR TO\n");

                        continue;
                    }
                }
                else
                {
                    perror("Nike name doesnot excist\n");
                    char se_message[20];
                    strcpy(se_message, "ERROR\n");
                    memset(buffer, '\0', sizeof(buffer));
                    sendres = send(clientsockt, se_message, strlen(se_message), 0);
                    if (sendres < 0)
                    {
                        printf("Mesage sanding faild");

                        continue;
                    }
                    continue;
                }
                pthread_mutex_lock(&mutex);
                clients[n].socket = clientsockt;

                // creating a thread for each client
                pthread_create(&recvt, NULL, recvmg, (void *)(uintptr_t)n);
                pthread_mutex_unlock(&mutex);
                n++;
            }
        }
    }

    // After chatting close the socket

    close(socket_desc);
}
