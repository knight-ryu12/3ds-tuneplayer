#include <3ds.h>
#include <stdio.h>
#include "linkedlist.h"

void show_playlist(LinkedList ll, LLNode *current,
                   PrintConsole *top, PrintConsole *bot, int f) {
    consoleSelect(top);
    LLNode *cur = ll.front;
    printf("Cur -> %s%s\n", current->directory, current->track_path);
    int i = 0;
    while (cur != NULL) {
        printf("%2d: %s/%s\n", i, cur->directory, cur->track_path);
        cur = cur->next;
        i++;
    }
}