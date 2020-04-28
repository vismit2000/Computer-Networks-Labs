#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXPENDING 3
#define BUFFERSIZE 32
#define TOTALCLIENTS 3

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {

    /*CREATE A TCP SOCKET*/
    int serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket < 0) {
      printf("Error while server socket creation "); 
      exit (0); 
    }

    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error_exit("setsockopt(SO_REUSEADDR) failed");
    printf("Server Socket Created\n");

    /*CONSTRUCT LOCAL ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddress, clientAddress; 
    memset(&serverAddress, 0, sizeof(serverAddress)); 
    
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(1234); 
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // We can also assign inet_addr("127.0.0.1");
    printf("Server address assigned\n"); 
    
    int temp = bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if (temp < 0) {
        printf("Error while binding\n");
        exit(0);
    }
    printf("Binding successful\n"); 
    
    int temp1 = listen(serverSocket, MAXPENDING);
    if (temp1 < 0) {
        printf("Error in listen");
        exit(0);
    }
    printf("Now Listening\n"); 
    
    char msg[BUFFERSIZE]; 
    int clientLength = sizeof(clientAddress); 

    int clientSocket;
  
    clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, &clientLength);
    if (clientLength < 0) {
        printf("Error in client socket");
        exit(0);
    }
    printf("Handling Client %s\n", inet_ntoa(clientAddress.sin_addr)); 

    int temp2 = recv(clientSocket, msg, BUFFERSIZE, 0);
    if (temp2 < 0) {
        printf("problem in temp 2");
        exit(0);
    }
    // msg[temp2] = '\0';
    printf("%s\n", msg); 
    
    // printf("ENTER MESSAGE FOR CLIENT %d\n", i+1); 
    // fgets(msg, BUFFERSIZE, stdin); 

    double n = atof(msg);
    if(n - (int)n == 0.0)
        sprintf(msg, "%d", (int)n);
    else
        sprintf(msg, "%d", (int)n+1);
        
    int bytesSent = send(clientSocket, msg, strlen(msg), 0);
    if (bytesSent != strlen(msg)) {
        printf("Error while sending message to client");
        exit(0);
    }

    close(serverSocket); 
    close(clientSocket);
}