#include "linklist.h"

#include <stdlib.h>

ConsLLNode* LinkListNew(u64 length, u64 initData) {
    if (length == 0)
        return NULL;

    ConsLLNode* head = LinkListNewNode(initData);

    ConsLLNode* currentNode = head;
    for (u64 i = 1; i < length; i++) {
        currentNode->next = LinkListNewNode(initData);
        currentNode->next->prev = currentNode;
        currentNode = currentNode->next;
    }

    return head;
}

ConsLLNode* LinkListNewNode(u64 data) {
    ConsLLNode* node = malloc(sizeof(ConsLLNode));
    if (node == NULL)
        return NULL;

    node->data = data;
    node->prev = node->next = NULL;

    return node;
}

ConsLLNode* LinkListInsertNode(ConsLLNode** head, ConsLLNode* node, s64 index) {
    if (head == NULL || node == NULL)
        return NULL;

    if (index <= 0) {
        node->next = *head;
        node->prev = NULL;
        if (*head)
            (*head)->prev = node;
        *head = node;
        return node;
    }

    if (*head == NULL)
        return NULL;

    ConsLLNode* currentNode = *head;
    s64 i = 0;

    while (i < index - 1 && currentNode->next) {
        currentNode = currentNode->next;
        i++;
    }

    // We're out of bounds.
    if (i < index - 1)
        return NULL;

    node->next = currentNode->next;
    node->prev = currentNode;

    if (currentNode->next)
        currentNode->next->prev = node;

    currentNode->next = node;

    return *head;
}

ConsLLNode* LinkListInsertHead(ConsLLNode** head, u64 data) {
    ConsLLNode* newNode = LinkListNewNode(data);
    if (newNode == NULL)
        return NULL;

    newNode->next = *head;

    if (*head != NULL)
        (*head)->prev = newNode;
    *head = newNode;

    return newNode;
}

ConsLLNode* LinkListInsertTail(ConsLLNode** head, u64 data) {
    ConsLLNode* newNode = LinkListNewNode(data);
    if (newNode == NULL)
        return NULL;

    if (*head == NULL) {
        *head = newNode;
        return newNode;
    }

    ConsLLNode* currentNode = *head;
    while (currentNode->next != NULL)
        currentNode = currentNode->next;

    currentNode->next = newNode;
    newNode->prev = currentNode;

    return newNode;
}

void LinkListDeleteNode(ConsLLNode** head, ConsLLNode* node) {
    if (head == NULL || *head == NULL || node == NULL)
        return;

    if (node->prev != NULL)
        node->prev->next = node->next;
    else
        *head = node->next; // Remove head.

    if (node->next != NULL)
        node->next->prev = node->prev;

    free(node);
}

void LinkListDelete(ConsLLNode** head) {
    if (head == NULL || *head == NULL)
        return;

    ConsLLNode* currentNode = *head;
    while (currentNode != NULL) {
        ConsLLNode* nextNode = currentNode->next;
        free(currentNode);

        currentNode = nextNode;
    }

    *head = NULL;
}

ConsLLNode* LinkListIndex(ConsLLNode* node, s64 index) {
    if (node == NULL)
        return NULL;

    if (index < 0) {
        while (node->next != NULL)
            node = node->next;
        for (s64 i = -1; i > index && node != NULL; i--)
            node = node->prev;
    }
    else {
        for (s64 i = 0; i < index && node != NULL; i++)
            node = node->next;
    }
    return node;
}

s64 LinkListCountNodes(ConsLLNode* head) {
    s64 nodeCount = 0;
    ConsLLNode* currentNode = head;

    while (currentNode != NULL) {
        nodeCount++;
        currentNode = currentNode->next;
    }

    return nodeCount;
}

ConsLLNode* LinkListFind(ConsLLNode* head, u64 data) {
    if (head == NULL)
        return NULL;

    ConsLLNode* currentNode = head;
    while (currentNode != NULL) {
        if (currentNode->data == data)
            return currentNode;

        currentNode = currentNode->next;

        if (currentNode == head)
            return NULL;
    }

    return NULL;
}

void LinkListRemoveByValue(ConsLLNode** head, u64 data) {
    if (head == NULL || *head == NULL)
        return;

    ConsLLNode* node = LinkListFind(*head, data);
    if (node != NULL)
        LinkListDeleteNode(head, node);
}
