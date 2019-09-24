#pragma once
#include <3ds.h>
#include <stdint.h>
typedef struct {
    uint8_t version;
    uint8_t debugmode;  // 1= Enable Debug
    int loopcheck;
    int loadMode;  // LoadMode; using new method or xmp mode
} PlayerConfig;

Result PlayerConfig_Load(PlayerConfig* config);
Result PlayerConfig_Save(PlayerConfig* config);
Result PlayerConfig_EnsuredLoad(PlayerConfig* config);
