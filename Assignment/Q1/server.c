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

int main()
{
    int serverSocket;
    int client_socket[2];    // Total 2 client sockets (since there are 2 TCP channels)
    int max_clients = 2;
    int serverLen;  // Length of serverAddress
    int clientLen;  // Length of clientAddress
    int sd;     //Socket Descriptor
    int max_sd;     // maximum socket descriptor (used in SELECT)
    int activity;   //Activating select
    int new_socket;
    int bytesReceived = 0;

    struct sockaddr_in serverAddress, clientAddress;

    PKT rcv_pkt;
    //set of socket descriptors  
    fd_set readfds; 

    //initialise all client_socket[] to 0 so not checked  
    for (int i = 0; i < max_clients; i++)   
        client_socket[i] = 0;   
    
    /*CREATE A TCP SOCKET*/
    if( (serverSocket = socket(AF_INET , SOCK_STREAM , 0)) < 0)      
        error_exit("socket failed");   
    
    //set master socket to allow multiple connections
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");

    /*CONSTRUCT LOCAL ADDRESS STRUCTURE - type of socket created */
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons( PORT );
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind the socket to the localhost PORT  
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)   
        error_exit("Bind failed");

    if (listen(serverSocket, MAXPENDING) < 0)   
        error_exit("Listen");

    //accept the incoming connection  
    serverLen = sizeof(serverSocket);

    /* Create file where data will be stored */
    FILE *fp = fopen("output.txt", "wb");

    if(fp == NULL)
        error_exit("File open error");

    fseek(fp, 0, SEEK_SET);

    while(1)   
    {   
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add server socket to set  
        FD_SET(serverSocket, &readfds);   
        max_sd = serverSocket;   
             
        //add child sockets to set  
        for (int i = 0 ; i < max_clients ; i++)   
        {   
            //socket descriptor  
            sd = client_socket[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        }   
     
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))     
            error_exit("Select error");   
             
        //If something happened on the server socket , then it is an incoming connection  
        if (FD_ISSET(serverSocket, &readfds))   
        {   
            clientLen = sizeof(clientAddress);
            if ((new_socket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLen)) < 0)   
                error_exit("Accept");   
             
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++)   
            {   
                //if position is empty  
                if( client_socket[i] == 0 )   
                {   
                    client_socket[i] = new_socket;   
                    break;   
                }   
            }   
        }
       //else it is some IO operation on some other socket 
        for (int i = 0; i < max_clients; i++)   
        {   
            sd = client_socket[i];   
                 
            if (FD_ISSET( sd , &readfds))   
            {   
                // handle the file work
                // toDiscard = false; 
                discardRandom();

                if(toDiscard == false)
                {
                    if ((bytesReceived = read(client_socket[i], &rcv_pkt, sizeof(rcv_pkt))) <= 0)
                        error_exit("read()");

                    printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", rcv_pkt.sq_no, rcv_pkt.size, rcv_pkt.channel_id);                        

                    /* MAINTAIN BUFFERRING at end*/

                    fseek(fp, rcv_pkt.sq_no, SEEK_SET);
                    fwrite(rcv_pkt.data, 1, rcv_pkt.size, fp);
                    fflush(fp);
                    
                    printf("SENT ACK: Seq. No %d of size %d Bytes from channel %d\n",rcv_pkt.sq_no, rcv_pkt.size, rcv_pkt.channel_id);
                    rcv_pkt.is_DATA = false;    // Now this packet becomes ACK
                    sleep(1);
                    write(client_socket[i], &rcv_pkt, sizeof(rcv_pkt));
                }
            }   
        }   
    }       

    fclose(fp);

    // Close client_sockets
    for(int i = 0; i < max_clients; i++)
        if( close(client_socket[i])  < 0)
            error_exit("Close");

    // Close the serverSocket
    if( close(serverSocket) < 0)
        error_exit("Close");

    return 0;
}