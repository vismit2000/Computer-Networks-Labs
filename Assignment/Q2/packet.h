#ifndef PACKET_H
#define PACKET_H

#include <stdio.h> // printf
#include <string.h> // memset
#include <stdlib.h> // exit(0);
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <arpa/inet.h> // different address structures are declared here
#include <sys/socket.h> // for socket(), connect(), send(), recv() functions
#include <unistd.h>  // close() function
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8882   //The server port
#define MAXPENDING 5    //Specify maximum of 5 pending connections for the server socket
#define TIMEOUT 5
#define PACKET_SIZE 100
#define PDR 10    // Packet Drop Rate - Initially set to 10
#define WINDOW_SIZE 4
#define RPORT0 8883
#define RPORT1 8884
#define DELAY_MAX 2
#define IP_SERVER "127.0.0.1"

#define SERVER_LOG_FILE "server.log"

typedef enum { SENT, RCVD, DROP, TIMOUT, RETRANSMIT } action;
typedef enum { CLIENT, SERVER, RELAY1, RELAY2 } nodeName;
typedef enum { DATA, ACK } packet_category;

char *actionStr[5] = {"SENT", "RCVD", "DROP", "TIMEOUT", "RETRANSMIT"};
char *nodeNameStr[4] = {"CLIENT", "SERVER", "RELAY1", "RELAY2"};
char *packetTypeStr[2] = {"DATA", "ACK"};

FILE *logFilePtr;




typedef struct packet {
    char data[PACKET_SIZE + 1];  // payload data
    int size; //Size of data
    int sq_no;  // (in terms of bytes) offset of 1st byte of packet wrt input file
    int window_no;
    bool is_last_pkt;
    bool is_DATA;   // specifies whether packet is DATA or ACK
    int channel_id;   // 0 or 1 (now it tells whether packet is sent through odd channel or even channel)
} PKT;


#endif