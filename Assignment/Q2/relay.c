#include "packet.h"

bool toDiscard;     // Whether to discard a packet or not

void discardRandom()
{
    int randNo = 1 + (rand() % 100);
    if(randNo <= PDR )    // Packet Drop Rate is PDR
        toDiscard = true;
    else
        toDiscard = false;
}

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


int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Calling convention: ./relay num (where is num is 1 or 2 depending on relay1 or relay2");
        exit(EXIT_FAILURE);
    }

    logFilePtr = fopen("log_file.txt", "a");

    int RNUM = atoi(argv[1]);

    int relayPort;

    if(RNUM == 1)
        relayPort = RPORT0;
    else
        relayPort = RPORT1;

    int serverSocket = 0;

    if((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error_exit("ServerSocket");

    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");

    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if(clientSocket < 0){
        error_exit("Client Socket");
    }

    struct sockaddr_in serverAddress, relayAddress, clientAddress;
    memset(&relayAddress, '0', sizeof(relayAddress));
    memset(&clientAddress, '0', sizeof(clientAddress));
    memset(&serverAddress, '0', sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons( PORT ); 
    serverAddress.sin_addr.s_addr = inet_addr(IP_SERVER); 

    relayAddress.sin_family = AF_INET;
    relayAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    relayAddress.sin_port = htons(relayPort);

    int clientLen = 0, serverLen = 0;
    clientLen = sizeof(clientAddress);
    serverLen = sizeof(serverAddress);


    if( bind(clientSocket, (struct sockaddr*)&relayAddress,sizeof(relayAddress)) < 0)
        error_exit("Bind");

    struct timeval timeout;      
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    if (setsockopt (serverSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        error_exit("setsockopt failed\n");

    PKT DATAPkt, ACKPkt;

    float DELAY;
    struct timespec delayTime;

    while(true){
        // rcv pkt from client
        if(recvfrom(clientSocket, &DATAPkt, sizeof(DATAPkt), 0, (struct sockaddr *) &clientAddress, &clientLen) < 0)
            error_exit("recv client");
    
        // printf("Received from client for seqNum %d : %s\n", DATAPkt.sq_no, DATAPkt.data);
        printLog(((RNUM == 1) ? RELAY1: RELAY2), RCVD, DATA, DATAPkt.sq_no, SERVER, CLIENT);

        discardRandom();

        if(toDiscard == false)
        {
            DELAY = ((float)rand()/(float)(RAND_MAX)) * DELAY_MAX;

            delayTime.tv_sec = (int)DELAY;
            delayTime.tv_nsec = (int)((DELAY - floor(DELAY)) * pow(10, 9));
            printf("Will wait for %lf seconds\n", DELAY);

            nanosleep(&delayTime, &delayTime);

            if( sendto(serverSocket, &DATAPkt, sizeof(DATAPkt), 0, (struct sockaddr *) &serverAddress, serverLen) < 0 )
                error_exit("Send server");

            // printf("sent to server\n");
            printLog(((RNUM == 1) ? RELAY1: RELAY2), SENT, DATA, DATAPkt.sq_no, CLIENT, SERVER);
            // rcv ack
            if(recvfrom(serverSocket, &ACKPkt, sizeof(ACKPkt), 0, (struct sockaddr *) &serverAddress, &serverLen) < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;   // don't send an ACK, since server didn't rcv the data pkt yet
                error_exit("Recv Ack Server");
            }

            // printf("received ACK from server for seqNum %d\n", ACKPkt.sq_no);
            printLog(((RNUM == 1) ? RELAY1: RELAY2), RCVD, ACK, ACKPkt.sq_no, SERVER, CLIENT);
            
            clientLen = sizeof(clientAddress);

            if( sendto(clientSocket, &ACKPkt, sizeof(ACKPkt), 0, (struct sockaddr *) &clientAddress, clientLen) < 0 ){
                error_exit("send ack client");
            }

            // printf("sent ack to client\n");
            printLog(((RNUM == 1) ? RELAY1: RELAY2), SENT, DATA, ACKPkt.sq_no, SERVER, CLIENT);
        }
        else{
            printf("====================Discarded pkt with seq num %d ===================\n", DATAPkt.sq_no);
        }
    }
    fclose(logFilePtr);
}