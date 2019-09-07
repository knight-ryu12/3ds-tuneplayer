#pragma once
#include <3ds.h>
#include <stdio.h>
#include <xmp.h>

#define CHANNEL 0x10
#define SAMPLE_RATE 48000

#define N3DS_BLOCK 4096
#define O3DS_BLOCK 8192
// can someone find sweet spot?

#define MS_TO_PCM16_SIZE(s, c, ms) ((uint32_t)((s) * 2 * (c) * ((ms) / 1000.0f)))

Result setup_ndsp();
void soundThread(void *arg);

static inline void _debug_pause() {
    printf("Press start.\n");
    while (1) {
        hidScanInput();
        uint32_t kDown = hidKeysDown();
        if (kDown & KEY_START) break;
    }
}