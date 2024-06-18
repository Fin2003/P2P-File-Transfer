#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <chk/pkgchk.h>
#include <crypt/sha256.h>
#include <tree/merkletree.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> 
#include <sys/mman.h>
#include <time.h>
// PART 1

/**
 * Loads the package for when a valid path is given
 */

struct bpkg_obj* bpkg_load(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return NULL;
    }

    char *file_in_memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_in_memory == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct bpkg_obj* obj = malloc(sizeof(struct bpkg_obj));
    if (!obj) {
        munmap(file_in_memory, st.st_size);
        close(fd);
        
        return NULL;
    }

    // Parse the file in memory
    char *bpkg = file_in_memory;
    //check the ident and filename over size
    char *buffer=malloc(2048);
    if(!buffer){
        close(fd);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }

    if (sscanf(bpkg, "ident:%[^\n]", buffer) != 1  || strlen(buffer)>1024) {
        close(fd);
        free(buffer);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }else{
        sscanf(bpkg, "ident:%[^\n]", obj->ident);
    }

    bpkg = strchr(bpkg, '\n') + 1;

    if (sscanf(bpkg, "filename:%[^\n]",buffer) != 1  || strlen(buffer)>256) {
        close(fd);
        free(buffer);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }else{
        sscanf(bpkg, "filename:%[^\n]", obj->filename);
    }
    free(buffer);


    bpkg = strchr(bpkg, '\n') + 1;

    sscanf(bpkg, "size:%u\n", &obj->size);
    bpkg = strchr(bpkg, '\n') + 1;

    sscanf(bpkg, "nhashes:%u\n", &obj->nhashes);
    bpkg = strchr(bpkg, '\n') + 1;
    if(obj->nhashes>obj->size || obj->nhashes==0){
        close(fd);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }

    bpkg = strchr(bpkg, '\n') + 1; // Skip the "hashes:" line
    obj->hashes = malloc(sizeof(char*) * obj->nhashes);
    for (int i = 0; i < obj->nhashes; i++) {
        if (strncmp(bpkg, "nchunks:", 8) == 0) {//save the hashes
        for (int j = 0; j < i; j++) {
            free(obj->hashes[j]);
            obj->hashes[j]=NULL;
        }
        free(obj->hashes);
        close(fd);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }
        obj->hashes[i] = malloc(sizeof(char) * 65);
        sscanf(bpkg, "%64s\n", obj->hashes[i]);
        bpkg = strchr(bpkg, '\n') + 1;
    }

    if (strncmp(bpkg, "nchunks:", 8) == 0) {
    sscanf(bpkg, "nchunks:%u\n", &obj->nchunks);//save the number of chunks
    } else {
        for (int j = 0; j < obj->nhashes; j++) {
            free(obj->hashes[j]);
            obj->hashes[j]=NULL;
        }
        free(obj->hashes);
        close(fd);
        bpkg_obj_init(obj);
        bpkg_obj_destroy(obj);
        return NULL;
    }

    bpkg = strchr(bpkg, '\n') + 1;

    bpkg = strchr(bpkg, '\n') + 1; // Skip the "chunks:" line
    obj->chunks = malloc(sizeof(struct chunk) * obj->nchunks);
    for (int i = 0; i < obj->nchunks; i++) {
        sscanf(bpkg, "%64s,%u,%u\n", obj->chunks[i].hash, &obj->chunks[i].offset, &obj->chunks[i].size);//save the chunks
        char* nextLine = strchr(bpkg, '\n');
        if (nextLine == NULL && i != obj->nchunks-1) {
            for (int j = 0; j < obj->nhashes; j++) {
                free(obj->hashes[j]);
                obj->hashes[j]=NULL;
                }
                free(obj->hashes);
                if (obj->chunks != NULL) {
                    free(obj->chunks);
                }
                bpkg_obj_init(obj);
                bpkg_obj_destroy(obj);
                return NULL;
            }
        bpkg = nextLine + 1;
   }


    munmap(file_in_memory, st.st_size);
    close(fd);
    obj->tree = build_merkle_tree(obj);
    if(obj->tree==NULL){
        bpkg_obj_destroy(obj);
        return NULL;
    }

    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg){
    struct bpkg_query qry = { 0 };
    qry.len = 1;
    qry.hashes = malloc(sizeof(char*)*qry.len);

    FILE *file = fopen(bpkg->filename, "rb");
    if(file!=NULL){
        //Check the size of file
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fclose(file);
        if(size==bpkg->size){//check the size of the file
            qry.hashes[0] = strdup("File Exists");
        }else{
            qry.hashes[0] = strdup("File Size Mismatch");
        }
    }else{
        file = fopen(bpkg->filename, "w+");
        if (file != NULL) {
            srand((unsigned)time(NULL));
            for (size_t i = 0; i < bpkg->size; ++i) {
                // Write a random byte to the file
                unsigned char random_byte = rand() % 256;
                if (fwrite(&random_byte, 1, 1, file) != 1) {
                    printf("Error writing to file\n");
                    break;
                }
            }
            fclose(file);
            qry.hashes[0] = strdup("File Created");
        } else {
            printf("File creation failed\n");
        }
    }
    return qry;
}

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = {0};
    
    // Allocate memory for hashes
    qry.hashes = malloc(sizeof(char*) * bpkg->tree->n_nodes);
    if (qry.hashes == NULL) {
        // handle error
        return qry;
    }

    // Fill the hashes
    add_hashes(bpkg->tree->root, &qry, bpkg->tree->n_nodes);
    return qry;
}


/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
    
    FILE *file = fopen(bpkg->filename,"r");
    if (file == NULL) {
        perror("Error opening file");
        return qry;
    }

    int SHA256_BFLEN = bpkg->chunks[0].size;
    if (SHA256_BFLEN == 0) {
        fprintf(stderr, "Error: SHA256_BFLEN is zero\n");
        return qry;
    }

    char *buf = malloc(bpkg->size);
    if (buf == NULL) {
        perror("Error allocating memory");
        return qry;
    }

    struct sha256_compute_data cdata = {0};
    size_t nbytes = 0;
    uint8_t hashout[SHA256_INT_SZ];
    char final_hash[65] = { 0 };
    // Initialize the hashes array in the query structure
    qry.hashes = malloc(sizeof(char*) * bpkg->nchunks);
    qry.len = 0;// Initialize the length of the query structure
    sha256_compute_data_init(&cdata);
    for(int j=0;j<bpkg->nchunks;j++){
        nbytes = fread(buf, 1, bpkg->chunks[j].size, file);
        if(nbytes>0){
            sha256_update(&cdata, buf, nbytes);
            sha256_finalize(&cdata, hashout);
            sha256_output_hex(&cdata, final_hash);
            // Add the hash to the query structure
            for (int i = 0; i < bpkg->nchunks; i++) {
                if (strcmp(bpkg->chunks[i].hash, final_hash) == 0) {;
                    qry.hashes[qry.len] = malloc(sizeof(char) * 65);
                    strcpy(qry.hashes[qry.len], bpkg->chunks[i].hash);
                    qry.len++;
                }
            }

            // Reinitialize the sha256_compute_data structure for the next chunk
            sha256_compute_data_init(&cdata);
        }
    }
    free(buf);
    return qry;
}


/**
 * Gets the mininum of hashes to represented the current completion state
 * Example: If chunks representing start to mid have been completed but
 * 	mid to end have not been, then we will have (N_CHUNKS/2) + 1 hashes
 * 	outputted
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    qry.hashes = malloc(bpkg->tree->n_nodes * sizeof(char*));
    //temp will be used to store the result of compute all the chunks from the file
    struct bpkg_query temp = bpkg_get_completed_chunks(bpkg);
    add_computed_to_tree(bpkg->tree, &temp);//add the computed hashes to the tree
    get_processing(bpkg->tree, &qry);//add the top level of matching hashes to the query
    bpkg_query_destroy(&temp);  // Destroy temp
    return qry;
}


/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash) {
    struct bpkg_query qry = { 0 };
    qry.hashes = malloc(sizeof(char*) * bpkg->tree->n_nodes);
    qry.len = 0;
    char* new_hash = malloc((64 + 1) * sizeof(char));
    strncpy(new_hash, hash, 64);
    new_hash[64] = '\0'; 
    // Find the node with the given hash
    struct merkle_tree_node* start_node = find_node(bpkg->tree, new_hash);
    // Collect all chunk hashes from the start node
    collect_chunk_hashes(start_node, &qry);
    free(new_hash); 
    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    if (qry) {
        if (qry->hashes) {
            for (size_t i = 0; i < qry->len; i++) {
                free(qry->hashes[i]);
            }
            free(qry->hashes);
        }
    }
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    free_merkle_tree(obj->tree);
    if (obj) {
        if (obj->hashes) {
            for (uint32_t i = 0; i < obj->nhashes; i++) {
                free(obj->hashes[i]);
            }
            free(obj->hashes);
        }
        if (obj->chunks) {
            free(obj->chunks);
        }
        free(obj);
    }
}

void bpkg_obj_init(struct bpkg_obj* obj){
    obj->ident[0]='\0';
    obj->filename[0]='\0';
    obj->size=0;
    obj->nhashes=0;
    obj->nchunks=0;
    obj->hashes=NULL;
    obj->chunks=NULL;
    obj->tree=NULL;
}