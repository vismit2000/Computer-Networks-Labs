#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXPENDING 5
#define BUFFERSIZE 32

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main ()
{
    /*CREATE A TCP SOCKET*/
    int listenfd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0){ 
        error_exit("socket");
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");

    printf ("Server Socket Created\n");
    
    /*CONSTRUCT LOCAL ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddress, clientAddress;
    memset (&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12346);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    printf ("Server address assigned\n");
    
    int temp = bind(listenfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (temp < 0){
        error_exit("bind");
    }
    printf ("Binding successful\n");

    int temp1 = listen(listenfd, MAXPENDING);
    
    if (temp1 < 0){
        error_exit("listen");
    }
    printf ("Now Listening\n");
    
    char msg[BUFFERSIZE];
    int clientLength = sizeof(clientAddress);
    pid_t pid;

    for( ; ; ){
        int connfd = accept (listenfd, (struct sockaddr*)&clientAddress, &clientLength);
        
        if (connfd < 0){
            error_exit("accept");
        }

        if((pid = fork()) == 0){
            close(listenfd);
            printf ("Handling Client %s inside process %d\n", inet_ntoa(clientAddress.sin_addr), getpid());

            int temp2 = recv(connfd, msg, BUFFERSIZE, 0);
            if (temp2 < 0){ 
                error_exit("recv");
            }
            printf("Received from client : %s\n", msg);
            
            printf ("ENTER MESSAGE FOR CLIENT\n");
            fgets(msg, BUFFERSIZE, stdin);
            
            int bytesSent = send (connfd, msg, strlen(msg), 0);
            if (bytesSent != strlen(msg)){
                error_exit("send");
            }

            if( close(connfd) < 0){
                error_exit("close connfd");
            }

            exit(EXIT_SUCCESS);
        }

        if( close(connfd) < 0){
            error_exit("close connfd");
        }
    }
    if( close(listenfd)  < 0){
        error_exit("close listenfd");
    }
}