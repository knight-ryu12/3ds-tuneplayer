#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "linkedlist.h"
#include "sndthr.h"

int loadSong(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT) {
    printf("Loading....\n");
    struct xmp_test_info ti;
    printf("%s\n", path);
    chdir(dir);
    xmp_end_player(c);
    xmp_release_module(c);
    if (xmp_test_module(path, &ti) != 0) {
        printf("Illegal module detected.\n");
        return 2;
    }
    printf("%s\n%s\n", ti.name, ti.type);
    if (initXMP(path, c, mi) != 0) {
        printf("Error on initXMP!!\n");
        return 1;
    }
    if (strstr(mi->mod->type, "XM") != NULL) {
        printf("XM Mode\n");
        *isFT = 1;
    } else
        *isFT = 0;
    svcSleepThread(150000000);  // wait 1.s
    xmp_start_player(c, SAMPLE_RATE, 0);
    return 0;
}

uint32_t searchsong(const char *searchPath, LinkedList *list) {
    uint32_t count = 0;
    printf("Scanning %s\n", searchPath);
    DIR *searchDir = opendir(searchPath);
    if (searchDir == NULL) {
        printf("Folder non-existent, skipping.\n");
        return 0;
    }  // Folder non-existent;
    struct dirent *de;
    for (de = readdir(searchDir); de != NULL; de = readdir(searchDir)) {
        if (de->d_name[0] == '.') continue;  // don't include . or .. and dot-files
        printf("Adding %s\n", de->d_name);
        add_single_node(list, create_node(de->d_name, searchPath));
        count++;
    }
    return count;
}
