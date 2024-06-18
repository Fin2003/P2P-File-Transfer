#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <chk/pkgchk.h>
#include "config.c"
#include "peer.c"
#include "package.c"
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ftw.h>
#include <arpa/inet.h>
#include <net/packet.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
//
// PART 2
//

volatile int server_running = 1;
struct obj_query* obj_list;
struct obj_query {
    struct bpkg_obj** objs;
    int size;
};
u_int16_t listen_port;

//manage the first time connection, and keep it.
void* handle_connection(void* arg) {
    while(server_running){
        int sock;
        struct sockaddr_in server_addr;
        char* ip_port_copy = (char*)arg;
        char* ip = strtok(ip_port_copy, ":");
        char* port_str = strtok(NULL, ":");
        int port = atoi(port_str);
        if (ip == NULL || port_str == NULL) {
            printf("Missing address and port argument.\n");
            pthread_exit(NULL);
        }
        if(check_node(ip, port)==-1){
            pthread_exit(NULL);
        }
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            close(sock);
            pthread_exit(NULL);
        }
        
        
        //set the server address and port
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
            printf("Invalid address/ Address not supported\n");
            close(sock);
            pthread_exit(NULL);
        }
        //connect to the server
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            printf("Unable to connect to request peer\n");
            close(sock);
            pthread_exit(NULL);
        }
        
        //send the ACP packet
        if(send_ACK(sock)){
            create_node(sock, server_addr);
        }
        pthread_exit(NULL);
    }
    pthread_exit(NULL);
}

void* client_listen(void* arg){
    int client_fd;
    Config* config = (Config*)arg;
    // Set the client listen port
    chdir(config->directory);//change the directory to the config directory
    listen_port=config->port;
    struct sockaddr_in client_addr;
    //create socket
    if((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket creation failed");
        free_config(config);
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
    //set the server address
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(listen_port);
    //bind the socket to client, so that it can listen for incoming connections
    if (bind(client_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("listen bind failed");
        close(client_fd);
        free_config(config);
        exit(EXIT_FAILURE);
    }
    //listen for incoming connections
    if (listen(client_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        close(client_fd);
        free_config(config);
        exit(EXIT_FAILURE);
    }
    int conn_fd;
    struct sockaddr_in client_conn_addr; 
    socklen_t client_conn_addr_len = sizeof(client_conn_addr);
    while(server_running){//if the server not running, exit the loop and close the socket
        // Accept incoming connections
        conn_fd = accept(client_fd, (struct sockaddr *)&client_conn_addr, &client_conn_addr_len);
        if (conn_fd < 0) {
            perror("accept failed");
            continue; 
        }
        while(server_running){
            //process the incoming connection with packet
            struct btide_packet packet;
            ssize_t nread = read(conn_fd, &packet, sizeof(struct btide_packet));
            if (nread <= 0) {
                close(conn_fd);
                break;
            }
                if(packet.msg_code == PKT_MSG_ACK){
                    //the connection is established
                    break;
                }
                //a new connection is established, send back a ACP packet(CONNECT)
                if(packet.msg_code == 1122){
                    packet.msg_code=PKT_MSG_ACP;
                }

                //if the packet is an PKT_MSG_PNG, send back an PKT_MSG_POG(PEERS)
                if(packet.msg_code == PKT_MSG_PNG) {
                    packet.msg_code = PKT_MSG_POG;
                }

                //if the packet is a DSN, close the connection(DISCONNECT)
                if(packet.msg_code==PKT_MSG_DSN){
                    close(conn_fd);
                    free(config);
                    break;
                }
                //if the packet is a REQ, send back a RES (FETCH)
                if(packet.msg_code==PKT_MSG_REQ){
                    packet.msg_code=PKT_MSG_RES;
                    //get the REQ information from the packet
                    uint32_t req_file_offset;
                    memcpy(&req_file_offset, packet.pl.data, sizeof(req_file_offset));
                    uint32_t req_data_len;
                    memcpy(&req_data_len, packet.pl.data+sizeof(req_file_offset), sizeof(req_data_len));
                    char req_chunk_hash[65];
                    memcpy(req_chunk_hash, packet.pl.data+sizeof(req_file_offset)+sizeof(req_data_len), 65);
                    req_chunk_hash[64]='\0';
                    char req_identifier[1025];
                    memcpy(req_identifier, packet.pl.data+sizeof(req_file_offset)+sizeof(req_data_len)+65, 1025);
                    req_identifier[1024]='\0';

                    //insert the REQ information into the RES packet
                    struct bpkg_obj* obj;
                    for(int i=0; i<obj_list->size; i++){
                        //check if the req identifier is in the package
                        if(strstr(obj_list->objs[i]->ident, req_identifier)!=NULL){
                            obj=obj_list->objs[i];
                            for(int j=0; j<obj->nhashes; j++){
                                //check if the req hash is in the package
                                if(strcmp(obj->hashes[j], req_chunk_hash)==0){
                                    //get the chunk from the file
                                    int fd = open(obj->filename, O_RDONLY);
                                    if (fd == -1) {
                                        perror("open");
                                        obj=NULL;
                                        packet.error=1;
                                        break;
                                    }
                                    struct stat sb;
                                    if (fstat(fd, &sb) == -1) {
                                        perror("fstat");
                                        obj=NULL;
                                        packet.error=1;
                                        break;
                                    }
                                    char* file_data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                                    if (file_data == MAP_FAILED) {
                                        perror("mmap");
                                        obj=NULL;
                                        packet.error=1;
                                        break;
                                    }
                                    
                                    //send the chunk to the client, if oversize, send in multiple packets
                                    uint32_t sent_bytes=0;
                                    while(sent_bytes<req_data_len){
                                        size_t offset=0;
                                        struct btide_packet packet_RES;
                                        memcpy(packet_RES.pl.data, &req_file_offset, sizeof(req_file_offset));
                                        offset+=sizeof(req_file_offset);
                                        uint32_t remain_bytes=req_data_len-sent_bytes;
                                        uint32_t chunk_size; //size of the chunk to be sent
                                        if(remain_bytes>2998){
                                            memcpy(packet_RES.pl.data+offset,file_data+req_file_offset+sent_bytes, 2998);//save the chunk into data
                                            chunk_size=2998;
                                        }else{
                                            memcpy(packet_RES.pl.data+offset, file_data+req_file_offset+sent_bytes, remain_bytes);
                                            memset(packet_RES.pl.data+offset+remain_bytes, 0, 2998-remain_bytes); // fill the rest with 0s
                                            chunk_size=remain_bytes;
                                        }
                                        sent_bytes+=chunk_size;
                                        offset+=chunk_size;//update the offset
                                        req_file_offset+=chunk_size;//update the file offset
                                        //update  sent_data_len into the excat size of the data sent
                                        uint32_t sent_data_len=chunk_size;
                                        memcpy(packet_RES.pl.data+offset, &sent_data_len, sizeof(sent_data_len));//data_len
                                        offset+=sizeof(sent_data_len);
                                        memcpy(packet_RES.pl.data+offset, req_chunk_hash, 64);//hash
                                        offset+=64;
                                        memcpy(packet_RES.pl.data+offset, req_identifier, 1024);//identifier
                                        //send the packet
                                        ssize_t nwritten = send(conn_fd, &packet_RES, sizeof(struct btide_packet), 0);
                                        if (nwritten <= 0) {
                                            fprintf(stderr, "could not write entire message to server: socket fd %d\n", conn_fd);
                                            close(conn_fd);
                                            break;
                                        }
                                        
                                    }
                                    //close the file
                                    munmap(file_data, sb.st_size);
                                    close(fd);
                                    break;
                                }else{
                                    //hash is not in the package
                                    obj=NULL;
                                    packet.error=1;
                                    break;
                                }
                            }
                            break;
                        }else{
                            //identifier is not in the package
                            obj=NULL;
                            packet.error=1;
                            break;
                        }
                    }
                    continue;
                }
                //sent back the packet
                ssize_t nwritten = send(conn_fd, &packet, sizeof(struct btide_packet), 0);
                if (nwritten <= 0) {
                    fprintf(stderr, "could not write entire message to server: socket fd %d\n", conn_fd);
                    close(conn_fd);
                }
        }


        
    }
    close(client_fd);
    free_config(config);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }
    // Load the config
    Config* config = load_config(argv[1]);
    if (!config) {
        return 1;
    }

    // Create an array to store client threads
    pthread_t *client_thread = (pthread_t *)malloc(config->max_peers * sizeof(pthread_t));
    if (client_thread == NULL) {
        exit(EXIT_FAILURE);
    }

    int peer_count=0;
    // Create a thread to listen for incoming connections
    pthread_t client_listen_thread;
    if(pthread_create(&client_listen_thread, NULL, client_listen, (void*)config)){
        perror("pthread_create");
        free_config(config);
        exit(EXIT_FAILURE);
    }

    char command[5521];
    // Initialize obj_query
    obj_list = malloc(sizeof(struct obj_query));
    obj_list->objs = malloc(sizeof(struct bpkg_obj *));
    obj_list->size = 0;
    set_max_peers(config->max_peers);
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    while (1) {//loop to get the command from the user
        fgets(command, 5521, stdin);
        // Remove trailing newline character
        command[strcspn(command, "\n")] = 0;
        char * command_type = strtok(command, " ");
        char * command_content = strtok(NULL, "");
        //catch the command type and the command content
        if(command_type==NULL && command_content==NULL){
            continue;
        }
        if (strcmp(command_type, "QUIT") == 0) {
            chdir(cwd);
            break;
        }

        if (strcmp(command_type, "ADDPACKAGE") == 0) {
            chdir(config->directory);//change the directory to the config directory
            if(!command_content){
                printf("Missing file argument\n");
                continue;
            }

            //check file's existence
            if (access(command_content, F_OK) == -1) {
                printf("Cannot open file\n");
                continue;
            }
            // Load the package
            struct bpkg_obj* obj = bpkg_load(command_content);
            struct bpkg_query qry1=bpkg_file_check(obj); //check the file
            bpkg_query_destroy(&qry1);
            if (obj==NULL) {
                printf("Unable to parse bpkg file\n");
            }else{
                obj_list->objs = realloc(obj_list->objs, (obj_list->size + 1) * sizeof(struct bpkg_obj *));
                obj_list->objs[obj_list->size] = obj;
                obj_list->size++;
            }
            continue;
        } 
        if(strcmp(command_type, "REMPACKAGE") == 0){
            if(command_content==NULL){
                printf("Missing identifier argument, please specify whole 1024 character or at least 20 characters.\n");
                if(obj_list->size==0){
                    printf("No packages managed\n");
                }
            }else{
                if(strlen(command_content)>1024 || strlen(command_content)<20){//check the length of the identifier
                    printf("Missing identifier argument, please specify whole 1024 character or at least 20 characters.\n");
                }else{
                    for(int i=0; i<obj_list->size; i++){
                        if (strstr(obj_list->objs[i]->ident, command_content) != NULL) {//check if the identifier is in the package
                            bpkg_obj_destroy(obj_list->objs[i]);//destroy the package
                            for(int j=i; j<obj_list->size-1; j++){
                                obj_list->objs[j]=obj_list->objs[j+1];
                            }
                            obj_list->size--;
                            printf("Package has been removed\n");
                            break;
                        }else{
                            printf("Identifier provided does not match managed packages\n");
                        }
                    }
                }
            }
            if(obj_list->size==0){
                printf("No packages managed\n");
            }
            continue;
        }
        if (strcmp(command_type, "PACKAGES") == 0) {
            int index=1;
            char path[1024];
            for (int n = 0; n < obj_list->size; n++) {
                struct bpkg_query qry = bpkg_get_min_completed_hashes(obj_list->objs[n]);//get the minimum completed hashes, if complete, should be the hash of the root
                snprintf(path, sizeof(path), "%s/%s", config->directory, obj_list->objs[n]->filename);
                if(qry.hashes[0]!=NULL && strcmp(obj_list->objs[n]->hashes[0], qry.hashes[0]) == 0){
                    printf("%d. %.32s, %s : %s\n", index, obj_list->objs[n]->ident,path, "COMPLETED");
                }else{
                    printf("%d. %.32s, %s : %s\n", index, obj_list->objs[n]->ident,path, "INCOMPLETE");
                }
                bpkg_query_destroy(&qry);
                index++;
            }
            continue;
        } 
        if (strcmp(command_type, "CONNECT") == 0){
            if(command_content==NULL){
                printf("Missing address and port argument.\n");
                continue;
            }
            pthread_t new_thread;
            if(pthread_create(&new_thread, NULL, handle_connection, (void*)command_content)){//create a new thread to handle the connection
                client_thread[peer_count++] = new_thread;
            }

            continue;
        }else if(strcmp(command_type, "PEERS") == 0){
            //test_and_cleanup_peers();//test the connection and clean up the peers without respond
            print_peer_addresses();
            continue;
        }else if(strcmp(command_type, "DISCONNECT") == 0){
            disconnect(command_content);
            continue;
        }else if(strcmp(command_type, "FETCH") == 0){ 
            if(command_content==NULL){
                printf("Missing identifier argument\n");
                continue;
            }
            char *token;
            char ip_port[256];
            char identifier[1025];
            char hash[65];
            uint32_t offset;
            //parse the command content into ip_port, identifier, hash, and offset
            token = strtok(command_content, " ");
            if(token != NULL) strcpy(ip_port, token);

            token = strtok(NULL, " ");
            if(token != NULL) strcpy(identifier, token);

            token = strtok(NULL, " ");
            if(token != NULL) {
                char *offset_start = strchr(token, '(');
                if(offset_start != NULL) {
                    *offset_start = '\0';
                    offset = (u_int16_t)atoi(offset_start + 1);
                }
                strcpy(hash, token);
            }

            //find the peer node and sock will using
            struct peer_node* temp_peers = NULL;
            temp_peers=find_peer_node(ip_port);
            if(temp_peers==NULL){
                printf("Unable to request chunk, peer not connected\n");
                continue;
            }
            if(strlen(identifier)>1024 || strlen(identifier)<20){
                printf("Unable to request chunk, package is not managed\n");
                }else{
                    for(int i=0; i<obj_list->size; i++){
                        if (strstr(obj_list->objs[i]->ident, identifier) != NULL) {
                            //find the .bpkg file
                            for(int j=0; j<obj_list->objs[i]->nchunks; j++){
                                if(strcmp(obj_list->objs[i]->chunks[j].hash, hash)==0){
                                    //find the chunks
                                    struct btide_packet_list* pack_list;
                                    pack_list=sent_REQ_RES(temp_peers->peer.sockfd, obj_list->objs[i]->chunks[j].offset+offset, obj_list->objs[i]->chunks[j].size, hash, identifier);//send the request to the server
                                    if(pack_list!=NULL){
                                        for(int k=0; k<pack_list->size; k++){
                                            load_package(pack_list->items[k], obj_list->objs[i]);//anaylze the package and load the chunk into local file
                                        }
                                        free_packet_list(pack_list);
                                    }
                                    break;
                                }else{
                                    printf("Unable to request chunk, chunk hash does not belong to package\n");
                                
                                }
                            }
                            break;
                        }else{
                            printf("Unable to request chunk, package is not managed\n");
                        }
                    }
                }
            continue;
        }else{
            printf("Invalid Input\n");
            continue;
        }
    }
    
    server_running=0;
    for(int i=0; i<peer_count; i++){
        pthread_join(client_thread[i], NULL);
    }
    free(client_thread);
    cleanup_peers();
    for (int i = 0; i < obj_list->size; i++) {
        bpkg_obj_destroy(obj_list->objs[i]); 
    }
    free(obj_list->objs);
    free(obj_list);
    return 0;
}
