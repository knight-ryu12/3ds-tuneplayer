#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

// Note to self; read OpenCubicPlayer sourcecode.

// Pitch (Y) *p_arg2 = "\e[33m";
// Volum (G) *p_arg2 = "\e[32m";
// Panning (C) *p_arg2 = "\e[36m";
// Global (M) *p_arg2 = "\e[35m";
// Misc (R) *p_arg2 = "\e[31m";

bool handleSubFX(uint8_t fxt, uint8_t h, uint8_t l, const char** p_arg1, const char** p_arg2, char* _arg1, bool isFT) {
    bool isBufferMem = false;
    switch (h) {
        case 0x1:  // Fine Porta Up E1X
            isBufferMem = true;
            *p_arg1 = "EPRtU";
            *p_arg2 = "\e[33m";
            break;
        case 0x2:  // File Porta Down E2X
            isBufferMem = true;
            *p_arg1 = "EPRtD";
            *p_arg2 = "\e[33m";
            break;
        case 0x3:  // E3x Glissando Control
            *p_arg1 = "EGLiC";
            *p_arg2 = "\e[33m";
            // Not supported widely...?
            break;
        case 0x4:  // E4x Set Vibrato Waveform
            *p_arg1 = "STVwF";
            *p_arg2 = "\e[33m";
            break;
        case 0x5:  // E5x Set Finetune
            *p_arg1 = "STFtV";
            *p_arg2 = "\e[33m";
            break;
        case 0x6:  //Pattern loop is the special thing...
            *p_arg2 = "\e[35m";
            if (l == 0)
                *p_arg1 = "PTLpS";  // E60 Pattern Loop Start
            else
                *p_arg1 = "PTLpG";  // E6X Pattern Loop
            break;
        case 0x7:  // E7X Set Tremolo WaveForm
            *p_arg1 = "ESTwF";
            *p_arg2 = "\e[32m";
            break;
        case 0x8:  // E8X Set panning (why)
            // No Buffer mem
            *p_arg1 = "ESEtP";
            *p_arg2 = "\e[36m";
            break;
        case 0x9:  // E9X Retrigger
            *p_arg1 = "ERTrG";
            *p_arg2 = "\e[31m";
            break;
        case 0xa:  // EAX Fine Volume Slide Up
            isBufferMem = true;
            *p_arg1 = "EFVsU";
            *p_arg2 = "\e[32m";
            break;
        case 0xb:  // EBX Fine Volume Slide Down
            isBufferMem = true;
            *p_arg1 = "EFVsD";
            *p_arg2 = "\e[32m";
            break;
        case 0xc:  // ECX Note Cut
            *p_arg1 = "ENCuT";
            *p_arg2 = "\e[35m";
            break;
        case 0xd:  // EDX Note Delay
            *p_arg1 = "EDElY";
            *p_arg2 = "\e[35m";
            break;
        case 0xe:  // EEX Pattern Delay
            *p_arg1 = "EPDlY";
            *p_arg2 = "\e[35m";
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
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            if (isFT) {
                //isBufferMem = true;
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
            else {
                snprintf(_arg1, 6, "A%02X%02X", fxt, fxp);
                *p_arg1 = _arg1;  // ?????????
            }

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
            isBufferMem = handleSubFX(fxt, h, l, p_arg1, p_arg2, _arg1, isFT);
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
            *p_arg1 = "VPNsL";  // This is Pan ver NOMEM
            break;

        // S3M/IT
        case 0x8d:  // XA4?
            *p_arg2 = "\e[36m";
            *p_arg1 = "SURrD";  // Set Aurround
            break;
        case 0x87:
            *p_arg2 = "\e[35m";
            *p_arg1 = "SETtP";
            break;

        case 0x80:
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            *p_arg1 = "TRLvL";
            break;
        case 0xc0:
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            *p_arg1 = "VVLsU";
            break;
        case 0xc1:
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            *p_arg1 = "VVLsD";
            break;
        case 0xc2:
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            *p_arg1 = "VVLfU";
            break;
        case 0xc3:
            isBufferMem = true;
            *p_arg2 = "\e[32m";
            *p_arg1 = "VVLfD";
            break;
        default:
            snprintf(_arg1, 6, "???%02X", fxt);
            *p_arg1 = _arg1;
    }
    return isBufferMem;
}
