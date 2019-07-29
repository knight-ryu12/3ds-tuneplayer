#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xmp.h>

void handleFX(uint8_t fxt, uint8_t fxp, char *_arg1, bool isFT) {
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
            snprintf(_arg1, 6, "PORtU");
            break;
        case 2:
            snprintf(_arg1, 6, "PORtD");
            break;
        case 3:
            snprintf(_arg1, 6, "TPOrT");
            break;
        case 4:
            snprintf(_arg1, 6, "VIBrT");
            break;
        case 5:
            snprintf(_arg1, 6, "TONvS");
            break;
        case 6:
            snprintf(_arg1, 6, "VIBvS");
            break;
        case 7:
            snprintf(_arg1, 6, "TREmO");
            break;
        case 8:
            snprintf(_arg1, 6, "PANsT");
            break;
        case 9:
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
            else if (h == 0xf || l == 0xf)  // ???
                snprintf(_arg1, 6, "VOLfS");
        ft2:
            if ((fxp & 0x0F) == 0 && (fxp >> 4 & 0xF) > 0)  // Up
                snprintf(_arg1, 6, "VOLsU");
            else if (((fxp >> 4) & 0xF) == 0 && (fxp & 0x0F) > 0)  // Down
                snprintf(_arg1, 6, "VOLsD");
            break;
        case 0xc:
            snprintf(_arg1, 6, "VOLsT");
            break;

        case 0xe:;  // in FTII, E denotes "Extended" effect; that means we need break
                    // it down
            h = (fxp >> 4) & 0xF;
            l = fxp & 0xF;
            fxp = l;
            switch (h) {
                default:
                    break;
                case 0x2:
                    snprintf(_arg1, 6, "EPRtD");
                    break;
                case 0x1:
                    snprintf(_arg1, 6, "EPRtU");
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
        case 0xd:
            snprintf(_arg1, 6, "BREaK");
            break;

        case 0x11:
            snprintf(_arg1, 6, "GVOlS");
            break;
        case 0xa3:
        case 0xf:
            snprintf(_arg1, 6, "SPDsT");
            break;

        case 0x1b:
            snprintf(_arg1, 6, "RETrG");
            break;

        // S3M/IT
        case 0x8d:
            snprintf(_arg1, 6, "SURrD");

        default:
            snprintf(_arg1, 6, "???%02X", fxt);
    }
}