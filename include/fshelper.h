#pragma once
#include <3ds.h>

typedef struct __attribute__((packed, aligned(4))) {
    u32 type;
    u64 extdataId;
} Extdata_Path;

Result FSHelp_FormatExtdata(u64 id, FS_MediaType media, u32 directories, u32 files, u64 sizeLimit, u32 smdhSize, u8 *smdh);
