// Linkedlist.h made by LiquidFenrir; Thanks.
#include <stdlib.h>
#define NODE_QUIRKS_AMOUNT 2

typedef struct _LLNode {
    char* track_path;
    int track_quirk[NODE_QUIRKS_AMOUNT];
    int reserved;

    // don't set these yourself, let the functions do it
    struct _LLNode* prev;
    struct _LLNode* next;
} LLNode;

typedef struct _LinkedList {
    LLNode* front;
    LLNode* back;
    size_t size;
} LinkedList;

LinkedList create_list();
void free_list(LinkedList* list);
LLNode* create_node();
// Assumes both next and prev of the node are NULL
void add_single_node(LinkedList* list, LLNode* node);
