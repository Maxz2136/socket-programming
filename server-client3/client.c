#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>

# define max_buffer_size 256

int main(int argc, char* argv[]) {
    int sockfd, portnum; // declaring the variable storing the socket filedescriptor and portnumber of the socket
    struct sockaddr_in  server_addr; // sockaddr is the structure to hold the address of a socket

    // checking if both IP address and port number are provided of the server
    if (argc != 3) {
        printf("Please enter both the IP address and the port number of the server\n");
        exit(0);
    }

    portnum = atoi(argv[2]);  //input port number is in string, converting it to integer

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // creating (opening) a socket using a socket API, AF_INET describes the protocol family, SOCK_STREAM means
                                               //the socket performs the connection oriented network (TCP protocol) and 3rd argument 0 is the protocol id for internet protocol (IP)
    
    if (sockfd == -1) {  //checking for socket creation error
        printf("Couldn't create a socket\n");
        exit(0);
    }

    // Now connecting the client socket to server socket for establishing a connection
    bzero((char *) &server_addr, sizeof(struct sockaddr_in)); //fill the sockaddr_in address space with 0 since sockaddr_in.sin_zero[8] needs to be \0
    server_addr.sin_family = AF_INET; //This data is not sent over network (so no proper change of byte order is required)
    server_addr.sin_port = htons(portnum); //port number is set and byte order is changed to network byte order as it will be transfered over network (to another machine maybe)
    
    if (!inet_aton(argv[1], &server_addr.sin_addr)) { //inet_aton() converts the string dot format IP address to network address and in the network byte order  
        printf("Wrong IP address provided!\n");       // returns 1 on successful conversion or otherwise 0
        exit(0);
    }

    // server socket address is all configured (sockaddr_in structure is configured which stores all the server socket address related info, protocol info)
    // Connecting to the server (sending the first connection request to server to see if communication is at all possible before sending any data)
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) < 0) { //establishing a connection to a server socket (checking if a server socket is open for communication or not)
        printf("Error in connection to the server!\n"); //connect() returns 0 in success and -1 on failure
        exit(0);
    }
    //connect() --> sends connection request to server socket. --> is non blocking in nature --> it returns 0 on success and -1 on failure
    // if the listen() is already executed on ther server socket, connect() will return 0 otherwise -1. (server doesn't have to accept the connection for connect() to return 0, connect() doesn't wait for accept() on the connection request)
    //this happens because, by the whole UNIX network api design, once a connection request is sent to a server socket, it can be either accepted or ignored by the server. connection can't be outrightly rejected, no such feature is provided
    int msg_len;
    char status[2];
    msg_len = recv(sockfd, status, sizeof(status), 0);  //recv() is blocking in nature, it waits until message is recieved. A interesting thing to note is that, even though if peer socket is not created, recv() will not return 0 or any error
    // recv() will just block until some data is recieved or until TIMEOUT happens (have to check on time out part).
    // one interesting thing to note is that, recv() returns zero when already a connection is estabished but peer socket is closed suddenly during a session, but it doesn't happen if peer socket is not created in the first place,
    // it means, recv() in some way check the recently closed file list in the peer and also if the connection request was accepted or not, if it was not, recv() waits (blocking) hoping request will be accepted and data will be recieved (--> have to check on this understanding!)

    if (msg_len < 0) {
        printf("Error in reading message from socket!\n");
        exit(0);
    }

    if (msg_len == 0) {
        printf("Server is down!\n");
    }

    if (status[0] == '0') { // checking if connection request is accepted by server or not
        printf("server is too busy!\n");
        exit(0);
    } else {
        printf("connection accepted by the server\n");
    }

    struct pollfd arr[1];
    arr[0].fd = sockfd;
    arr[0].events = POLLHUP;
    
    printf("Press ctrl-c to exit!\n");
    //now build a repl for the remaining read and write part
    char expression[max_buffer_size];
    while(1) {
        printf("Enter the arithmetic expression\n");
        scanf("%[^\n]%*c", expression);
        printf("Sending the expression to the server\n");

        //sending string to the server
        // send() is non blocking in nature,it means it will never wait for message to be recieved by the peer, or if peer socket is open or closed, it just checks for 
        // the remaining write buffer space in the socket, if buffer has sufficient space, it will store the data in the write buffer and move on to the execution of the next statement
        // if write buffer is full, error will be returned. remaining task of ensuring reliability of data packet reaching peer socket is taken care by network protocols
        // but if suppose, the peer socket is closed and client is sending data using send(), isn't there is any feature to notify send() that peer socket is closed like in case of recv() --> have to read and research on this question
        if (send(sockfd, expression, strlen(expression) + 1, 0) < 0) { //exit on failure
            printf("Couldn't write to the socket: socket issue!\n");
            exit(0);
        } 

        char result[max_buffer_size];
        ssize_t size = recv(sockfd, result, max_buffer_size, 0);  //blocking system call
        if (size < 0) {
            printf("Error reading from buffer\n");
            exit(0);
        }
        if (size == 0) {
            printf("Server is down!\n");
            exit(0);
        }
        printf("The resut is %s \n", result);
    }
}