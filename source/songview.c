#include <3ds.h>
#include <stdio.h>
#include "linkedlist.h"
#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))
static uint32_t song_number_calc = 0;

void show_playlist(LinkedList ll, LLNode *current,
                   PrintConsole *top, PrintConsole *bot, int *f, int *subscroll) {
    consoleSelect(top);

    gotoxy(0, 0);
    LLNode *cur = ll.front;
    //printf("Cur -> %s%s\n", current->directory, current->track_path);
    int i = 0;
    int toscroll = *f;
    int cinf = song_number_calc;
    int chmax = 0;
    bool isuind = false;
    bool isdind = false;
    char ind = ' ';
    if (song_number_calc == 0) {
        while (cur != NULL) {
            //printf("%2d: %s/%s\n", i, cur->directory, cur->track_path);
            cur = cur->next;
            i++;
        }
        song_number_calc = i;  // calculated.
        cinf = i;
    }
    if (cinf <= 29) {
        toscroll = 0;
        chmax = cinf;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < cinf - 29) {
        if (toscroll != 0) isuind = true;
        isdind = true;
        chmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= cinf - 29) {
        isuind = true;
        toscroll = cinf - 29;
        chmax = cinf;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }
    cur = ll.front;  // Reset
    for (i = 0; i < toscroll; i++) {
        cur = cur->next;
        if (cur == NULL) break;
    }
    for (i = toscroll; i < chmax; i++) {
        ind = ' ';
        if (i == toscroll && isuind) ind = '^';  // There is a lot more to write
        if (i == chmax - 1 && isdind) ind = 'v';
        printf("%2d: %s/%s %c\n", i, cur->directory, cur->track_path, ind);
        cur = cur->next;
        if (cur == NULL) break;
    }
}