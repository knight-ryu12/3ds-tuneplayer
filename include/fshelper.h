#pragma once
#include <3ds.h>

typedef struct __attribute__((packed, aligned(4))) {
    uint32_t type;
    uint64_t extdataId;
} Extdata_Path;

Result FSHelp_FormatExtdata(uint64_t id, FS_MediaType media, uint32_t directories, uint32_t files, uint64_t sizeLimit, uint32_t smdhSize, uint8_t *smdh);
Result FSHelp_DeleteExtdata(uint64_t id, FS_MediaType media);
Result FSHelp_EnsuredExtdataMount(FS_Archive* archive, uint64_t id, FS_MediaType media, uint32_t directories, uint32_t files, uint64_t sizeLimit, uint32_t smdhSize, uint8_t* smdh);
void FSHelp_Cleanup();
