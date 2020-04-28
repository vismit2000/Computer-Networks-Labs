#include "packet.h"

void error_exit(char *s){
    perror(s);
    exit(EXIT_FAILURE);
}


// Returns the local date/time formatted as 2014-03-19 11:11:52
char* getFormattedTime(void) {

    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Must be static, otherwise won't work
    static char _retval[20];
    strftime(_retval, sizeof(_retval), "%H:%M:%S", timeinfo);

    return _retval;
}

void printLog(nodeName actionNode, action event, packet_category type, int seqNum, nodeName sourceNode, nodeName destNode){
    char* timestamp = getFormattedTime();
    printf("%s | %s | %s | %s | %d | %s | %s |\n", timestamp, nodeNameStr[actionNode], actionStr[event], packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fprintf(logFilePtr, "%s | %s | %s | %s | %d | %s | %s |\n", timestamp, nodeNameStr[actionNode], actionStr[event], packetTypeStr[type], seqNum, nodeNameStr[sourceNode], nodeNameStr[destNode] );
    fflush(logFilePtr);
}


int main()
{
    logFilePtr = fopen("log_file.txt", "a");

    struct sockaddr_in serverAddr, clientAddr;
    int sockfd, i, slen = sizeof(clientAddr), recv_len;
    //char buf[BUFLEN];
    PKT rcv_pkt;
    //create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
        error_exit("socket");
    }

    //set master socket to allow multiple connections
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");
    // zero out the structure
    memset((char *) &serverAddr, 0, sizeof(serverAddr));
     
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons( PORT );
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(sockfd , (struct sockaddr*)&serverAddr, sizeof(serverAddr) ) == -1){
        error_exit("bind");
    }
    
    /* Create file where data will be stored */
    FILE *fp = fopen("output.txt", "wb");

    if(fp == NULL)
        error_exit("File open error");

    fseek(fp, 0, SEEK_SET);

    // handle the file work
    while(1)
    {
        if(recvfrom(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &clientAddr, &slen) < 0)
            error_exit("recv client");

        // printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", rcv_pkt.sq_no, rcv_pkt.size, rcv_pkt.channel_id);                        
        printLog(SERVER, RCVD, DATA, rcv_pkt.sq_no, SERVER, RELAY1);

        /* MAINTAIN BUFFERRING at end*/

        fseek(fp, rcv_pkt.sq_no, SEEK_SET);
        fwrite(rcv_pkt.data, 1, rcv_pkt.size, fp);
        fflush(fp);
        
        // printf("SENT ACK: Seq. No %d of size %d Bytes from channel %d\n",rcv_pkt.sq_no, rcv_pkt.size, rcv_pkt.channel_id);
        rcv_pkt.is_DATA = false;    // Now this packet becomes ACK

        if( sendto(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &clientAddr, slen) < 0 )
            error_exit("Send Relay");
        printLog(SERVER, SENT, ACK, rcv_pkt.sq_no, SERVER, RELAY2);
    }
    fclose(fp);
    fclose(logFilePtr);

    // Close the serverSocket
    if( close(sockfd) < 0)
        error_exit("Close");

    return 0;
}