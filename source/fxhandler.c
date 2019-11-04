#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

// Note to self; read OpenCubicPlayer sourcecode.

bool handleSubFX(uint8_t fxt, uint8_t h, uint8_t l, const char** p_arg1, char* _arg1, bool isFT) {
    bool isBufferMem = false;
    switch (h) {
        case 0x1:
            isBufferMem = true;
            *p_arg1 = "EPRtU";
            break;
        case 0x2:
            isBufferMem = true;
            *p_arg1 = "EPRtD";
            break;
        case 0x3:
            // Not supported widely...?
            break;
        case 0x4:
            *p_arg1 = "STVwF";
            break;
        case 0x5:
            *p_arg1 = "STFtV";
            break;
        case 0x6:  //Pattern loop is the special thing...
            if (l == 0)
                *p_arg1 = "PTLpS";
            else
                *p_arg1 = "PTLpG";
            break;

        case 0x8:
            // No Buffer mem
            *p_arg1 = "ESEtP";
            break;
        case 0xa:
            isBufferMem = true;
            *p_arg1 = "EFVsU";
            break;
        case 0xb:
            isBufferMem = true;
            *p_arg1 = "EFVsD";
            break;
        case 0xc:
            *p_arg1 = "ENCuT";
            break;
        case 0xd:
            *p_arg1 = "EDElY";
            break;
        case 0xe:
            *p_arg1 = "EPDlY";
            break;
        default:
            snprintf(_arg1, 6, "??E%02X", fxt);
            *p_arg1 = _arg1;
    }
    return isBufferMem;
}

// Pitch (Y) *p_arg2 = "\e[33m";
// Volum (G) *p_arg2 = "\e[32m";
// Panning (C) *p_arg2 = "\e[36m";
// Global (M) *p_arg2 = "\e[35m";
// Misc (R) *p_arg2 = "\e[31m";

bool handleFX(uint8_t fxt, uint8_t fxp, const char** p_arg1, const char** p_arg2, char* _arg1, bool isFT) {
    bool isBufferMem = false;
    uint8_t h, l;
    h = (fxp >> 4) & 0xF;
    l = fxp & 0xF;
    switch (fxt) {
        case 0:
        case 0xb4:       // MOD/XM 0xy, IT/S3M Jxy
            if (!fxp) {  // need to be real, not fake one
                break;
            }
            *p_arg1 = "ARPeG";  //P
            *p_arg2 = "\e[33m";
            break;
        case 1:  // 1xx Fxx // But when FFx, it FINELY, FEx EXTRA FINE...????
            isBufferMem = true;
            *p_arg1 = "PORtU";
            *p_arg2 = "\e[33m";  // P
            break;
        case 2:  // 2xx Exx // But when EFx, it FINELY, EEx EXTRA FINE...???? I don't get it anymore.
            isBufferMem = true;
            *p_arg1 = "PORtD";
            *p_arg2 = "\e[33m";  //P
            break;
        case 3:  // 3xx Gxx
            isBufferMem = true;
            *p_arg1 = "TPOrT";
            *p_arg2 = "\e[33m";
            break;
        case 4:  // 4xy Hxy
            isBufferMem = true;
            *p_arg1 = "VIBrT";
            *p_arg2 = "\e[33m";
            break;
        case 5:
            isBufferMem = true;
            *p_arg1 = "TONvS";
            *p_arg2 = "\e[31m";
            break;
        case 6:
            isBufferMem = true;
            *p_arg1 = "VIBvS";
            *p_arg2 = "\e[31m";
            break;
        case 7:
            isBufferMem = true;
            *p_arg1 = "TREmO";
            *p_arg2 = "\e[32m";
            break;
        case 8:
            *p_arg1 = "PANsT";
            *p_arg2 = "\e[36m";
            break;
        case 9:
            isBufferMem = true;
            *p_arg1 = "OFStS";
            *p_arg2 = "\e[31m";
            break;
        case 0xa:
        case 0xa4:
            *p_arg2 = "\e[32m";
            if (isFT) {
                isBufferMem = true;
                if (l == 0 && h > 0)  // Up
                    *p_arg1 = "VOLsU";
                else if (h == 0 && l > 0)  // Down
                    *p_arg1 = "VOLsD";
                else {
                    snprintf(_arg1, 6, "A%02X%02X", fxp, fxt);
                    *p_arg1 = _arg1;
                }
                break;
            }
            // S3M/IT
            if (l == 0xF && h != 0)
                *p_arg1 = "VOLsU";
            else if (h == 0xf && l != 0)
                *p_arg1 = "VOLsD";
            else if (h == 0 && l != 0)  // S3M
                *p_arg1 = "VOLsD";
            else if (h != 0 && l == 0)
                *p_arg1 = "VOLsU";
            else if (h == 0xf && l == 0xf)  // ???
                *p_arg1 = "VOLsS";
            break;

        case 0xb:
            *p_arg1 = "POSjP";
            *p_arg2 = "\e[35m";
            break;
        case 0xc:
            *p_arg1 = "VOLsT";
            *p_arg2 = "\e[32m";
            break;
        case 0xd:
            *p_arg1 = "PTBrK";
            *p_arg2 = "\e[35m";
            break;
        case 0xe:;  // in FTII, E denotes "Extended" effect; that means we need break
                    // it down
            // TODO: Optimize the heck out of it.
            isBufferMem = handleSubFX(fxt, h, l, p_arg1, _arg1, isFT);
            break;

        case 0xa3:
        case 0xf:
            *p_arg1 = "SPDsT";
            *p_arg2 = "\e[35m";
            break;

            // END MOD

        case 0x10:  //Gxy
            *p_arg2 = "\e[32m";
            *p_arg1 = "GVOlT";
            break;

        case 0x11:  //Hxy
            isBufferMem = true;
            *p_arg1 = "GVOlS";
            *p_arg2 = "\e[32m";
            break;

        case 0x14:  //Kxx
            *p_arg1 = "KEYoF";
            *p_arg2 = "\e[31m";
            break;

        case 0x15:  //Lxy
            *p_arg1 = "EVPsS";
            *p_arg2 = "\e[32m";
            break;

        case 0x19:  //Pxy
            isBufferMem = true;
            *p_arg1 = "PANsL";
            *p_arg2 = "\e[36m";
            break;

        case 0x1b:  //Rxy
            isBufferMem = true;
            *p_arg1 = "RETrG";
            *p_arg2 = "\e[31m";
            break;

        case 0x1d:  //Txy
            isBufferMem = true;
            *p_arg1 = "TREmR";
            *p_arg2 = "\e[32m";
            break;

            // END XM

        case 0xb5:
            *p_arg1 = "VPNsL";
            break;

        // S3M/IT
        case 0x8d:
            *p_arg1 = "SURrD";
            break;
        case 0x87:
            *p_arg1 = "SETtP";
            break;

        case 0x80:
            *p_arg1 = "TRLvL";
            break;
        case 0xc0:
            *p_arg1 = "VVLsU";
            break;
        case 0xc1:
            *p_arg1 = "VVLsD";
            break;
        case 0xc2:
            *p_arg1 = "VVLfU";
            break;
        case 0xc3:
            *p_arg1 = "VVLfD";
            break;
        default:
            snprintf(_arg1, 6, "???%02X", fxt);
            *p_arg1 = _arg1;
    }
    return isBufferMem;
}
