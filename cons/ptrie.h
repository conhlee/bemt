#ifndef CONS_PTRIE_H
#define CONS_PTRIE_H

// CONS -- patricia trie implementation

#include "type.h"

typedef struct ConsPtrieNode {
    char* key; // Owned by this node. NULL on root node & internal nodes.
    u64 refBit; // (u64)-1 means NPOS.
    struct ConsPtrieNode* left; // Will always be followed on root node.
    struct ConsPtrieNode* right;
} ConsPtrieNode;

typedef struct ConsPtrie {
    u64 nodeCount; // Excludes the root node.

    // The root node is empty. The first node can be found at root->left (if present).
    // Otherwise, root->left will point to the root node.
    ConsPtrieNode* root;
} ConsPtrie;

typedef struct ConsFlatPtrieNode {
    char* key;
    u64 refBit;
    u64 leftIndex;
    u64 rightIndex;
} ConsFlatPtrieNode;

typedef struct ConsFlatPtrie {
    u64 nodeCount; // Excludes the root node.
    ConsFlatPtrieNode* nodes; // First node will be root node.
} ConsFlatPtrie;

// Initialize an empty patricia trie.
void PtrieInit(ConsPtrie* trie);

// Destroy a patricia trie.
void PtrieDestroy(ConsPtrie* trie);
// Destroy a flattened patricia trie.
void PtrieDestroyFlat(ConsFlatPtrie* trie);

// Search for a node by it's key.
ConsPtrieNode* PtrieSearch(ConsPtrie* trie, const char* key);
ConsFlatPtrieNode* PtrieSearchFlat(ConsFlatPtrie* trie, const char* key);

// Insert a node into the patricia trie (the key will be duplicated).
ConsPtrieNode* PtrieInsert(ConsPtrie* trie, const char* key);

// Flatten a patricia trie.
void PtrieFlatten(ConsPtrie* trie, ConsFlatPtrie* flat);

// Swap two flat trie nodes.
void PtrieSwapFlatNode(ConsFlatPtrie* trie, u64 aIdx, u64 bIdx);

#endif // CONS_PTRIE_H
