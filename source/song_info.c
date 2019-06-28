#include "song_info.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <xmp.h>
#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))

static char *note_name[] = {"C-", "C#", "D-", "D#", "E-", "F-",
                            "F#", "G-", "G#", "A-", "A#", "B-"};
static char buf[16];
static char fx_buf[32];
static char fx2_buf[32];
static uint8_t old_note[256];
static uint8_t old_fxt[256];
static uint8_t old_fxp[256];
static uint8_t old_f2t[256];
static uint8_t old_f2p[256];

void show_generic_info(struct xmp_frame_info fi, struct xmp_module_info mi,
                       PrintConsole *top, PrintConsole *bot) {
    char secondbuf[32];
    snprintf(secondbuf, 32, "%02d:%02d/%02d:%02d", fi.time / 1000 / 60, fi.time / 1000 % 60, fi.total_time / 1000 / 60, fi.total_time / 1000 % 60);
    consoleSelect(bot);
    gotoxy(0, 0);
    printf("%02x %02x %02x %1x %3d %1d %s\n", fi.pos, fi.pattern, fi.row,
           fi.speed, fi.bpm, fi.loop_count, secondbuf);
    printf("%s\n%s\n", mi.mod->name, mi.mod->type);
}

void set_effect_memory(int ch, uint8_t fxp, uint8_t fxt, uint8_t *ofxt,
                       uint8_t *ofxp) {
    ofxp[ch] = fxp;
    ofxt[ch] = fxt;
}

uint8_t get_effect_memory(int ch, uint8_t *ofxt, uint8_t *ofxp) {
    return ofxp[ch];
}

void parse_fx(int ch, char *buf, uint8_t *ofxt, uint8_t *ofxp, uint8_t fxt,
              uint8_t fxp, bool isFT, bool isf2) {
    static char _arg1[8];
    bool isEFFM = false;
    bool isNNA = false;
    uint8_t _fxp = fxp;
    // ofxt[ch] = fxt;

    if ((fxt == 1 || fxt == 2 || fxt == 3 || fxt == 4 || fxt == 6 || fxt == 7 ||
         fxt == 0xa || fxt == 0x11)) {
        if (fxp == 0) {
            _fxp = get_effect_memory(ch, ofxt, ofxp);
            isEFFM = true;
        } else {
            isNNA = true;
            set_effect_memory(ch, fxp, fxt, ofxt, ofxp);
        }
    }

    snprintf(_arg1, 6, "-----");
    uint8_t h, l;
    switch (fxt) {
        case 0:
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
        case 0x11:
            snprintf(_arg1, 6, "GVOlS");
            break;
        case 0xa:
        case 0xa4:
            if (isFT) goto ft2;
            // S3M/IT
            h = (_fxp >> 4) & 0xF;
            l = _fxp & 0xF;
            if (l == 0xF && h != 0)
                snprintf(_arg1, 6, "VOLsU");
            else if (h == 0xf && l != 0)
                snprintf(_arg1, 6, "VOLsD");
            else if (h == 0xf || l == 0xf)
                snprintf(_arg1, 6, "VOLfS");
        ft2:
            if ((_fxp & 0x0F) == 0 && (_fxp >> 4 & 0xF) > 0)  // Up
                snprintf(_arg1, 6, "VOLsU");
            else if (((_fxp >> 4) & 0xF) == 0 && (_fxp & 0x0F) > 0)  // Down
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
                    snprintf(_arg1, 6, "E_CuT");
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

        case 0x1b:
            snprintf(_arg1, 6, "RETrG");
            break;
        default:
            snprintf(_arg1, 6, "???%02X", fxt);
    }
    snprintf(buf, 20, "%-5.5s%s%02X\e[0m", _arg1,
             isEFFM ? "\e[36m" : isNNA ? "\e[31m" : "\e[0m", _fxp);
    // free(_arg1);
}

void show_channel_info(struct xmp_frame_info fi, struct xmp_module_info mi,
                       PrintConsole *top, PrintConsole *bot, int *f, int isFT) {
    consoleSelect(top);
    gotoxy(0, 0);
    struct xmp_module *xm = mi.mod;
    int toscroll = *f;
    int cinf = xm->chn;
    int chmax = 0;
    if (cinf <= 29) {
        toscroll = 0;
        chmax = cinf;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < cinf - 29) {
        chmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= cinf - 29) {
        toscroll = cinf - 29;
        chmax = cinf;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }

    for (int i = toscroll; i < chmax; i++) {
        // Parse note
        struct xmp_channel_info ci = fi.channel_info[i];
        struct xmp_event ev = ci.event;
        if (ev.note > 0x80)
            snprintf(buf, 15, "===");
        else if (ev.note > 0) {
            snprintf(buf, 15, "\e[34;1m%s%d\e[0m", note_name[ev.note % 12],
                     ev.note / 12);
            old_note[i] = ev.note;
        } else if (ev.note == 0 && ci.volume != 0) {
            snprintf(buf, 15, "\e[36;1m%s%d\e[0m", note_name[old_note[i] % 12],
                     old_note[i] / 12);
        } else {
            snprintf(buf, 15, "---");
            old_note[i] = 0;
        }
        parse_fx(i, fx_buf, old_fxt, old_fxp, ev.fxt, ev.fxp, isFT, false);
        parse_fx(i, fx2_buf, old_f2t, old_f2p, ev.f2t, ev.f2p, isFT, true);
        printf("%2d:%c%02x %s%s%-4x %02x %02d%02d %5x %s %s\n", i + 1,
               ev.note != 0 ? '!' : ci.volume == 0 ? ' ' : 'G', ci.instrument, buf,
               ci.pitchbend < 0 ? "-" : "+", ci.pitchbend < 0 ? -(unsigned)ci.pitchbend : ci.pitchbend,
               ci.pan, ci.volume, ev.vol, ci.position, fx_buf, fx2_buf);
    }
}

void show_instrument_info(struct xmp_module_info mi, PrintConsole *top,
                          PrintConsole *bot, int *f) {
    // int line_lim = isN3DS?29:16;
    consoleSelect(top);
    gotoxy(0, 0);
    int toscroll = *f;
    struct xmp_module *xm = mi.mod;
    // char *ind = (char*)' ';
    bool ind_en = false;
    bool isind = false;
    int insmax = 0;
    if (xm->ins <= 29) {
        toscroll = 0;
        insmax = xm->ins;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < xm->ins - 29) {
        insmax = 29 + toscroll;
        isind = true;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= xm->ins - 29) {
        toscroll = xm->ins - 29;
        insmax = xm->ins;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }
    struct xmp_instrument *xi;
    for (int i = toscroll; i < insmax; i++) {
        xi = &xm->xxi[i];
        if (isind && toscroll == insmax - 1) ind_en = true;
        printf("%2x:\"%-32.32s\" %02x%04x %c%c%c%c\n", i, xi->name, xi->vol,
               xi->rls, xi->aei.flg & 1 ? 'A' : '-', xi->pei.flg & 1 ? 'P' : '-',
               xi->fei.flg & 1 ? 'F' : '-', ind_en ? 'v' : ' ');
    }
}

void show_sample_info(struct xmp_module_info mi, PrintConsole *top,
                      PrintConsole *bot, int *f) {
    int toscroll = *f;
    consoleSelect(top);
    gotoxy(0, 0);
    struct xmp_module *xm = mi.mod;
    int smpmax = 0;
    if (xm->smp <= 29) {
        toscroll = 0;
        smpmax = xm->smp;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < xm->smp - 29) {
        smpmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= xm->smp - 29) {
        toscroll = xm->smp - 29;
        smpmax = xm->smp;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }
    for (int i = toscroll; i < smpmax; i++) {
        struct xmp_sample *xs = &xm->xxs[i];
        // ignore sample that doesn't have size
        // if(xs->len==0) continue;

        printf("%2d:\"%-16.16s\" %5x%5x%5x %c%c%c%c%c%c\n", i, xs->name, xs->len,
               xs->lps, xs->lpe, xs->flg & XMP_SAMPLE_16BIT ? 'W' : '-',
               xs->flg & XMP_SAMPLE_LOOP ? 'L' : '-',
               xs->flg & XMP_SAMPLE_LOOP_BIDIR ? 'B' : '-',
               xs->flg & XMP_SAMPLE_LOOP_REVERSE ? 'R' : '-',
               xs->flg & XMP_SAMPLE_LOOP_FULL ? 'F' : '-',
               xs->flg & XMP_SAMPLE_SYNTH ? 'S' : '-');
    }
}

void show_channel_intrument_info(struct xmp_frame_info fi,
                                 struct xmp_module_info mi, PrintConsole *top,
                                 PrintConsole *bot, int *f) {}