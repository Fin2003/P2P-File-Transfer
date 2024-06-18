#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <chk/pkgchk.h>
#include <crypt/sha256.h>
#include <tree/merkletree.h>
#include <stdbool.h>

struct merkle_tree_node* build_merkle_tree_structure(struct chunk* chunks, int start, int end, int depth) {
    struct merkle_tree_node* node = malloc(sizeof(struct merkle_tree_node));
    if (node == NULL) {
        // handle error
        return NULL;
    }
    // Initialize the node
    node->value = depth;
    node->search = 0;
    node->computed_hash[0] = '\0';
    node->expected_hash[0] = '\0';

    if (start == end) { 
        // This is a leaf node
        node->key = NULL;
        node->key = &chunks[start];
        node->left = NULL;
        node->right = NULL;
        node->is_leaf = 1;
    } else {
        // This is an internal node
        node->key=0;
        node->is_leaf = 0;
        int mid = start + (end - start) / 2;
        node->left = build_merkle_tree_structure(chunks, start, mid, depth + 1);
        node->right = build_merkle_tree_structure(chunks, mid + 1, end, depth + 1);
    }

    return node;
}

void assign_hashes_bfs(struct merkle_tree* tree, char** hashes, int* hash_index) {
    // Initialize the queue
    struct merkle_tree_node** queue = malloc(tree->n_nodes * sizeof(struct merkle_tree_node*));
    // Add the root node to the queue
    queue[0] = tree->root;
    int front = 0;
    int rear = 1;
    // Iterate over each node and assign the expected hash
    while (front < rear) {
        struct merkle_tree_node* node = queue[front++];
        if (node->is_leaf==1) {
            strncpy(node->expected_hash, ((struct chunk*)node->key)->hash, SHA256_HEXLEN);
            node->expected_hash[SHA256_HEXLEN] = '\0';  
        } else {
            strncpy(node->expected_hash, hashes[*hash_index], SHA256_HEXLEN);
            node->expected_hash[SHA256_HEXLEN] = '\0';  
            (*hash_index)++;
            queue[rear++] = node->left;
            queue[rear++] = node->right;
        }
    }

    free(queue);
}

struct merkle_tree* build_merkle_tree(struct bpkg_obj* obj) {
    if(obj->nchunks-obj->nhashes!=1){
        return NULL;
    }
    int hash_index = 0;
    struct merkle_tree* tree = malloc(sizeof(struct merkle_tree));
    if (tree == NULL) {
        // handle error
        return NULL;
    }
    // Initialize the tree
    tree->n_nodes = obj->nchunks + obj->nhashes;
    tree->root = build_merkle_tree_structure(obj->chunks, 0, obj->nhashes, 0);//build the tree structure from the bottom to the top
    assign_hashes_bfs(tree, obj->hashes, &hash_index);//assign the hashes to the tree

    return tree;
}

// Function to free the memory allocated for a merkle tree node
void free_merkle_tree_node(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }

    free_merkle_tree_node(node->left);
    free_merkle_tree_node(node->right);

    free(node);
}

// Function to free the memory allocated for a merkle tree
void free_merkle_tree(struct merkle_tree* tree) {
    if (tree == NULL) {
        return;
    }

    free_merkle_tree_node(tree->root);

    free(tree);
}

//add alls hashes to the query
void add_hashes(struct merkle_tree_node* node, struct bpkg_query* qry, size_t max_hashes) {
    if (node == NULL || qry->len >= max_hashes) {
        return;
    }

    // Add the expected hash of the current node
    qry->hashes[qry->len] = malloc(SHA256_HEXLEN + 1); 
    strncpy(qry->hashes[qry->len], node->expected_hash, SHA256_HEXLEN); 
    qry->hashes[qry->len][SHA256_HEXLEN] = '\0';  
    qry->len++;

    // Recursively add the hashes of the left and right children
    add_hashes(node->left, qry, max_hashes);
    add_hashes(node->right, qry, max_hashes);
}

//collect the hashes of the chunks hash will connect to the node
void collect_chunk_hashes(struct merkle_tree_node* node, struct bpkg_query* qry) {
    // Base case: if the node is NULL, return
    if (node == NULL) {
        return;
    }

    // If the node is a leaf node, add its expected_hash to the qry
    if (node->is_leaf == 1) {
        qry->hashes[qry->len] = malloc(SHA256_HEXLEN + 1);
        strncpy(qry->hashes[qry->len], node->expected_hash, SHA256_HEXLEN);
        qry->hashes[qry->len][SHA256_HEXLEN] = '\0';
        qry->len++;
    } else {
        // If the node is not a leaf node, recursively call the function on its children
        collect_chunk_hashes(node->left, qry);
        collect_chunk_hashes(node->right, qry);
    }
}

//return the node with the hash
struct merkle_tree_node* find_node(struct merkle_tree* tree,char* hash){

    // Initialize the queue
    struct merkle_tree_node** queue = malloc(tree->n_nodes * sizeof(struct merkle_tree_node*));
    if (queue == NULL) {
        return NULL;
    }
    // Add the root node to the queue
    queue[0] = tree->root;
    int front = 0;
    int rear = 1;

    // Iterate over each node
    while (front < rear) {
        // Dequeue a node
        struct merkle_tree_node* node = queue[front++];

        if(strcmp(node->expected_hash, hash) == 0){
            free(queue);  // Free the queue before returning
            return node;
        }

        // Enqueue the left and right child nodes
        if (node->left != NULL) {
            queue[rear++] = node->left;
        }
        if (node->right != NULL) {
            queue[rear++] = node->right;
        }
    }
    free(queue);

    return NULL;
}

//return the node that all the leaf node are correct
void get_processing(struct merkle_tree* tree, struct bpkg_query* qry) {
    // Create a queue and enqueue the root
    struct merkle_tree_node** queue = malloc(tree->n_nodes * sizeof(struct merkle_tree_node*));
    int front = 0;
    int rear = 1;
    queue[0] = tree->root;
    while (front < rear) {
        struct merkle_tree_node* node = queue[front++];
        if(node->search==0 && check_and_mark_leaf_hashes(node)){
            qry->hashes[qry->len] = malloc(SHA256_HEXLEN + 1);
            strcpy(qry->hashes[qry->len], node->expected_hash);
            qry->len++;
        }

        // Enqueue the left and right children
        if (node->left != NULL) {
            queue[rear++] = node->left;
        }
        if (node->right != NULL) {
            queue[rear++] = node->right;
        }
    }
    if(qry->len==0){
        qry->hashes[qry->len]=NULL;
    }
    reset_search_flag(tree->root);
    free(queue);
}

//check if all the leaf nodes connect to node have same expect and compte hash
bool check_and_mark_leaf_hashes(struct merkle_tree_node* node) {
    if (node == NULL) {
        return true;
    }

    if (node->is_leaf == 1) {
        bool match = (strcmp(node->computed_hash, node->expected_hash) == 0);
        if (match) {
            node->search = 1;
        }
        return match;
    }

    bool left_match = check_and_mark_leaf_hashes(node->left);
    bool right_match = check_and_mark_leaf_hashes(node->right);

    if (left_match && right_match) {
        node->search = 1;
        return true;
    } else {
        // Reset search flag for all nodes in the subtree
        reset_search_flag(node);
        return false;
    }
}

//reset the search flag of all the children of the node
void reset_search_flag(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }

    node->search = 0;
    reset_search_flag(node->left);
    reset_search_flag(node->right);
}

//add the computed hash to the leaf node
void add_computed_to_tree(struct merkle_tree* tree, struct bpkg_query* temp) {
    struct merkle_tree_node** queue = malloc(tree->n_nodes * sizeof(struct merkle_tree_node*));
    int front = 0;
    int rear = 1;
    queue[0] = tree->root;
    int index = 0;
    int test = 0;
    while (front < rear) {
        if(index >= temp->len){
            break;
        }
        struct merkle_tree_node* node = queue[front++];
        if(node->is_leaf == 1){
            for (size_t i = 0; i < temp->len; i++) {
                if (strcmp(temp->hashes[i], node->expected_hash) == 0) {
                    strncpy(node->computed_hash, node->expected_hash, SHA256_HEXLEN);
                    node->computed_hash[SHA256_HEXLEN] = '\0'; 
                    test++;
                }
            }
        }

        if (node->left != NULL) {
            queue[rear++] = node->left;
        }
        if (node->right != NULL) {
            queue[rear++] = node->right;
        }
    }
    free(queue);
}

