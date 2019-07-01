// Linkedlist.h made by LiquidFenrir; Thanks.
#include <stdlib.h>
#define NODE_QUIRKS_AMOUNT 2

typedef struct _LLNode {
    char* track_path;
    char* directory;
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
LLNode* create_node(const char* path, const char* dir);
// Assumes both next and prev of the node are NULL
void add_single_node(LinkedList* list, LLNode* node);

/*
main store a list created with create_list(),
adds all the paths to it through whatever mean
(iterating over the folders, user input, etc...), and setting the quirks
then stores a pointer to the first node using list.front
you can move left or right in the song list by doing
 
for left:
node = node->prev;
if(node == NULL)
    node = list.back;
 
for right:
node = node->next;
if(node == NULL)
    node = list.front;
 
either, followed by
loadSong(c, &mi, node->track_path, &isFT)
 
when done, just call free_list(&list) before returning from main
*/