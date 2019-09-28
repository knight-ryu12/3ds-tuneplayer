#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "linkedlist.h"
#include "sndthr.h"

//this undefined makes loading directory slower, but it's an option
#define DISABLE_LOADCHECK

#define MEMBLOCK_SIZE 2048
// on over 2MB song, this value will be used

static uint8_t *buffer_ptr = NULL;
static uint32_t buffer_sz = 0;

/*int initXMP(char *path, xmp_context c, struct xmp_module_info *mi) {
    if (xmp_load_module(c, path) != 0) return 1;
    xmp_get_module_info(c, mi);
    xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_LINEAR);
    xmp_set_player(c, XMP_PLAYER_VOICES, 256);
    return 0;
}*/

int loadSongMemory(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT, bool *released) {
    FILE *fp = NULL;
    struct xmp_test_info ti;
    int32_t res;
    uint32_t rem = 0;
    printf("Loading...\n");
    //free(buffer_ptr);
    //buffer_ptr = NULL;
    chdir(dir);
    //xmp_stop_module(c);
    xmp_end_player(c);
    if (!released || !*released) {
        xmp_release_module(c);
        if (released) *released = true;
    }
    // Then
    free(buffer_ptr);
    //buffer_ptr = NULL;
    // This'll release ptr memory from memory

    if (xmp_test_module(path, &ti) != 0) {
        printf("Illegal module detected.\n");
        return 1;
    }
    // First, read filesize
    printf("%s\n%s\n", ti.name, ti.type);
    fp = fopen(path, "rb");
    fseek(fp, 0, SEEK_END);
    buffer_sz = ftell(fp);
    printf("Filesize %ld\n", buffer_sz);
    fseek(fp, 0, SEEK_SET);
    buffer_ptr = malloc(buffer_sz + 1);
    if (buffer_ptr == NULL) {
        //???????
        printf("Memory Allocation Error!\n");
        fclose(fp);
        return 2;
    }
    printf("malloced at %8p\n", buffer_ptr);
    // Fancy loading?
    uint32_t blocks = MEMBLOCK_SIZE;
    if (buffer_sz >= 0x100000)
        blocks = blocks * 2;
    if (buffer_sz >= 0x400000)
        blocks = blocks * 2;

    rem = buffer_sz;
    for (uint32_t i = 0; i < buffer_sz; i += blocks) {
        //COOL LOADING
        res = fread((buffer_ptr + i), 1, blocks < rem ? blocks : rem, fp);
        if (res <= 0) {
            printf("error on reading\n");
            fclose(fp);
            return 3;
        }
        rem -= res;
        printf("Load%8p rem%08lX read%4ld\r", buffer_ptr + i, rem, res);
    }
    printf("\n");
    // Try testing first
    // this is what initXMP do
    res = xmp_load_module_from_memory(c, buffer_ptr, buffer_sz);
    printf("xmp loadstate %ld\n", res);
    if (res != 0) {
        printf("loadstate != 0!?\n");
        fclose(fp);
        return 4;
    }
    xmp_get_module_info(c, mi);
    if (strstr(mi->mod->type, "XM") != NULL || strstr(mi->mod->type, "MOD") != NULL) {
        printf("XM Mode\n");
        *isFT = 1;
    } else
        *isFT = 0;

    res = xmp_start_player(c, SAMPLE_RATE, 0);
    printf("xmp state %ld\n", res);
    xmp_set_player(c, XMP_PLAYER_MIX, 50);
    xmp_set_player(c, XMP_PLAYER_DEFPAN, 70);
    xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_LINEAR);
    xmp_set_player(c, XMP_PLAYER_VOICES, 256);
    //_debug_pause();
    fclose(fp);
#ifdef DEBUG
    _debug_pause();
#endif
    // we loaded so we'll need releasing next time
    if (released) *released = false;

    return 0;
}

/*int loadSong(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT) {
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
    //svcSleepThread(150000000);  // wait 1.s
    xmp_start_player(c, SAMPLE_RATE, 0);
    return 0;
}*/

uint32_t searchsong(const char *searchPath, LinkedList *list) {
    uint32_t count = 0;
    printf("Scanning %s\n", searchPath);
    DIR *searchDir = opendir(searchPath);
    if (searchDir == NULL) {
        printf("Folder non-existent, skipping.\n");
        return 0;
    }  // Folder non-existent;
    struct dirent *de;
#ifndef DISABLE_LOADCHECK
    char *pathbuf = malloc(PATH_MAX + 1);
    if (!pathbuf)
        printf("Couldn't allocate path buffer.\nModules will only be tested at loading.\n");
#endif
    for (de = readdir(searchDir); de != NULL; de = readdir(searchDir)) {
        if (de->d_name[0] == '.') continue;  // don't include . or .. and dot-files
        if (de->d_type != DT_REG) continue;  // don't include non files, e.g. folders
#ifndef DISABLE_LOADCHECK
        if (pathbuf) {
            snprintf(pathbuf, PATH_MAX + 1, "%s%s%s", searchPath, searchPath[strlen(searchPath) - 1] == '/' ? "" : "/", de->d_name);
            if (xmp_test_module(pathbuf, NULL))
                printf("Skipping %s, failed module test.\n", de->d_name);
        }
#endif
        printf("Adding %s\n", de->d_name);
        add_single_node(list, create_node(de->d_name, searchPath));
        count++;
    }
#ifndef DISABLE_LOADCHECK
    free(pathbuf);
#endif
    return count;
}

// load song from list until one is found
// remove bad songs from list
// exit with non 0 if no song found and clear list
// support initial loading if current_song was not set yet.
int load_song(xmp_context c, struct xmp_module_info *mi, LinkedList *ll, LLNode **current_song, int *isFT, bool *released, bool next) {
    if (*current_song) {
        *current_song = next ? (*current_song)->next : (*current_song)->prev;
        if (!*current_song) *current_song = next ? ll->front : ll->back;
    } else
        *current_song = next ? ll->front : ll->back;
    if (!*current_song) return 1;
    xmp_stop_module(c);
    while (loadSongMemory(c, mi, (*current_song)->track_path, (*current_song)->directory, isFT, released)) {
        printf("Bad song detected\n");
        LLNode *node = *current_song;
        *current_song = next ? (*current_song)->next : (*current_song)->prev;
        remove_single_node(ll, node);
        if (!*current_song) *current_song = next ? ll->front : ll->back;
        if (!*current_song) return 1;  // congratulations! all songs were unable to load!
    }
    return 0;
}
