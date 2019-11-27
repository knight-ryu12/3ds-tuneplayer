#include "config.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fshelper.h"

#define DEBUG
#define CONFIG_VERSION 3
#define FSA_PATH 0x4152435A00000001LLU

static const char* const _config = "/config.bin";  //length 11 + 1 null
static const FS_Path configpath = {PATH_ASCII, 12, _config};
static const Extdata_Path _ext = {.type = MEDIATYPE_SD, .extdataId = FSA_PATH};
static const FS_Path _extpath = {PATH_BINARY, sizeof(_ext), &_ext};

// need to make function to set these
static uint8_t* _smdh_data = NULL;
static uint32_t _smdh_size = 0;

static Result PlayerConfig_GetConfig(PlayerConfig* config, Handle file) {
    uint32_t readsz;
    Result r = FSFILE_Read(file, &readsz, 0, config, sizeof(PlayerConfig));
#ifdef DEBUG
    printf("FSFILE_FR %08lx, %ld\n", r, readsz);
    printf("Version %d\n", config->version);
    printf("Debug %d\n", config->debugmode);
#endif
    if (R_SUCCEEDED(r) && config->version < CONFIG_VERSION) {
        //write new config, while preserving config contents;
        printf("Outdated/New creation, updating...");
        PlayerConfig default_pc = {CONFIG_VERSION, 0, -1, 0};
        default_pc.loopcheck = config->loopcheck;
        uint32_t writesz = 0;
        r = FSFILE_Write(file, &writesz, 0, &default_pc, sizeof(default_pc), 0);
        *config = default_pc;
        if (R_SUCCEEDED(r))
            printf("Done.\n");
        else
            printf("Fail.\n");

#ifdef DEBUG
        printf("FSFILE_FW %08lx, %ld\n", r, writesz);
#endif
    }
    FSFILE_Close(file);
    return r;
}

Result PlayerConfig_Load(PlayerConfig* config) {
    Handle file;
    Result r = FSUSER_OpenFileDirectly(&file, ARCHIVE_EXTDATA, _extpath, configpath, FS_OPEN_READ | FS_OPEN_WRITE, 0);
#ifdef DEBUG
    printf("FSUSER_OF %08lx\n", r);
#endif
    if (R_FAILED(r))
        return r;
    return PlayerConfig_GetConfig(config, file);
}

Result PlayerConfig_Save(PlayerConfig* config) {
    Handle file;
    Result r = FSUSER_OpenFileDirectly(&file, ARCHIVE_EXTDATA, _extpath, configpath, FS_OPEN_READ | FS_OPEN_WRITE, 0);

    uint32_t writesz = 0;
    r = FSFILE_Write(file, &writesz, 0, config, sizeof(PlayerConfig), 0);
#ifdef DEBUG
    printf("FSFILE_FW %08lx, %ld\n", r, writesz);
#endif
    return r;
}

Result PlayerConfig_EnsuredLoad(PlayerConfig* config) {
    FS_Archive extarc;
    Result r = FSHelp_EnsuredExtdataMount(&extarc, FSA_PATH, MEDIATYPE_SD, 1, 2, 131072, _smdh_size, _smdh_data);
#ifdef DEBUG
    printf("FSUSER_OA %08lx\n", r);
#endif
    if (R_FAILED(r))
        return r;

    Handle file;
    r = FSUSER_OpenFile(&file, extarc, configpath, FS_OPEN_READ | FS_OPEN_WRITE, 0);
#ifdef DEBUG
    printf("FSUSER_OF %08lx\n", r);
#endif

    if (R_FAILED(r)) {
        if (R_SUMMARY(r) != RS_NOTFOUND) {
            // some reason other than file not found? try delete first
            r = FSUSER_DeleteFile(extarc, configpath);
#ifdef DEBUG
            printf("FSUSER_DF %08lx\n", r);
#endif
            if (R_FAILED(r)) {
                FSUSER_CloseArchive(extarc);
                return r;
            }
        }
        r = FSUSER_CreateFile(extarc, configpath, 0, 128);
#ifdef DEBUG
        printf("FSUSER_CF %08lx\n", r);
#endif
        if (R_FAILED(r)) {
            FSUSER_CloseArchive(extarc);
            return r;
        }

        r = FSUSER_OpenFile(&file, extarc, configpath, FS_OPEN_READ | FS_OPEN_WRITE, 0);
#ifdef DEBUG
        printf("FSUSER_OF %08lx\n", r);
#endif
        if (R_FAILED(r)) {
            FSUSER_CloseArchive(extarc);
            return r;
        }
    }
    FSUSER_CloseArchive(extarc);
    return PlayerConfig_GetConfig(config, file);
}
