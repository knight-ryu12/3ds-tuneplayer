#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>

bool handleFX(uint8_t fxt, uint8_t fxp, char *_arg1, bool isFT) {
    bool isBufferMem = false;
    uint8_t h, l;
    switch (fxt) {
        case 0:
        case 0xb4:
            if (!fxp) {  // need to be real, not fake one
                break;
            }
            strcpy(_arg1, "ARPeG");
            break;
        case 1:
            isBufferMem = true;
            strcpy(_arg1, "PORtU");
            break;
        case 2:
            isBufferMem = true;
            strcpy(_arg1, "PORtD");
            break;
        case 3:
            isBufferMem = true;
            strcpy(_arg1, "TPOrT");
            break;
        case 4:
            isBufferMem = true;
            strcpy(_arg1, "VIBrT");
            break;
        case 5:
            isBufferMem = true;
            strcpy(_arg1, "TONvS");
            break;
        case 6:
            isBufferMem = true;
            strcpy(_arg1, "VIBvS");
            break;
        case 7:
            isBufferMem = true;
            strcpy(_arg1, "TREmO");
            break;
        case 8:
            strcpy(_arg1, "PANsT");
            break;
        case 9:
            isBufferMem = true;
            strcpy(_arg1, "OFStS");
            break;
        case 0xa:
        case 0xa4:
            if (isFT) goto ft2;
            // S3M/IT
            h = (fxp >> 4) & 0xF;
            l = fxp & 0xF;
            if (l == 0xF && h != 0)
                strcpy(_arg1, "VOLsU");
            else if (h == 0xf && l != 0)
                strcpy(_arg1, "VOLsD");
            else if (h == 0 && l != 0)  // S3M
                strcpy(_arg1, "VOLsD");
            else if (h != 0 && l == 0)
                strcpy(_arg1, "VOLsU");
            else if (h == 0xf && l == 0xf)  // ???
                strcpy(_arg1, "VOLsS");
            break;
        ft2:
            isBufferMem = true;
            if ((fxp & 0x0F) == 0 && (fxp >> 4 & 0xF) > 0)  // Up
                strcpy(_arg1, "VOLsU");
            else if (((fxp >> 4) & 0xF) == 0 && (fxp & 0x0F) > 0)  // Down
                strcpy(_arg1, "VOLsD");
            break;

        case 0xb:
            strcpy(_arg1, "POSjP");
            break;
        case 0xc:
            strcpy(_arg1, "VOLsT");
            break;
        case 0xd:
            strcpy(_arg1, "PTBrK");
            break;
        case 0xe:;  // in FTII, E denotes "Extended" effect; that means we need break
                    // it down
            h = (fxp >> 4) & 0xF;
            l = fxp & 0xF;
            fxp = l;
            switch (h) {
                default:
                    break;
                case 0x1:
                    isBufferMem = true;
                    strcpy(_arg1, "EPRtU");
                    break;
                case 0x2:
                    isBufferMem = true;
                    strcpy(_arg1, "EPRtD");
                    break;
                case 0x3:
                    // Not supported widely...?
                    break;
                case 0x4:
                    strcpy(_arg1, "STVwF");
                    break;
                case 0x5:
                    strcpy(_arg1, "STFtV");
                    break;
                case 0x6:  //Pattern loop is the special thing...
                    if (l == 0)
                        strcpy(_arg1, "PTLpS");
                    else
                        strcpy(_arg1, "PTLpG");
                    break;
                case 0xa:
                    strcpy(_arg1, "EFVsU");
                    break;
                case 0xb:
                    strcpy(_arg1, "EFVsD");
                    break;
                case 0xc:
                    strcpy(_arg1, "ENCuT");
                    break;
                case 0xd:
                    strcpy(_arg1, "EDElY");
                    break;
            }
            break;

        case 0xa3:
        case 0xf:
            strcpy(_arg1, "SPDsT");
            break;

        case 0x10:
            strcpy(_arg1, "GVOlS");
            break;

        case 0x11:
            isBufferMem = true;
            strcpy(_arg1, "GVOlS");
            break;

        case 0x15:
            strcpy(_arg1, "EVPsS");
            break;

        case 0x1b:
            isBufferMem = true;
            strcpy(_arg1, "RETrG");
            break;

        // S3M/IT
        case 0x8d:
            strcpy(_arg1, "SURrD");
            break;
        case 0x87:
            strcpy(_arg1, "SETtP");
            break;
            // Btw; Txx effect is kinda special
	case 0x80:
	    strcpy(_arg1, "TRLvL");
	    break;


        default:
            snprintf(_arg1, 6, "???%02X", fxt);
    }
    return isBufferMem;
}
