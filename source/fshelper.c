#include "fshelper.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int __system_argc;
extern char** __system_argv;

// taken part from ctrulib's "romfs_dev.c"
// needed since i needed to access smdh data
// and i don't think lib provides way for that..
#define _3DSX_MAGIC 0x58534433  // '3DSX'
#define _SMDH_MAGIC 0x48444d53  // 'SMDH'
typedef struct
{
    uint32_t magic;
    uint16_t headerSize, relocHdrSize;
    uint32_t formatVer;
    uint32_t flags;

    // Sizes of the code, rodata and data segments +
    // size of the BSS section (uninitialized latter half of the data segment)
    uint32_t codeSegSize, rodataSegSize, dataSegSize, bssSize;
    // offset and size of smdh
    uint32_t smdhOffset, smdhSize;
    // offset to filesystem
    uint32_t fsOffset;
} _3DSX_Header;

typedef struct {
    // Each one is UTF-16 encoded.
    uint8_t shortDesc[0x80];
    uint8_t longDesc[0x100];
    uint8_t Publisher[0x80];
} _APPT_Header;

typedef struct
{
    // Applicaiton settings
    uint8_t ageRating[0x10];
    uint32_t regLockout;
    uint8_t mmID[0xC];  //M(atch)M(aker)ID
    uint32_t flag;
    uint16_t vEula;
    uint16_t reserved;
    uint32_t OADF;   // Optimal Animation Default Frame ???
    uint32_t CECID;  // Street pass ID
} _APPS_Header;

typedef struct {
    uint8_t small[0x480];  // these encoding is word-order crap.
    uint8_t large[0x1200];
} _ICON_Header;

typedef struct {
    uint32_t magic;
    uint16_t version, reserved1;
    _APPT_Header titles[16];  // 0x200 bytes long
    _APPS_Header applicationSetting;
    uint64_t reserved2;
    _ICON_Header iconGraphic;
} _SMDH_Header;

static uint8_t* smdh_data = NULL;
static uint32_t smdh_size = 0;
static LightLock lock = {1};  // it's global, so i'm just pre-initializing it

static char _3dsxpath[PATH_MAX + 1] = {0};
static uint16_t u16_3dsxpath[PATH_MAX + 1] = {0};

// also based around romfsMount, but SMDH
static bool GetSMDH() {
    static const FS_Path archPath = {PATH_EMPTY, 1, ""};

    Result ret;
    uint8_t* _smdh_data;
    uint64_t _smdh_size;
    uint32_t read;
    Handle fileHandle;

    LightLock_Lock(&lock);
    if (smdh_data && smdh_size) {
        LightLock_Unlock(&lock);
        return true;
    }

    if (!envIsHomebrew()) {
        static const uint32_t filePath_data[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};
        static const FS_Path filePath = {PATH_BINARY, 0x14, filePath_data};

        ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, archPath, filePath, FS_OPEN_READ, 0);
        if (R_FAILED(ret)) {
            LightLock_Unlock(&lock);
            return false;
        }

        ret = FSFILE_GetSize(fileHandle, &_smdh_size);
        if (R_FAILED(ret) || _smdh_size > 0x100000000LLU || (_smdh_data = (uint8_t*)malloc((size_t)_smdh_size)) == NULL) {
            FSFILE_Close(fileHandle);
            LightLock_Unlock(&lock);
            return false;
        }

        ret = FSFILE_Read(fileHandle, &read, 0, _smdh_data, (uint32_t)_smdh_size);
        FSFILE_Close(fileHandle);
        if (R_FAILED(ret) || read != (uint32_t)_smdh_size) {
            free(_smdh_data);
            LightLock_Unlock(&lock);
            return false;
        }
        smdh_data = _smdh_data;
        smdh_size = _smdh_size;
        LightLock_Unlock(&lock);
        return true;
    }

    const char* filename = NULL;
    if (__system_argc > 0 && __system_argv[0])
        filename = __system_argv[0];
    if (!filename) {
        LightLock_Unlock(&lock);
        return false;
    }

    if (!strncmp(filename, "sdmc:/", 6))
        filename += 5;
    else if (!strncmp(filename, "3dslink:/", 9)) {
        strcpy(_3dsxpath, "/3ds");
        strncat(_3dsxpath, filename + 8, PATH_MAX);
        filename = _3dsxpath;
    } else {
        LightLock_Unlock(&lock);
        return false;
    }

    ssize_t units = utf8_to_utf16(u16_3dsxpath, (const uint8_t*)filename, PATH_MAX);
    if (units < 0 || units >= PATH_MAX) {
        LightLock_Unlock(&lock);
        return false;
    }
    u16_3dsxpath[units] = 0;

    // should at a point try to also try to find .smdh if .3dsx doesn't have it, for some reason
    // it should however exist, unless we are using older toolsets

    FS_Path filePath = {PATH_UTF16, (units + 1) * 2, u16_3dsxpath};

    ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SDMC, archPath, filePath, FS_OPEN_READ, 0);
    if (R_FAILED(ret)) {
        LightLock_Unlock(&lock);
        return false;
    }

    _3DSX_Header hdr;
    ret = FSFILE_Read(fileHandle, &read, 0, &hdr, sizeof(hdr));
    if (R_FAILED(ret) || read != sizeof(hdr) || hdr.magic != _3DSX_MAGIC || hdr.headerSize < sizeof(hdr) || !hdr.smdhOffset || !hdr.smdhSize || (_smdh_data = (uint8_t*)malloc(hdr.smdhSize)) == NULL) {
        FSFILE_Close(fileHandle);
        LightLock_Unlock(&lock);
        return false;
    }

    ret = FSFILE_Read(fileHandle, &read, hdr.smdhOffset, _smdh_data, hdr.smdhSize);
    FSFILE_Close(fileHandle);
    if (R_FAILED(ret) || read != hdr.smdhSize) {
        free(_smdh_data);
        LightLock_Unlock(&lock);
        return false;
    }
    smdh_data = _smdh_data;
    smdh_size = hdr.smdhSize;
    LightLock_Unlock(&lock);
    return true;
}

Result FSHelp_FormatExtdata(uint64_t id, FS_MediaType media, uint32_t directories, uint32_t files, uint64_t sizeLimit, uint32_t smdhSize, uint8_t* smdh) {
    FS_ExtSaveDataInfo esdi;
    memset(&esdi, 0, sizeof(esdi));
    esdi.saveId = id;
    esdi.mediaType = media;

    if ((!smdhSize || !smdh) && !GetSMDH()) {
        // special case just for citra with a 3dsx
        Result res = FSUSER_CreateExtSaveData(esdi, directories, files, sizeLimit, 0, NULL);
        if (R_SUCCEEDED(res))
            return res;
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }
    if (!smdhSize || !smdh) {
        smdh = smdh_data;
        smdhSize = smdh_size;
    }
    return FSUSER_CreateExtSaveData(esdi, directories, files, sizeLimit, smdh_size, smdh_data);
}

Result FSHelp_DeleteExtdata(uint64_t id, FS_MediaType media) {
    FS_ExtSaveDataInfo esdi;
    memset(&esdi, 0, sizeof(esdi));
    esdi.saveId = id;
    esdi.mediaType = media;
    return FSUSER_DeleteExtSaveData(esdi);
}

// this function is a bit relentless to data, it will try to erase and create extdata again if need be
Result FSHelp_EnsuredExtdataMount(FS_Archive* archive, uint64_t id, FS_MediaType media, uint32_t directories, uint32_t files, uint64_t sizeLimit, uint32_t smdhSize, uint8_t* smdh) {
    Extdata_Path extdata = {.type = media, .extdataId = id};
    FS_Path path = {
        PATH_BINARY,
        sizeof(extdata),
        &extdata
    };
    Result res = FSUSER_OpenArchive(archive, ARCHIVE_EXTDATA, path);
    if (R_SUCCEEDED(res))
        return res;
    if (R_SUMMARY(res) != RS_NOTFOUND) {
        res = FSHelp_DeleteExtdata(id, media);
        if (R_FAILED(res))
            return res;
    }
    res = FSHelp_FormatExtdata(id, media, directories, files, sizeLimit, smdhSize, smdh);
    if (R_FAILED(res))
        return res;
    return FSUSER_OpenArchive(archive, ARCHIVE_EXTDATA, path);
}

void FSHelp_Cleanup() {
    free(smdh_data);
    smdh_data = NULL;
    smdh_size = 0;
}
