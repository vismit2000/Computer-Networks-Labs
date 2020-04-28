#ifndef PACKET_H
#define PACKET_H

#include <stdio.h> // printf
#include <string.h> // memset
#include <stdlib.h> // exit(0);
#include <stdbool.h>
#include <arpa/inet.h> // different address structures are declared here
#include <sys/socket.h> // for socket(), connect(), send(), recv() functions
#include <unistd.h>  // close() function
#include <sys/types.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 8882   //The port on which to send data
#define MAXPENDING 5    //Specify maximum of 5 pending connections for the server socket
#define TIMEOUT 2
#define PACKET_SIZE 100
#define PDR 50    // Packet Drop Rate - Initially set to 10 %


typedef struct packet {
    char data[PACKET_SIZE + 1];  // payload data
    int size; //Size of data
    int sq_no;  // (in terms of bytes) offset of 1st byte of packet wrt input file
    bool is_last_pkt;
    bool is_DATA;   // specifies whether packet is DATA or ACK
    int channel_id;   // 0 or 1
} PKT;


#endif