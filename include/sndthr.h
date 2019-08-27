#pragma once
#include <3ds.h>
#include <stdio.h>
#include <xmp.h>

#define CHANNEL 0x10
#define SAMPLE_RATE 32728

Result setup_ndsp();
void soundThread(void *arg);

struct thread_data {
    xmp_context c;
    int model;
};

static inline void _debug_pause() {
    printf("Press start.\n");
    while (1) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;
    }
}