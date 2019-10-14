// Linkedlist.c made by LiquidFenrir; Thanks.
#include "linkedlist.h"
#include <stdlib.h>
#include <string.h>

LinkedList create_list() {
    return (LinkedList){NULL, NULL, 0};
}

void free_list(LinkedList* list) {
    if (list) {
        LLNode* next = NULL;
        LLNode* prev = NULL;
        for (LLNode *front = list->front, *back = list->back; front != NULL && back != NULL; front = next, back = prev) {
            // it was strdup'd, needs to be freed
            if (front) {
                free(front->track_path);
                free(front->directory);
                next = front->next;
                if (next)
                    next->prev = NULL;  // In case back ends up needing to free it
                free(front);
                front = NULL;
            }

            if (back) {
                free(back->track_path);
                free(back->directory);
                prev = back->prev;
                if (prev)
                    prev->next = NULL;  // In case front ends up needing to free it
                free(back);
                back = NULL;
            }
        }

        list->front = NULL;
        list->back = NULL;
        list->size = 0;
    }
}

LLNode* create_node(const char* path, const char* dir) {
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    node->track_path = strdup(path);
    node->directory = strdup(dir);

    for (unsigned i = 0; i < NODE_QUIRKS_AMOUNT; ++i)
        node->track_quirk[i] = 0;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void add_single_node(LinkedList* list, LLNode* node) {
    if (list->size == 0) {
        list->front = node;
        list->back = node;
        list->size = 1;
    } else {
        node->prev = list->back;
        list->back->next = node;
        list->back = node;
        list->size++;
    }
}

void remove_single_node(LinkedList* list, LLNode* node) {
    //just some simple checks
    //quit if list is null or size is 0
    //or if node is null
    //if node previous is null, is expected to be list front. if not, quit
    //if node next is null, is expected to be list back. if not, quit
    if (!list || list->size == 0 || !node || (node->prev == NULL && list->front != node) || (node->next == NULL && list->back != node))
        return;

    if (list->front == node)
        list->front = node->next;
    if (list->back == node)
        list->back = node->prev;

    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;

    free(node->track_path);
    free(node->directory);
    free(node);

    list->size--;

    if (list->size == 0) {
        list->front = NULL;
        list->back = NULL;
    }
}
