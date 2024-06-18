// echo client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0] );
        return -1;
    }

    int port;
    errno=0;
    port = atoi(argv[1]);
    if (errno) {
        fprintf(stderr, "Failed to convert %s to int\n", argv[1]);
        return -1;
    }

    struct sockaddr_in serv_addr;
    int sock = 0;

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    printf("Connected to server. Type 'exit' to quit.\n");

    char buffer[BUFFER_SIZE];
    printf("Enter message: ");
    char *s = fgets(buffer, BUFFER_SIZE, stdin);
    if (NULL == s) {
        fprintf(stderr, "failed to read input from stdin\n");
        return 1;
    }
    buffer[strlen(buffer)] = '\0';

    // Send message to server
    ssize_t nwritten = send(sock, buffer, strlen(buffer), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to server: socket fd %d\n", sock);
        close(sock);
        sock = 0;
        return 2; // fail
    }

    // read message
    ssize_t nread = read(sock, buffer, BUFFER_SIZE);
    if (nread <= 0) {
        fprintf(stderr, "Server disconnected: socket fd %d\n", sock);
        close(sock);
        return 3; // failed, but could be good reasons to try again!
    }
    printf("Server response: %s\n", buffer);


    close(sock); // Close the socket
    return 0;
}
