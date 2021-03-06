#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xmp.h>
#include "linkedlist.h"

int loadSong(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT);
uint32_t searchsong(const char *searchPath, LinkedList *list);
//int loadSongMemory(char *path, char *dir);
int loadSongMemory(xmp_context c, struct xmp_module_info *mi, char *path, char *dir, int *isFT, bool *released);
int load_song(xmp_context, struct xmp_module_info *, LinkedList *, LLNode **, int *, bool *, bool);