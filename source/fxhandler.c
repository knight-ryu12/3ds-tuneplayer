#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
            snprintf(_arg1, 6, "ARPeG");
            break;
        case 1:
            isBufferMem = true;
            snprintf(_arg1, 6, "PORtU");
            break;
        case 2:
            isBufferMem = true;
            snprintf(_arg1, 6, "PORtD");
            break;
        case 3:
            isBufferMem = true;
            snprintf(_arg1, 6, "TPOrT");
            break;
        case 4:
            isBufferMem = true;
            snprintf(_arg1, 6, "VIBrT");
            break;
        case 5:
            isBufferMem = true;
            snprintf(_arg1, 6, "TONvS");
            break;
        case 6:
            isBufferMem = true;
            snprintf(_arg1, 6, "VIBvS");
            break;
        case 7:
            isBufferMem = true;
            snprintf(_arg1, 6, "TREmO");
            break;
        case 8:
            snprintf(_arg1, 6, "PANsT");
            break;
        case 9:
            isBufferMem = true;
            snprintf(_arg1, 6, "OFStS");
            break;
        case 0xa:
        case 0xa4:
            if (isFT) goto ft2;
            // S3M/IT
            h = (fxp >> 4) & 0xF;
            l = fxp & 0xF;
            if (l == 0xF && h != 0)
                snprintf(_arg1, 6, "VOLsU");
            else if (h == 0xf && l != 0)
                snprintf(_arg1, 6, "VOLsD");
            else if (h == 0 && l != 0)  // S3M
                snprintf(_arg1, 6, "VOLsD");
            else if (h != 0 && l == 0)
                snprintf(_arg1, 6, "VOLsU");
            else if (h == 0xf && l == 0xf)  // ???
                snprintf(_arg1, 6, "VOLsS");
            break;
        ft2:
            isBufferMem = true;
            if ((fxp & 0x0F) == 0 && (fxp >> 4 & 0xF) > 0)  // Up
                snprintf(_arg1, 6, "VOLsU");
            else if (((fxp >> 4) & 0xF) == 0 && (fxp & 0x0F) > 0)  // Down
                snprintf(_arg1, 6, "VOLsD");
            break;

        case 0xb:
            snprintf(_arg1, 6, "POSjP");
            break;
        case 0xc:
            snprintf(_arg1, 6, "VOLsT");
            break;
        case 0xd:
            snprintf(_arg1, 6, "PTBrK");
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
                    snprintf(_arg1, 6, "EPRtU");
                    break;
                case 0x2:
                    isBufferMem = true;
                    snprintf(_arg1, 6, "EPRtD");
                    break;
                case 0x3:
                    // Not supported widely...?
                    break;
                case 0x4:
                    snprintf(_arg1, 6, "STVwF");
                    break;
                case 0x5:
                    snprintf(_arg1, 6, "STFtV");
                    break;
                case 0x6:  //Pattern loop is the special thing...
                    if (l == 0)
                        snprintf(_arg1, 6, "PTLpS");
                    else
                        snprintf(_arg1, 6, "PTLpG");
                    break;
                case 0xa:
                    snprintf(_arg1, 6, "EFVsU");
                    break;
                case 0xb:
                    snprintf(_arg1, 6, "EFVsD");
                    break;
                case 0xc:
                    snprintf(_arg1, 6, "ENCuT");
                    break;
                case 0xd:
                    snprintf(_arg1, 6, "EDElY");
                    break;
            }
            break;

        case 0xa3:
        case 0xf:
            snprintf(_arg1, 6, "SPDsT");
            break;

        case 0x10:
            snprintf(_arg1, 6, "GVOlS");
            break;

        case 0x11:
            isBufferMem = true;
            snprintf(_arg1, 6, "GVOlS");
            break;

        case 0x15:
            snprintf(_arg1, 6, "EVPsS");
            break;

        case 0x1b:
            isBufferMem = true;
            snprintf(_arg1, 6, "RETrG");
            break;

        // S3M/IT
        case 0x8d:
            snprintf(_arg1, 6, "SURrD");
            break;
        case 0x87:
            snprintf(_arg1, 6, "SETtP");
            break;
            // Btw; Txx effect is kinda special
	case 0x80:
	    snprintf(_arg1,6,"TRLvL");
	    break;


        default:
            snprintf(_arg1, 6, "???%02X", fxt);
    }
    return isBufferMem;
}
