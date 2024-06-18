// echo server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>


#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void sigint_handler(int signum) {
    printf("\nReceived SIGINT (Ctrl + C). Quitting...\n");
    exit(signum); // Exit the program with the signal number
}

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0] );
        return -1;
    }

    // allow quit via ctrl-C
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Error registering signal handler for SIGINT");
        return 1;
    }

    int port;
    errno=0;
    port = atoi(argv[1]);
    if (errno) {
        fprintf(stderr, "Failed to convert %s to int\n", argv[1]);
        return -1;
    }

    int server_fd; 
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    

    // inf loop
    int new_socket;
    while (1) {
        printf("waiting for connections\n");

        socklen_t addrlen = sizeof(struct sockaddr);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        printf("New connection, socket fd is %d, IP is : %s, port : %d\n",
               new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        // read message
        char buffer[BUFFER_SIZE];
        ssize_t nread = read(new_socket, buffer, BUFFER_SIZE);
        if (nread <= 0) {
            fprintf(stderr, "Client disconnected: socket fd %d\n", new_socket);
            close(new_socket);
            new_socket = 0;
            break; // failed
        }

        // send message back
        buffer[nread] = '\0';  // Null-terminate the received message
        printf("Received: %s\n", buffer);
        ssize_t nwritten;
        nwritten = send(new_socket, buffer, strlen(buffer), 0);
        if (nwritten <= 0) {
            fprintf(stderr, "could not write entire message to client: socket fd %d\n", new_socket);
            close(new_socket);
            new_socket = 0;
            break; // failed (but there could be a good reason to try again!)
        }
    }

    // resources no longer needed
    if (new_socket > 0)
        close(new_socket);
    close(server_fd);

    return 0;
}

