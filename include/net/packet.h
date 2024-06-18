#ifndef NETPKT_H
#define NETPKT_H

#include <stdint.h>
#include <stdbool.h>

#define PAYLOAD_MAX (4092)

#define PKT_MSG_ACK 0x0c
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00


union btide_payload {
    uint8_t data[PAYLOAD_MAX];

};//save the data

struct btide_packet {
    uint16_t msg_code;
    uint16_t error;
    union btide_payload pl;
};//the packet in TCP fomat

struct btide_packet_list {
    struct btide_packet** items;
    size_t size;
};//save the packet list

struct peer {
    int sockfd;
    struct sockaddr_in address;
};//save the peer information

struct peer_node {
    struct peer peer;
    struct peer_node* next;
};//Single linked list of peers
int reconnect_peer_node(struct peer_node* node);
void free_packet_list(struct btide_packet_list* list);

void load_package(struct btide_packet* packet, struct bpkg_obj* obj);

bool test_peer_connection(struct peer_node* node);

void print_peer_addresses();

void set_max_peers(int max);

void disconnect(const char* ip_port);

void cleanup_peers();

void test_and_cleanup_peers();

int check_node(const char* ip, int port);

bool send_ACK(int sockfd);

bool test_peer_connection(struct peer_node* node);

void send_DSN(int sockfd);

struct peer_node* find_peer_node(const char* ip_port);

struct btide_packet_list* sent_REQ_RES(int sockfd, uint32_t file_offset, uint32_t data_len, const char* chunk_hash, const char* identifier);
#endif
