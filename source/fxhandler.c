#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

// Note to self; read OpenCubicPlayer sourcecode.

bool handleFX(uint8_t fxt, uint8_t fxp, const char** p_arg1, char* _arg1, bool isFT) {
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
            *p_arg1 = "ARPeG";
            break;
        case 1:  // 1xx Fxx // But when FFx, it FINELY, FEx EXTRA FINE...????
            isBufferMem = true;
            *p_arg1 = "PORtU";
            break;
        case 2:  // 2xx Exx // But when EFx, it FINELY, EEx EXTRA FINE...???? I don't get it anymore.
            isBufferMem = true;
            *p_arg1 = "PORtD";
            break;
        case 3:  // 3xx Gxx
            isBufferMem = true;
            *p_arg1 = "TPOrT";
            break;
        case 4:  // 4xy Hxy
            isBufferMem = true;
            *p_arg1 = "VIBrT";
            break;
        case 5:
            isBufferMem = true;
            *p_arg1 = "TONvS";
            break;
        case 6:
            isBufferMem = true;
            *p_arg1 = "VIBvS";
            break;
        case 7:
            isBufferMem = true;
            *p_arg1 = "TREmO";
            break;
        case 8:
            *p_arg1 = "PANsT";
            break;
        case 9:
            isBufferMem = true;
            *p_arg1 = "OFStS";
            break;
        case 0xa:
        case 0xa4:
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
            break;
        case 0xc:
            *p_arg1 = "VOLsT";
            break;
        case 0xd:
            *p_arg1 = "PTBrK";
            break;
        case 0xe:;  // in FTII, E denotes "Extended" effect; that means we need break
                    // it down
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
            break;

        case 0xa3:
        case 0xf:
            *p_arg1 = "SPDsT";
            break;

        case 0x10:  // Hxy
            isBufferMem = true;
            *p_arg1 = "GVOlS";
            break;

        case 0x11:
            isBufferMem = true;
            *p_arg1 = "GVOlS";
            break;

        case 0x15:
            *p_arg1 = "EVPsS";
            break;

        case 0x19:
            isBufferMem = true;
            *p_arg1 = "PANsL";
            break;
        case 0xb5:
            *p_arg1 = "VPNsL";
            break;

        case 0x1d:
            isBufferMem = true;
            *p_arg1 = "TREmR";
            break;

        case 0x1b:
            isBufferMem = true;
            *p_arg1 = "RETrG";
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
        case 0xc2:
            *p_arg1 = "VVLsU";
            break;
        case 0xc3:
            *p_arg1 = "VVLsD";
            break;
        default:
            snprintf(_arg1, 6, "???%02X", fxt);
            *p_arg1 = _arg1;
    }
    return isBufferMem;
}
