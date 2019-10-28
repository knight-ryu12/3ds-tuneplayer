#pragma once
#include <stdint.h>

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
    uint16_t vEula;  //Version EULA
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