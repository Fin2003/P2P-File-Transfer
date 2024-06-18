#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/packet.h>
#include <stdbool.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// read ACK and send ACK
bool send_ACK(int sockfd){
    // Send to server
    struct btide_packet* packet = malloc(sizeof(struct btide_packet));
    //tell the server that the client is ready to connect
    packet->msg_code=1122;
    ssize_t nwritten = send(sockfd, packet, sizeof(struct btide_packet), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to server: socket fd %d\n", sockfd);
        close(sockfd);
        free(packet);
        return false;
    }
    // Received status
    ssize_t nread = read(sockfd, packet, sizeof(struct btide_packet));
    if (nread <= 0) {
        fprintf(stderr, "didn't received ACP\n");
        close(sockfd);
        free(packet);
        return false; 
    }
    //packet->msg_code = ntohs(packet->msg_code);
    if(packet->msg_code == PKT_MSG_ACP){
        packet->msg_code=PKT_MSG_ACK;
        ssize_t nwritten = send(sockfd, packet, sizeof(struct btide_packet), 0);
        if (nwritten <= 0) {
            printf("Unable to connect to request peer\n");
            close(sockfd);
            free(packet);
            return false; // fail
        }
        printf("Connection established with peer\n");
        free(packet);
        return true;
    }
    else{
        printf("Unable to connect to request peer\n");
        free(packet);
        return false;
    }
}

// Send DSN to server
void send_DSN(int sockfd){
    //Send code before disconnecting
    struct btide_packet* packet = malloc(sizeof(struct btide_packet));
    memset(packet, 0, sizeof(struct btide_packet));
    packet->msg_code=htons(PKT_MSG_DSN);
    if (write(sockfd, packet, sizeof(struct btide_packet)) < 0) {
        perror("Failed to send disconnection signal");
        close(sockfd);
        free(packet);
        }
    free(packet);
}

// Send PING to server should return PONG
bool test_peer_connection(struct peer_node* node) {
    if (node == NULL) {
        printf("Peer node is NULL\n");
        return false;
    }
    // Send to server
    struct btide_packet* packet = malloc(sizeof(struct btide_packet));
    packet->msg_code=PKT_MSG_PNG;
    ssize_t nwritten = send(node->peer.sockfd, packet, sizeof(struct btide_packet), 0);
    if (nwritten <= 0) {
        printf("could not write entire message to server: socket fd %d\n", node->peer.sockfd);
        free(packet);
        return false;
    }
    // Receive from server
    ssize_t nread = read(node->peer.sockfd, packet, sizeof(struct btide_packet));
    if (nread <= 0) {
        printf("Server didn't respond\n");
        close(node->peer.sockfd);
        free(packet);
        return false;
    }
    if(packet->msg_code==PKT_MSG_POG){
        free(packet);
        return true;
    }else{
        free(packet);
        return false;
    }
    free(packet);
    return true;
}

// Send REQ to server and excpet receive 1 or N * RES
struct btide_packet_list* sent_REQ_RES(int sockfd, uint32_t file_offset, uint32_t data_len, const char* chunk_hash, const char* identifier) {
    struct btide_packet_list* list = malloc(sizeof(struct btide_packet_list));//list of packets
    if (!list) {
        fprintf(stderr, "Memory allocation failed for packet list\n");
        return NULL;
    }
    list->items = NULL;
    list->size = 0;

    struct btide_packet* packet = malloc(sizeof(struct btide_packet));
    if (!packet) {
        fprintf(stderr, "Memory allocation failed for packet\n");
        free(list);
        return NULL;
    }
    packet->msg_code = PKT_MSG_REQ;//preparing request

    size_t offset = 0;
    memcpy(packet->pl.data + offset, &file_offset, sizeof(file_offset));//copying file offset into packet
    offset += sizeof(file_offset);

    memcpy(packet->pl.data + offset, &data_len, sizeof(data_len));//copying data length into packet
    offset += sizeof(data_len);

    memcpy(packet->pl.data + offset, chunk_hash, 64);//copying chunk hash into packet
    offset += 64;

    memcpy(packet->pl.data + offset, identifier, 1024);//copying identifier into packet
    offset += 1024;

    ssize_t nwritten = send(sockfd, packet, sizeof(struct btide_packet), 0);//sending packet to server
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to server: socket fd %d\n", sockfd);
        close(sockfd);
        free(packet);
        free(list);
        return NULL;
    }

    // Receive response, expect 1 or N * RES
    uint32_t num_packets = ceil((double)data_len / 2998.0);//the max size of data in packet is 2998, should recived ceil(data_len/2998) packets
    size_t received_len = 0;
    while (received_len < num_packets) {//use while loop to recive all packets
        struct btide_packet* response_packet = malloc(sizeof(struct btide_packet));
        ssize_t nread = read(sockfd, response_packet, sizeof(struct btide_packet));
        if (nread <= 0) {
            fprintf(stderr, "Server disconnected: socket fd %d\n", sockfd);
            close(sockfd);
            free(response_packet);
            free(packet);
            free(list->items);
            free(list);
            return NULL;
        }
        if (response_packet->msg_code == PKT_MSG_RES) {
            if (response_packet->error == 0) {
                // Dynamically add packet to list
                size_t newSize = list->size + 1;
                struct btide_packet** newItems = realloc(list->items, newSize * sizeof(struct btide_packet*));
                if (!newItems) {
                    fprintf(stderr, "Memory allocation failed for packet items\n");
                    free(response_packet);
                    free(packet);
                    free(list->items);
                    free(list);
                    return NULL;
                }
                newItems[list->size] = response_packet;//add packet to list
                list->items = newItems;
                list->size = newSize;
                }
        } else {
            free(response_packet);
        }
        received_len++;
    }

    free(packet);
    return list;
}

//load packet and write to file
void load_package(struct btide_packet* packet, struct bpkg_obj* obj) {
    uint32_t rec_file_offset;
    memcpy(&rec_file_offset, packet->pl.data, sizeof(rec_file_offset));//get file offset from packet

    uint8_t rec_data[2998]; 
    memcpy(rec_data, packet->pl.data + sizeof(rec_file_offset), 2998);//get data from packet

    uint16_t rec_data_len;
    memcpy(&rec_data_len, packet->pl.data + sizeof(rec_file_offset) + 2998, sizeof(rec_data_len));//get data length from packet

    char rec_chunk_hash[65];
    memcpy(rec_chunk_hash, packet->pl.data + sizeof(rec_file_offset) + 2998 + sizeof(rec_data_len), 64);//get chunk hash from packet
    rec_chunk_hash[64] = '\0';

    char rec_identifier[1025];
    memcpy(rec_identifier, packet->pl.data + sizeof(rec_file_offset) + 2998 + sizeof(rec_data_len) + 64, 1024);//get identifier from packet
    rec_identifier[1024] = '\0';

    //open the file and prepare to write
    int fd = open(obj->filename, O_RDWR);
    if (fd == -1) {
        perror("open");
        return;
    }
    //get size of file
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return;
    }
    //mappe the file
    uint8_t* file_content = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_content == MAP_FAILED) {
        perror("failed to mmap");
        close(fd);
        return;
    }
    //write the data to the file
    memcpy(file_content + rec_file_offset, rec_data, rec_data_len);
    //sync the file
    if (msync(file_content, st.st_size, MS_SYNC) == -1) {
        perror("msync");
        close(fd);
        return;
    }
    //unmap the file
    if (munmap(file_content, st.st_size) == -1) {
        perror("munmap");
        close(fd);
        return;
    }
    //close the file
    close(fd);
}

//find the peer node
void free_packet_list(struct btide_packet_list* list) {
    if (list == NULL) {
        return;
    }

    for (size_t i = 0; i < list->size; i++) {
        free(list->items[i]);
    }

    free(list->items);
    free(list);
}