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
#include <error.h>

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
    
    int client_sockfd[max_buffer_size];
    for (int i = 0; i < max_buffer_size; i++) {
        client_sockfd[i] = -1;
    }

    struct sockaddr client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    int len;
    char res[max_buffer_size];
    int client_num = 0;
    // repl for the single process server to handle multiple clients
    while(1) {
        if (client_num) {
            struct pollfd arr[1];
            arr[0].fd = sockfd;
            arr[0].events = POLLIN;
            int num_req = poll(arr, (unsigned long) 1, 0);
            if (num_req == 1 && arr[0].revents == POLLIN) {
                int i;
                for (i = 0; i < max_buffer_size; i++) {
                    if (client_sockfd[i] == -1) {
                        client_sockfd[i] = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &addr_len);
                        char ack[2] = "1";
                        int msg_len = send(client_sockfd[i], ack, strlen(ack) + 1, 0);
                        if (msg_len < 0) {
                            printf("Sending to socket failed!\n");
                            exit(0);
                        }
                        client_num++;
                        printf("Client %d has joined!\n", i);
                        break;
                    }
                }
                if (i == max_buffer_size) {
                    int reject_client = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &addr_len);
                    char ack[2] = "0";
                    int msg_len = send(reject_client, ack, strlen(ack) + 1, 0);
                    if (msg_len < 0) {
                        printf("sending to socket failed!\n");
                        exit(0); // can we do something without closing the server since the client was already rejected and other present connections are fully functional
                    }
                }
            }
        } else {
            struct pollfd arr[1];
            arr[0].fd = sockfd;
            arr[0].events = POLLIN;
            int num_req = poll(arr, (unsigned long) 1, -1);
            if (num_req == 1 && arr[0].revents == POLLIN) {
                client_sockfd[client_num] = accept(sockfd, (struct sockaddr *) &client_addr, (socklen_t *) &addr_len);
                char ack[2] = "1";
                int msg_len = send(client_sockfd[client_num], ack, strlen(ack) + 1, 0);
                if (msg_len < 0) {
                    printf("Sending to socket failed!\n");
                    exit(0); // can do something else without closing the server (maybe close the client in some way)
                }
                client_num++;
                printf("client %d has joined!\n", 0);
            }
        }

        //poll and loop to read from clients.
        struct pollfd arr[max_buffer_size];
        for (int i = 0; i < max_buffer_size; i++) {
                arr[i].fd = client_sockfd[i];
                arr[i].events = POLLIN;
        }
        int count = poll(arr, (unsigned long) max_buffer_size, 0); //timeout for the poll is 0 ms
        if (count > 0) {
            for (int i = 0; i < max_buffer_size; i++) {
                if (arr[i].fd < 0) continue;
                if(arr[i].revents == POLLIN) {
                    int len = recv(arr[i].fd, res, max_buffer_size, 0);
                    if (len < 0) {
                        printf("error in reading from socket!\n");
                        exit(0);  //can do something else except exiting the server process
                    }
                    if  (len > 0) {
                        printf("client %d: %s\n", i, res);
                        if (send(arr[i].fd, res, strlen(res) + 1, 0) < 0) {
                            printf("Sending to socket failed!\n");
                            exit(0); // can actually close the client socket and make the client quit.
                        }
                    }
                    if (len == 0) {
                        printf("client %d has left!\n", i);
                        close(arr[i].fd);
                        client_sockfd[i] = -1;
                        client_num--;
                    }
                }
            }
        }
        if (count < 0) {
            printf("error in poll()\n");
            exit(0);
        } 
    }  
}