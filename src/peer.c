#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/packet.h>
#include <stdbool.h> 

struct peer_node* peers = NULL;
int max_peers = 0;
int current_peers = 0;

//create a new node
void create_node(int sockfd, struct sockaddr_in serv_addr){
    struct peer_node* new_node = malloc(sizeof(struct peer_node));
    new_node->peer.sockfd = sockfd;
    new_node->peer.address = serv_addr;
    new_node->next = NULL;

    if (peers == NULL) {
        peers = new_node;
    } else {
        //traverse to the end of the list
        struct peer_node* current = peers;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }

    current_peers++;
}

//print the peer address
void print_peer_addresses() {
    if (peers == NULL) {
        return;
    }
    char addr_str[INET_ADDRSTRLEN];
    struct peer_node* current = peers;
    int i = 1;
    printf("Connected to:\n\n");
    while (current != NULL) {
        inet_ntop(AF_INET, &(current->peer.address.sin_addr), addr_str, INET_ADDRSTRLEN);
        printf("%d. %s:%d\n", i, addr_str, ntohs(current->peer.address.sin_port));
        current = current->next;
        i++;
    }
}

void set_max_peers(int max) {
    max_peers = max;
}

//traverse the list and find the peer, then sent a message to the server means close the connection
void disconnect(const char* ip_port){
    char* ip_port_copy = strdup(ip_port);
    char* ip = strtok(ip_port_copy, ":");
    char* port_str = strtok(NULL, ":");
    if (ip == NULL || port_str == NULL) {
        printf("Missing address and port argument.\n");
        free(ip_port_copy);
        return;
    }
    int port = atoi(port_str);

    // Find the peer in the list
    struct peer_node* current = peers;
    struct peer_node* prev = NULL;
    int found = 0;
    while (current != NULL) {
        char current_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(current->peer.address.sin_addr), current_ip, INET_ADDRSTRLEN);
        if (strcmp(current_ip, ip) == 0 && ntohs(current->peer.address.sin_port) == port) {
            send_DSN(current->peer.sockfd);//send the message to the server
            if (prev == NULL) {
                peers = current->next;
            } else {
                prev->next = current->next;
            }
            close(current->peer.sockfd);
            free(current);
            current_peers--;
            printf("Disconnected from peer\n");
            found = 1;
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!found) {
        printf("Unknown peer, not connected\n");
    } else if (peers == NULL) {
        printf("Not connected to any peers\n");
    }
    free(ip_port_copy);
}

//free the packet list
void cleanup_peers() {
    struct peer_node* current = peers;
    while (current != NULL) {
        close(current->peer.sockfd);
        struct peer_node* temp = current;
        current = current->next;
        free(temp);
    }
    peers = NULL;
    current_peers = 0;
}

//test the connection of the peer
void test_and_cleanup_peers() {
    struct peer_node* current = peers;
    struct peer_node* prev = NULL;
    while (current != NULL) {
        // Test the current peer connection
        bool connected = test_peer_connection(current);
        if (!connected) {
            //try to reconnect
            if(reconnect_peer_node(current)==0){
                prev = current;
                current = current->next;
                continue;
            }
            // If the reconnection test failed, close the socket and remove the node from the list
            close(current->peer.sockfd);
            if (prev == NULL) {
                peers = current->next;
                free(current);
                current = peers;
            } else {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
            current_peers--;
        } else {
            prev = current;
            current = current->next;
        }
    }
}

//check the node by the ip and port
int check_node(const char* ip, int port){
    if (current_peers >= max_peers) {
        printf("Maximum number of peers reached\n");
        return -1;
    }
    
    // Check if already connected to this peer
    struct peer_node* current = peers;
    while (current != NULL) {
        char current_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(current->peer.address.sin_addr), current_ip, INET_ADDRSTRLEN);
        if (strcmp(current_ip, ip) == 0 && ntohs(current->peer.address.sin_port) == port) {
            printf("Already connected to peer\n");
            return -1;
        }
        current = current->next;
    }
    return 0;
}

//find the peer node by the ip and port
struct peer_node* find_peer_node(const char* ip_port){
    char* ip_port_copy = strdup(ip_port);
    char* ip = strtok(ip_port_copy, ":");
    char* port_str = strtok(NULL, ":");
    int port = atoi(port_str);
    if (ip == NULL || port_str == NULL) {
        printf("Missing address and port argument.\n");
        free(ip_port_copy);
        return NULL;
    }
    
    
    struct peer_node* current = peers;
    while (current != NULL) {
        char current_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(current->peer.address.sin_addr), current_ip, INET_ADDRSTRLEN);
        if (strcmp(current_ip, ip) == 0 && ntohs(current->peer.address.sin_port) == port) {
            free(ip_port_copy);
            return current;
        }
        current = current->next;
    }
    free(ip_port_copy);
    return NULL;
}

// Attempt to reconnect a peer_node
int reconnect_peer_node(struct peer_node* node) {
    if (node == NULL) {
        return -1; // Invalid node
    }

    // Close the old socket if it was open
    if (node->peer.sockfd >= 0) {
        close(node->peer.sockfd);
    }

    // Create a new socket
    node->peer.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (node->peer.sockfd < 0) {
        return -1; // Socket creation failed
    }

    // Attempt to connect using the new socket
    if (connect(node->peer.sockfd, (struct sockaddr*)&node->peer.address, sizeof(node->peer.address)) < 0) {
        close(node->peer.sockfd); // Close the socket on failure
        node->peer.sockfd = -1; // Mark as closed
        return -1; // Connection attempt failed
    }
    // Connection successful
    // Send to server
    struct btide_packet* packet = malloc(sizeof(struct btide_packet));
    packet->msg_code=PKT_MSG_PNG;
    ssize_t nwritten = send(node->peer.sockfd, packet, sizeof(struct btide_packet), 0);
    if (nwritten <= 0) {
        free(packet);
        return -1;
    }
    // Receive from server
    ssize_t nread = read(node->peer.sockfd, packet, sizeof(struct btide_packet));
    if (nread <= 0) {
        close(node->peer.sockfd);
        free(packet);
        return -1;
    }
    if(packet->msg_code==PKT_MSG_POG){
        free(packet);
        return 0;
    }else{
        free(packet);
        return -1;
    }
    free(packet);
    return -1;
}