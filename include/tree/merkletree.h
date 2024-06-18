#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
#include "chk/pkgchk.h"
#include <unistd.h>
#include <stdbool.h>

#define SHA256_HEXLEN (65)

struct merkle_tree_node {
    int search;
    void* key;
    int value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf;
    char expected_hash[SHA256_HEXLEN];
    char computed_hash[SHA256_HEXLEN];
};


struct merkle_tree {
    struct merkle_tree_node* root;
    size_t n_nodes;
};

struct merkle_tree* build_merkle_tree(struct bpkg_obj* obj);

struct merkle_tree_node* build_merkle_tree_structure(struct chunk* chunks, int start, int end, int depth);

struct merkle_tree_node* find_node(struct merkle_tree* tree,char* hash);

void free_merkle_tree_node(struct merkle_tree_node* node);

void free_merkle_tree(struct merkle_tree* tree);

void add_hashes(struct merkle_tree_node* node, struct bpkg_query* qry, size_t max_hashes);

void collect_chunk_hashes(struct merkle_tree_node* node, struct bpkg_query* qry);

void assign_hashes_bfs(struct merkle_tree* tree, char** hashes, int* hash_index);

void get_processing(struct merkle_tree* tree, struct bpkg_query* qry);

bool check_and_mark_leaf_hashes(struct merkle_tree_node* node);

void reset_search_flag(struct merkle_tree_node* node);

void add_computed_to_tree(struct merkle_tree* tree, struct bpkg_query* temp);
#endif