#include "packet.h"

int curr_seq_no = 0;

void error_exit(char *s){
    perror(s);
    exit(EXIT_FAILURE);
}

PKT generate_next_pkt(FILE *fp, char *buff, int channel_id)
{
    PKT new_pkt;

    memset(buff, 0, sizeof(buff));
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
    new_pkt.sq_no = curr_seq_no;
    new_pkt.size = nread;
    
    new_pkt.is_DATA = true;
    new_pkt.channel_id = channel_id;

    curr_seq_no += PACKET_SIZE;

    return new_pkt;
}

void send_pkt(int sockfd, PKT pkt)
{
    if(pkt.size != 0)
    {
        printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", pkt.sq_no, pkt.size, pkt.channel_id);
        write(sockfd, &pkt, sizeof(pkt));
    }
}

void receive_ACK(int sockfd, PKT pkt)
{
    int bytesReceived;
    if ((bytesReceived = read(sockfd, &pkt, sizeof(pkt))) < 0)
        error_exit("read()");
    printf("RCVD ACK: Seq. No %d of size %d Bytes from channel %d\n", pkt.sq_no, pkt.size, pkt.channel_id);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("Usage: ./client <input_file>\n");
        error_exit("Insufficient arguments");
    }

    int sockfd0 = 0, sockfd1 = 0, max_sd = 0;
    int bytesReceived;
    
    struct sockaddr_in serverAddress;

    /* Create the first socket */
    if((sockfd0 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_exit("Socket 0 failed");

    /* Create the second socket */
    if((sockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error_exit("Socket 1 failed");

    max_sd = (sockfd0 >= sockfd1) ? sockfd0 : sockfd1;

    /* Initialize sockaddr_in data structure */
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons( PORT ); // port
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  //inet_addr("127.0.0.1");
    

    /* Attempt first connection */
    if(connect(sockfd0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        error_exit("Connect 0 Failed");

     /* Attempt second connection */
    if(connect(sockfd1, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        error_exit("Connect 1 Failed");
    

    // Open input file provided as argument1
    FILE *fp = fopen(argv[1], "rb");
    if(fp == NULL)
        error_exit("File open error");

    /* Read data from file and send it for uploading*/
    fseek(fp, 0, SEEK_SET);
    
    unsigned char buff[PACKET_SIZE] = {0};

    PKT pkt0, pkt1;

    /* Channel 0 - Send 1st packet*/
    pkt0 = generate_next_pkt(fp, buff, 0);
    send_pkt(sockfd0, pkt0);
    
    /* Channel 1 - Send 2nd packet*/
    pkt1 = generate_next_pkt(fp, buff, 1);
    send_pkt(sockfd1, pkt1);

    while(1)
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
            if(FD_ISSET(sockfd0, &rcvSet)){
                receive_ACK(sockfd0, pkt0);
                if(pkt0.is_last_pkt)
                    exit(EXIT_SUCCESS);

                pkt0 = generate_next_pkt(fp, buff, 0);
                send_pkt(sockfd0, pkt0);
            }
            
            if(FD_ISSET(sockfd1, &rcvSet)){
                receive_ACK(sockfd1, pkt1);
                if(pkt1.is_last_pkt)
                    exit(EXIT_SUCCESS);

                pkt1 = generate_next_pkt(fp, buff, 1);
                send_pkt(sockfd1, pkt1);
            }
        }
        else    // Timeout occurred
        {
            send_pkt(sockfd0, pkt0);
            send_pkt(sockfd1, pkt1);
        }
        // sleep(1);
    }

    fclose(fp);

    // Close the sockets
    if( close(sockfd0) < 0)
        error_exit("Close");

    if( close(sockfd1) < 0)
        error_exit("Close");

    return 0;
}