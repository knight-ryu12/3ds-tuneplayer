#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "linkedlist.h"
#include "sndthr.h"

#define MEMBLOCK_SIZE 2048

static uint8_t *buffer_ptr = NULL;
static uint32_t buffer_sz;

int initXMP(char *path, xmp_context c, struct xmp_module_info *mi) {
    if (xmp_load_module(c, path) != 0) return 1;
    xmp_get_module_info(c, mi);
    xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
    //xmp_set_player(c, XMP_PLAYER_VOICES, 256);
    return 0;
}

int loadSongMemory(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT) {
    FILE *fp = NULL;
    struct xmp_test_info ti;
    int32_t res;
    printf("Loading...\n");
    //free(buffer_ptr);
    //buffer_ptr = NULL;
    chdir(dir);
    //xmp_stop_module(c);
    xmp_end_player(c);
    xmp_release_module(c);
    // Then
    free(buffer_ptr);
    //buffer_ptr = NULL;
    // This'll release ptr memory from memory

    if (xmp_test_module(path, &ti) != 0) {
        printf("Illegal module detected.\n");
        return 2;
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
        return 1;
    }
    printf("malloced at %8p\n", buffer_ptr);
    // Fancy loading?
    uint32_t blocks = MEMBLOCK_SIZE;
    if (buffer_sz <= 0x100000) blocks = MEMBLOCK_SIZE / 2;

    for (uint32_t i = 0; i < buffer_sz; i += blocks) {
        //COOL LOADING
        res = fread(buffer_ptr + i, 1, blocks, fp);
        if (res <= 0) {
            printf("error on reading\n");
            break;
        }
        printf("Load%8p rem %08lx read%4ld\r", buffer_ptr + i, buffer_sz - i, res);
    }
    printf("\n");
    // Try testing first
    // this is what initXMP do
    res = xmp_load_module_from_memory(c, buffer_ptr, buffer_sz);
    printf("xmp loadstate %ld\n", res);
    xmp_get_module_info(c, mi);
    if (strstr(mi->mod->type, "XM") != NULL) {
        printf("XM Mode\n");
        *isFT = 1;
    } else
        *isFT = 0;

    res = xmp_start_player(c, SAMPLE_RATE, 0);
    printf("xmp state %ld\n", res);
    //xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
    //_debug_pause();
    fclose(fp);

    return 0;
}

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
    //svcSleepThread(150000000);  // wait 1.s
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
