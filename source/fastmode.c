#include "fastmode.h"
#include <3ds.h>
#include <stdio.h>

int try_speedup() {
    Result res;
    uint8_t model;
    int r = 0;
    res = ptmSysmInit();
    if (R_FAILED(res)) return -1;
    res = cfguInit();
    if (R_FAILED(res)) return -2;
    CFGU_GetSystemModel(&model);
    if (model == 2 || model >= 4) {
        // Ignore model that doesn't have N3DS CPU
        PTMSYSM_ConfigureNew3DSCPU(3);
        printf("Using N3DS Mode\n");
        r = 1;
    }
    ptmSysmExit();
    cfguExit();
    return r;
}
