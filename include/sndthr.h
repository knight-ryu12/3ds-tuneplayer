#pragma once
#include <3ds.h>
#include <xmp.h>

#define CHANNEL 0x10
#define SAMPLE_RATE 32728

Result setup_ndsp();
int initXMP(const char *path, xmp_context c, struct xmp_module_info *mi);
void soundThread(void *arg);
void readThread(void *arg);

struct thread_data {
    xmp_context c;
    int model;
};
