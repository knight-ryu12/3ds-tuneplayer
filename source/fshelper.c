#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "fshelper.h"

extern int __system_argc;
extern char** __system_argv;

// taken part from ctrulib's "romfs_dev.c"
// needed since i needed to access smdh data
// and i don't think lib provides way for that..
#define _3DSX_MAGIC 0x58534433 // '3DSX'
typedef struct
{
    u32 magic;
    u16 headerSize, relocHdrSize;
    u32 formatVer;
    u32 flags;

    // Sizes of the code, rodata and data segments +
    // size of the BSS section (uninitialized latter half of the data segment)
    u32 codeSegSize, rodataSegSize, dataSegSize, bssSize;
    // offset and size of smdh
    u32 smdhOffset, smdhSize;
    // offset to filesystem
    u32 fsOffset;
} _3DSX_Header;

static u8* smdh_data = NULL;
static u32 smdh_size = 0;
static LightLock lock = {1}; // it's global, so i'm just pre-initializing it 

static char _3dsxpath[PATH_MAX+1] = {0};
static u16 u16_3dsxpath[PATH_MAX+1] = {0};

// also based around romfsMount, but SMDH
static bool GetSMDH() {
    static const FS_Path archPath = {PATH_EMPTY, 1, ""};

    Result ret;
    u8* _smdh_data;
    u64 _smdh_size;
    u32 read;
    Handle fileHandle;

    LightLock_Lock(&lock);
    if (smdh_data && smdh_size) {
        LightLock_Unlock(&lock);
        return true;
    }

    if (!envIsHomebrew()) {
        static const u32 filePath_data[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};
        static const FS_Path filePath = {PATH_BINARY, 0x14, filePath_data};

        ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, archPath, filePath, FS_OPEN_READ, 0);
        if (R_FAILED(ret)) {
            LightLock_Unlock(&lock);
            return false;
        }

        ret = FSFILE_GetSize(fileHandle, &_smdh_size);
        if (R_FAILED(ret) || _smdh_size > 0x100000000LLU || (_smdh_data = (u8*)malloc((size_t)_smdh_size)) == NULL) {
            FSFILE_Close(fileHandle);
            LightLock_Unlock(&lock);
            return false;
        }

        ret = FSFILE_Read(fileHandle, &read, 0, _smdh_data, (u32)_smdh_size);
        FSFILE_Close(fileHandle);
        if (R_FAILED(ret) || read != (u32)_smdh_size) {
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
        strncat(_3dsxpath, filename+8, PATH_MAX);
        filename = _3dsxpath;
    } else {
        LightLock_Unlock(&lock);
        return false;
    }

    ssize_t units = utf8_to_utf16(u16_3dsxpath, (const u8*)filename, PATH_MAX);
    if (units < 0 || units >= PATH_MAX) {
        LightLock_Unlock(&lock);
        return false;
    }
    u16_3dsxpath[units] = 0;

    // should at a point try to also try to find .smdh if .3dsx doesn't have it, for some reason
    // it should however exist, unless we are using older toolsets

    FS_Path filePath = {PATH_UTF16, (units+1)*2, u16_3dsxpath};

    ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SDMC, archPath, filePath, FS_OPEN_READ, 0);
    if (R_FAILED(ret)) {
        LightLock_Unlock(&lock);
        return false;
    }

    _3DSX_Header hdr;
    ret = FSFILE_Read(fileHandle, &read, 0, &hdr, sizeof(hdr));
    if (R_FAILED(ret) || read != sizeof(hdr) || hdr.magic != _3DSX_MAGIC || hdr.headerSize < sizeof(hdr) || !hdr.smdhOffset || !hdr.smdhSize || (_smdh_data = (u8*)malloc(hdr.smdhSize)) == NULL) {
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

Result FSHelp_FormatExtdata(u64 id, FS_MediaType media, u32 directories, u32 files, u64 sizeLimit, u32 smdhSize, u8 *smdh) {
    if ((!smdhSize || !smdh) && !GetSMDH())
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    FS_ExtSaveDataInfo esdi;
    memset(&esdi, 0, sizeof(esdi));
    esdi.saveId = id;
    esdi.mediaType = media;
    return FSUSER_CreateExtSaveData(esdi, directories, files, sizeLimit, smdh_size, smdh_data);
}