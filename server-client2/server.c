#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>

#define max_buffer_size 256

int ops(char a, char b, char operator) {
    int u = a - 48;
    int v = b - 48;
    switch(operator) {
        case '+': return u+v;
        case '-': return u-v;
        case '*': return u*v;
        case '/': return u/v;
    }
}

// method to parse the arithmetic expression string and evaluate the result
int evaluate_expression(char* expression) {
    char exp[max_buffer_size];
    strcpy(exp, expression);
    // first do a parse of the expression to check for correctness of the expression (first check --> parantheses balance) and then store position of the parentheses
    char stack[max_buffer_size];
    int count = 0;
    for (int i = 0; i < strlen(exp); i++) {
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

    int val[max_buffer_size];
    char ops[max_buffer_size];
    int val_count = 0, ops_count = 0;
    for (int i = 0; i < strlen(exp); i++) {
        if (exp[i] == '(') {
            ops[ops_count] = exp[i];
            ops_count++;
        }
        if (isdigit(exp[i])) {
            int var = 0;
            while(isdigit(exp[i]) && i < strlen(exp)) {
                var = var * 10 + (exp[i] - 48);
                i++;
            }
            i--;
        }
        
    }
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
    
    int client_sockfd;
    struct sockaddr client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    int len;
    char res[max_buffer_size];
    int pid;
    long long int count = 0; // a counter to assign new id to newly spawned client.
    long long int id = 0; //client id
    // repl for the parent server to accept connection request from the server port
    // and create a new child
    while(1) {
        id = count;
        struct pollfd arr[1];
        arr[0].fd = sockfd;
        arr[0].events = POLLIN;
        int num_req = poll(arr, (unsigned long) 1, -1); //blocking in nature as TIMEOUT == -1
        if (num_req == 1 && arr[0].revents == POLLIN) {
            client_sockfd = accept(sockfd, (struct sockaddr*) &client_addr, (socklen_t *) &addr_len);
            if ((pid = fork()) < 0) { //creating a child server process
                printf("Error in spawning a child server!\n");
                int msg_len;
                char ack[2] = "0";
                msg_len = send(client_sockfd, ack, strlen(ack) + 1, 0);
                if (msg_len < 0) {
                    printf("Sending to socket failed!\n");
                    exit(0);
                }
            }
            if (pid > 0) count++;
            if (pid == 0) break;
        }
    }

    //send a acknowledgment message to the client that his connection request is accepted
    int msg_len;
    char ack[2] = "1";
    msg_len = send(client_sockfd, ack, strlen(ack) + 1, 0);
    if (msg_len < 0) {
        printf("Sending to socket failed!\n");
        exit(0);
    }

    // repl for child server to communicate with the server.
    while(1) {

        len = recv(client_sockfd, res, max_buffer_size, 0); //blocking in nature, blocks until a new message is received in the socket
        if (len < 0) {
            printf("Error in reading socket\n");
            exit(0);
        }

        if (len == 0) { // recv() returns 0 when it is waiting for data from peer socket (blocking nature) but the peer socket is closed.
            if (close(client_sockfd) < 0) {
                printf("Error on closing socket\n");
                exit(0);
            }
            printf("Client %lld is down!\n", id);
            exit(0);
        }
        printf("client %lld: %s\n",id, res);
        //sending back message to the client
        len = send(client_sockfd, res, strlen(res) + 1, 0);
        printf("len: %d\n", len);
        if (len < 0) {
            printf("sending failed on the socket!\n");
            exit(0);
        }
    }  
}