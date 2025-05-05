#include "ptrie.h"

#include <stdlib.h>

#include <string.h>

#include <stdio.h>

#include "macro.h"

#define REF_BIT_NPOS ((u64)-1)
#define NODE_NPOS ((u64)-1)

ConsPtrieNode* _CreateNode(const char* key) {
    ConsPtrieNode* node = malloc(sizeof(ConsPtrieNode));

    if (key != NULL)
        node->key = strdup(key);
    else
        node->key = NULL;
    node->refBit = REF_BIT_NPOS;
    node->left = node;
    node->right = node;

    return node;
}

// Does not destroy children.
void _DestroyNode(ConsPtrieNode* node) {
    if (node == NULL)  
        return;

    if (node->key != NULL)
        free(node->key);
    free(node);
}

void PtrieInit(ConsPtrie* trie) {
    trie->nodeCount = 0;
    trie->root = malloc(sizeof(ConsPtrieNode));

    trie->root->key = NULL;
    trie->root->refBit = REF_BIT_NPOS;
    trie->root->left = trie->root;
    trie->root->right = trie->root;
}

static void _DestroyRecursive(ConsPtrieNode* node, ConsPtrieNode* parent, ConsPtrieNode* root) {
    if (node == NULL || node == root || node->refBit <= parent->refBit)
        return;

    _DestroyRecursive(node->left, node, root);
    _DestroyRecursive(node->right, node, root);
    _DestroyNode(node);
}

void PtrieDestroy(ConsPtrie* trie) {
    if (trie == NULL || trie->root == NULL)
        return;

    _DestroyRecursive(trie->root->left, trie->root, trie->root);

    _DestroyNode(trie->root);
    trie->root = NULL;
    trie->nodeCount = 0;
}

void PtrieDestroyFlat(ConsFlatPtrie* trie) {
    if (trie == NULL || trie->nodes == NULL)
        return;

    free(trie->nodes);

    trie->nodes = NULL;
    trie->nodeCount = 0;
}

static u32 _ExtractRefBit(const char* key, u32 refBit) {
    u32 len = strlen(key);
    u32 invByteIdx = refBit >> 3;

    if (invByteIdx >= len)
        return 0;

    u32 byteIdx = len - 1 - invByteIdx;
    u32 shift = refBit & 7;

    return ((u8)(key[byteIdx]) >> shift) & 1;
}

static u64 _GetFirstDifferingBit(const char* a, const char* b) {
    u64 aLen = strlen(a);
    u64 bLen = strlen(b);

    u64 maxBits = MAX(aLen, bLen) * 8;

    for (u32 bit = 0; bit < maxBits; bit++) {
        u32 abit = _ExtractRefBit(a, bit);
        u32 bbit = _ExtractRefBit(b, bit);

        if (abit != bbit)
            return bit;
    }

    if (aLen != bLen)
        return MIN(aLen, bLen) * 8;

    return REF_BIT_NPOS;
}

ConsPtrieNode* PtrieSearch(ConsPtrie* trie, const char* key) {
    ConsPtrieNode* node = trie->root->left;
    ConsPtrieNode* prevNode = node;

    while (1) {
        prevNode = node;

        u32 bit = _ExtractRefBit(key, node->refBit);
        node = bit ? node->right : node->left;

        if (node == trie->root)
            return NULL;
        if (node->refBit <= prevNode->refBit)
            break;
    }

    if (strcmp(key, node->key) == 0)
        return node;
    else
        return NULL;
}

ConsPtrieNode* PtrieInsert(ConsPtrie* trie, const char* key) {
    if (trie->root == NULL) {
        // Trie is uninitialized.
        return NULL;
    }

    if (trie->root->left == trie->root) {
        trie->root->left = _CreateNode(key);
        trie->nodeCount++;
        return trie->root->left;
    }

    ConsPtrieNode* node = trie->root->left;
    ConsPtrieNode* parent;

    do {
        parent = node;
        u32 bit = _ExtractRefBit(key, node->refBit);
        node = bit ? node->right : node->left;
    } while (node->refBit > parent->refBit);

    // Node already exists.
    if (strcmp(key, node->key) == 0)
        return node;

    u64 diffBit = _GetFirstDifferingBit(key, node->key);

    node = trie->root->left;
    ConsPtrieNode* prev = NULL;
    while (node->refBit < diffBit && (prev == NULL || node->refBit > prev->refBit)) {
        prev = node;
        u32 bit = _ExtractRefBit(key, node->refBit);
        node = bit ? node->right : node->left;
    }

    ConsPtrieNode* newLeaf = _CreateNode(key);
    trie->nodeCount++;

    ConsPtrieNode* newNode = _CreateNode(NULL);
    trie->nodeCount++;

    newNode->refBit = diffBit;

    u32 keyBit = _ExtractRefBit(key, diffBit);
    if (keyBit) {
        newNode->left = node;
        newNode->right = newLeaf;
    }
    else {
        newNode->left = newLeaf;
        newNode->right = node;
    }

    if (prev == NULL) {
        trie->root->left = newNode;
    }
    else {
        u32 prevBit = _ExtractRefBit(key, prev->refBit);
        if (prevBit)
            prev->right = newNode;
        else
            prev->left = newNode;
    }

    return newLeaf;
}

typedef struct _NodeIndexMap {
    ConsPtrieNode* node;
    u64 index;
} _NodeIndexMap;

static u64 _FindIndex(ConsPtrieNode* node, _NodeIndexMap* map, u64 count) {
    for (u64 i = 0; i < count; i++) {
        if (map[i].node == node)
            return map[i].index;
    }
    return NODE_NPOS;
}

static u64 _FlattenNode(
    ConsPtrieNode* node, ConsPtrieNode* root,
    ConsFlatPtrieNode* outNodes, _NodeIndexMap* map, u64* counter
) {
    if (node == NULL || node == root)
        return NODE_NPOS;

    u64 existingIndex = _FindIndex(node, map, *counter);
    if (existingIndex != NODE_NPOS)
        return existingIndex;

    u64 index = (*counter)++;

    map[index].node = node;

    outNodes[index].key = node->key ? strdup(node->key) : NULL;
    outNodes[index].refBit = node->refBit;

    outNodes[index].leftIndex = 
        (node->left == NULL || node->left == node) ? index :
        _FlattenNode(node->left, root, outNodes, map, counter);

    outNodes[index].rightIndex = 
        (node->right == NULL || node->right == node) ? index :
        _FlattenNode(node->right, root, outNodes, map, counter);

    return index;
}

void PtrieFlatten(ConsPtrie* trie, ConsFlatPtrie* flat) {
    if (trie == NULL || trie->root == NULL)
        return;

    ConsFlatPtrie tempFlat;

    tempFlat.nodeCount = trie->nodeCount - 1;
    tempFlat.nodes = calloc(tempFlat.nodeCount + 1, sizeof(ConsFlatPtrieNode));

    _NodeIndexMap* map = calloc(tempFlat.nodeCount + 1, sizeof(_NodeIndexMap));
    u64 counter = 0;

    u64 firstNodeIndex = _FlattenNode(trie->root->left, trie->root, tempFlat.nodes, map, &counter);

    free(map);

    flat->nodeCount = trie->nodeCount;
    flat->nodes = calloc(flat->nodeCount + 1, sizeof(ConsFlatPtrieNode));

    for (u64 i = 1; i <= trie->nodeCount; i++) {
        flat->nodes[i] = tempFlat.nodes[i - 1];
        flat->nodes[i].leftIndex += 1;
        flat->nodes[i].rightIndex += 1;
    }

    free(tempFlat.nodes);

    flat->nodes[0].key = NULL;
    flat->nodes[0].refBit = REF_BIT_NPOS;
    flat->nodes[0].leftIndex = firstNodeIndex + 1;
    flat->nodes[0].rightIndex = 0;
}

ConsFlatPtrieNode* PtrieSearchFlat(ConsFlatPtrie* trie, const char* key) {
    if (trie == NULL)
        return NULL;

    ConsFlatPtrieNode* node = trie->nodes + trie->nodes->leftIndex;
    ConsFlatPtrieNode* prevNode = node;

    while (1) {
        prevNode = node;

        u32 bit = _ExtractRefBit(key, node->refBit);
        node = trie->nodes + (bit ? node->rightIndex : node->leftIndex);

        if (node == trie->nodes)
            return NULL;
        if (node->refBit <= prevNode->refBit)
            break;
    }

    if (strcmp(key, node->key) == 0)
        return node;
    else
        return NULL;
}

void PtrieSwapFlatNode(ConsFlatPtrie* trie, u64 aIdx, u64 bIdx) {
    if (trie == NULL || aIdx == bIdx || aIdx >= (trie->nodeCount + 1) || bIdx >= (trie->nodeCount + 1))
        return;

    ConsFlatPtrieNode* nodeA = trie->nodes + aIdx;
    ConsFlatPtrieNode* nodeB = trie->nodes + bIdx;

    ConsFlatPtrieNode origA = *nodeA;
    ConsFlatPtrieNode origB = *nodeB;

    if (origA.leftIndex == bIdx) origA.leftIndex = aIdx;
    else if (origA.leftIndex == aIdx) origA.leftIndex = bIdx;

    if (origA.rightIndex == bIdx) origA.rightIndex = aIdx;
    else if (origA.rightIndex == aIdx) origA.rightIndex = bIdx;

    if (origB.leftIndex == aIdx) origB.leftIndex = bIdx;
    else if (origB.leftIndex == bIdx) origB.leftIndex = aIdx;

    if (origB.rightIndex == aIdx) origB.rightIndex = bIdx;
    else if (origB.rightIndex == bIdx) origB.rightIndex = aIdx;

    *nodeA = origB;
    *nodeB = origA;

    for (u64 i = 0; i < trie->nodeCount + 1; i++) {
        if (i == aIdx || i == bIdx)
            continue;

        ConsFlatPtrieNode* n = trie->nodes + i;

        if (n->leftIndex == aIdx)
            n->leftIndex = bIdx;
        else if (n->leftIndex == bIdx)
            n->leftIndex = aIdx;

        if (n->rightIndex == aIdx)
            n->rightIndex = bIdx;
        else if (n->rightIndex == bIdx)
            n->rightIndex = aIdx;
    }
}
