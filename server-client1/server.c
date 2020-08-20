#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define max_buffer_size 256

// method to parse the arithmetic expression string and evaluate the result
int evaluate_expression(char* expression) {
    char exp[max_buffer_size];
    strcpy(exp, expression);
    // first do a parse of the expression to check for correctness of the expression (first check --> parantheses balance) and then store position of the parentheses
    char stack[max_buffer_size];
    int count = 0;
    for (int i = 0; i < strlen(exp), i++) {
        if (exp[i] == '(') {
            stack[count] = exp[i] && count++;
        }
        if (exp[i] == ')') {
            if((count - 1 >= 0) && exp[count-1] == '(') {
                if (count > 0) count--;
            } else {
                return -1;
            }
        }
    }
    if (count > 0) return -1;
    
    return 0;
}


int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        printf("Please enter the port number\n");
        exit(0);
    }
    int sockfd;
    struct sockaddr_in my_addr;

    //opening a socket for the server using the port info provided
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Error in creating a socket!\n");
        exit(0);
    }

    //binding a socket to server address (IP and port)
    bzero((char *) &my_addr, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(argv[1]));
    if (!inet_aton("127.0.0.1", &my_addr.sin_addr)) {
        printf("Can't convert the IP address to network address\n");
        exit(0);
    }
    // calling the bind() function
    if(bind(sockfd,(struct sockaddr*) &my_addr, sizeof(struct sockaddr_in)) < 0) {  //assign the address, port number to the open socket which will be used by server for communication
        printf("Error in binding to the socket!\n");              //this binding of socket to port and IP address is needed for network to identify which socket to dump the data in and what protocol the socket supports 
        exit(0);
    }

    if (listen(sockfd, max_buffer_size) < 0) { //makes the socket a passive socket, means kernel will put connection request for the port in the connection request queue
        printf("Couldn't listen to the socket\n"); // if queue is full, connection request are rejected and error code is sent back to client by kernel only
    }
    // now,we will accept connection request from the queue of connection request some kernel process has created.
    // also creating a repl which will accept the connection request from the queue of connection request
    
    int client_sockfd[2];
    struct sockaddr client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    int len;
    char res[max_buffer_size];
    int flag = 0;  //marks if a connection is ongoing or not, if flag == 1, reject other connection request
    while(1) {
        if (flag == 0) {  //if no connection is ongoing, this code block will execute to accept a new request
            client_sockfd[flag] = accept(sockfd,(struct sockaddr*) &client_addr, (socklen_t *) &addr_len); //blocking in nature, so until some connection request is in the queue, it will just block the program  
            flag = 1;
            char ack[2] = "1"; //this string act as a acknowledgement to the client that the request is accepted by the server (by the logic of our software --> meaning the socket will not be deleted immediately and there is no ongoing connection)
            int msg_len = send(client_sockfd[0], ack, strlen(ack), 0);
            if (msg_len < 0) {
                printf("Sending data to the socket failed!\n");
                exit(0);
            }
            printf("Connection accepted\n");
        } else { // if a connection is ongoing, this code block will execute to reject the incoming new request
            struct pollfd arr[1];  //the list of file descriptors to be checked using poll() to see if read() or accept() is possible without block.
            arr[0].fd = sockfd;
            arr[0].events = POLLIN;
            int num_req = poll(arr, (unsigned long) 1, 0);
            if (num_req == 1 && arr[0].revents == POLLIN) {
                client_sockfd[flag] = accept(sockfd,(struct sockaddr*) &client_addr, (socklen_t *) &addr_len);
                int msg_len;
                char ack[2] = "0"; // this string act as a acknowledgement to the client that the client is not accepted (by the logic of the server --> meaning the peer socket of the client will be closed immidately after connection request is accepted)
                                   // important point to note is that tcp/ip or network level UNIX API support is not provided for rejecting a connection request --> once a connection request is present in the  request queue of the server socket, it can be either ignored or has to be accepted by the server logic (code)
                msg_len = send(client_sockfd[1], ack, strlen(ack), 0);  //send() is non blocking in nature (it doesn't check if peer socket is open or closed).
                if (msg_len < 0) {
                    printf("Sending to socket failed!\n");
                    exit(0);
                }

                if (close(client_sockfd[flag]) < 0) { //closing the newly created socket
                    printf("error in closing socket\n");
                    exit(0);
                }
            }
        }

        len = recv(client_sockfd[0], res, max_buffer_size, 0); //blocking in nature, blocks until a new message is received in the socket
        if (len < 0) {
            printf("Error in reading socket\n");
            exit(0);
        }

        if (len == 0) { // recv() returns 0 when it is waiting for data from peer socket (blocking nature) but the peer socket is closed.
            if (close(client_sockfd[0]) < 0) {
                printf("Error on closing socket\n");
                exit(0);
            }
            flag = 0;
            continue;
        }
        printf("client: %s\n", res);
        //sending back message to the client
        len = send(client_sockfd[0], res, strlen(res), 0);
        if (len < 0) {
            printf("sending failed on the socket!\n");
            exit(0);
        }
    }  
}