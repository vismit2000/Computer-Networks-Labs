#include "packet.h"

int curr_seq_no_0 = 0;
int curr_seq_no_1 = PACKET_SIZE;

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


PKT generate_next_pkt(FILE *fp, char *buff, int channel_id, int window_num)
{
    PKT new_pkt;

    memset(buff, 0, sizeof(buff));

    if(channel_id == 0)
    {
        new_pkt.sq_no = curr_seq_no_0;
        fseek(fp, curr_seq_no_0, SEEK_SET);
        curr_seq_no_0 += 2 * PACKET_SIZE;
    }
    else
    {
        new_pkt.sq_no = curr_seq_no_1;
        fseek(fp, curr_seq_no_1, SEEK_SET);
        curr_seq_no_1 += 2 * PACKET_SIZE;
    }
    
    int nread = fread(buff, 1, PACKET_SIZE, fp); // // nread contains the No. of Bytes read  

    if(nread < PACKET_SIZE)
    {
        new_pkt.is_last_pkt = true;
        // If there is something tricky going on with read ..Either there was error, or we reached end of file.
        if (feof(fp))
            printf("End of file\n");
        if (ferror(fp))
            printf("Error reading\n");
    }
    else
        new_pkt.is_last_pkt = false;

    if(feof(fp))            // Handling case when last packet is also of PACKET_SIZE
        new_pkt.is_last_pkt = true;

    buff[nread] = '\0';
    strcpy(new_pkt.data, buff);
    new_pkt.size = nread;
    new_pkt.is_DATA = true;
    new_pkt.channel_id = channel_id;
    new_pkt.window_no = window_num;

    return new_pkt;
}

bool unACKed(int *arr)
{
    int ans = 0;
    for(int i = 0; i < WINDOW_SIZE; i++)
    {
        if(arr[i] == 0)
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
        error_exit("Insufficient arguments");

    logFilePtr = fopen("log_file.txt", "a");

    int window[WINDOW_SIZE];
    memset(window, 0, sizeof(int)*WINDOW_SIZE);

    int sockfd0 = 0, sockfd1 = 0, max_sd = 0;
    int bytesReceived;
    
    struct sockaddr_in relay0Address, relay1Address;

    /* Create the first socket (EVEN) */
    if((sockfd0 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error_exit("Socket 0 failed");

    /* Create the second socket (ODD) */
    if((sockfd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        error_exit("Socket 1 failed");

    max_sd = (sockfd0 >= sockfd1) ? sockfd0 : sockfd1;
    

    int slen0 = sizeof(relay0Address), slen1 = sizeof(relay1Address);
 
    memset((char *) &relay0Address, 0, sizeof(relay0Address));
    relay0Address.sin_family = AF_INET;
    relay0Address.sin_port = htons( RPORT0 );
	relay0Address.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset((char *) &relay1Address, 0, sizeof(relay1Address));
    relay1Address.sin_family = AF_INET;
    relay1Address.sin_port = htons( RPORT1 );
	relay1Address.sin_addr.s_addr = inet_addr("127.0.0.1");

    PKT rcv_ack;

    // Open input file provided as argument1
    FILE *fp = fopen(argv[1], "rb");
    if(fp == NULL)
        error_exit("File open error");

    unsigned char buff[PACKET_SIZE] = {0};

    fseek(fp, 0, SEEK_END);
	int file_length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    int num_windows = file_length / WINDOW_SIZE;
    int k;

    if(file_length % WINDOW_SIZE != 0)
        num_windows++;

    for(int i = 0; i < num_windows; i++)
    {
        PKT pkt[WINDOW_SIZE];
        memset(window, -1, sizeof(int) * WINDOW_SIZE);
        for(int j = 0; j < WINDOW_SIZE; j++)
        {
            k = i*WINDOW_SIZE + j;
            window[j] = 0;
            pkt[j] = generate_next_pkt(fp, buff, k % 2, j);
            if(pkt[j].size == 0)
                break;

            if(k % 2 == 0)
            {
                if(pkt[j].size != 0)
                    if (sendto(sockfd0 , &pkt[j], sizeof(pkt[j]), 0 , (struct sockaddr *) &relay0Address, slen0) == -1){
                        error_exit("sendto() 0");
                    }
                // printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", pkt[j].sq_no, pkt[j].size, pkt[j].channel_id);
                printLog(CLIENT, SENT, DATA, pkt[j].sq_no, CLIENT, RELAY1);
            }
            else
            {
                if(pkt[j].size != 0)
                    if (sendto(sockfd1 , &pkt[j], sizeof(pkt[j]), 0 , (struct sockaddr *) &relay1Address, slen1) == -1){
                        error_exit("sendto() 1");
                    }
                // printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", pkt[j].sq_no, pkt[j].size, pkt[j].channel_id);
                printLog(CLIENT, SENT, DATA, pkt[j].sq_no, CLIENT, RELAY1);
            }
        }

        while(unACKed(window))
        {
            fd_set rcvSet;
            int activity;
            
            struct timeval tv;

            tv.tv_sec = TIMEOUT;
            tv.tv_usec = 0;

            FD_SET(sockfd0, &rcvSet);
            FD_SET(sockfd1, &rcvSet);

            if((activity = select(max_sd + 1, &rcvSet, NULL, NULL, &tv) ) < 0){
                error_exit("error on select");
            }

            if(activity != 0) 
            {
                // printf("\nNo Timeout\n");
                if(FD_ISSET(sockfd0, &rcvSet))
                {
                    if (recvfrom(sockfd0 , &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &relay0Address, &slen0) == -1)
                        error_exit("recvfrom() 0");
                    
                    window[rcv_ack.window_no] = 1;

                    // printf("RCVD ACK: Seq. No %d of size %d Bytes from channel %d\n", rcv_ack.sq_no, rcv_ack.size, rcv_ack.channel_id);
                    printLog(CLIENT, RCVD, ACK, rcv_ack.sq_no, RELAY1, CLIENT);
                }   
                   

                if(FD_ISSET(sockfd1, &rcvSet))
                {
                    if (recvfrom(sockfd1 , &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &relay1Address, &slen1) == -1)
                        error_exit("recvfrom() 1");
                    
                    window[rcv_ack.window_no] = 1;

                    // printf("RCVD ACK: Seq. No %d of size %d Bytes from channel %d\n", rcv_ack.sq_no, rcv_ack.size, rcv_ack.channel_id);
                    printLog(CLIENT, RCVD, ACK, rcv_ack.sq_no, RELAY1, CLIENT);
                }  
            }

            else    // Timeout occurred
            {
                // printf("\nTimeout occurred\n");
                for(int j = 0; j < WINDOW_SIZE; j++)
                {
                    k = i*WINDOW_SIZE + j;
                    if(window[j] == 0)
                    {
                        if(k % 2 == 0)
                        {
                            if(pkt[j].size != 0)
                                if (sendto(sockfd0 , &pkt[j], sizeof(pkt[j]), 0 , (struct sockaddr *) &relay0Address, slen0) == -1){
                                    error_exit("sendto() 0");
                                }
                            // printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", pkt[j].sq_no, pkt[j].size, pkt[j].channel_id);
                            printLog(CLIENT, RETRANSMIT, DATA, pkt[j].sq_no, CLIENT, RELAY1);
                        }
                        else
                        {
                            if(pkt[j].size != 0)
                                if (sendto(sockfd1 , &pkt[j], sizeof(pkt[j]), 0 , (struct sockaddr *) &relay1Address, slen1) == -1){
                                    error_exit("sendto() 1");
                            }
                            // printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", pkt[j].sq_no, pkt[j].size, pkt[j].channel_id);
                            printLog(CLIENT, RETRANSMIT, DATA, pkt[j].sq_no, CLIENT, RELAY1);
                        }
                    }
                }
            }
            // sleep(1);
        }
    }

    fclose(fp);
    fclose(logFilePtr);

    // Close the sockets
    if( close(sockfd0) < 0)
        error_exit("Close");

    if( close(sockfd1) < 0)
        error_exit("Close");

    return 0;
}