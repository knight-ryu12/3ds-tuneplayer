#pragma once
#include <3ds.h>
#include <xmp.h>

#define CHANNEL 0x10
#define SAMPLE_RATE 32728

Result setup_ndsp();
void soundThread(void *arg);

struct thread_data {
    xmp_context c;
    int model;
};
