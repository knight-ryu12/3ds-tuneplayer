#include "song_info.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "fxhandler.h"
#include "player.h"

#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))

static const char *note_name[] = {"C-", "C#", "D-", "D#", "E-", "F-",
                                  "F#", "G-", "G#", "A-", "A#", "B-"};
static char buf[32];
static char fx_buf[32];
static char fx2_buf[32];
static uint8_t old_note[256];
static uint8_t old_fxp[256][256];
static uint8_t old_f2p[256][256];
static char timebuf[22];
static char infobuf[256];
extern Player g_player;

void show_generic_info(struct xmp_frame_info *fi, struct xmp_module_info *mi,
                       PrintConsole *top, PrintConsole *bot, int cur_subsong) {
    const float SYS_TICK = CPU_TICKS_PER_MSEC;  // seems like 3ds uses this clock no matter what
    uint16_t cur = 0;
    snprintf(timebuf, 22, "%02d:%02d/%02d:%02d", fi->time / 1000 / 60, fi->time / 1000 % 60, fi->total_time / 1000 / 60, fi->total_time / 1000 % 60);
    consoleSelect(bot);
    gotoxy(4, 0);
    // TODO: write whatever the place, maybe?
    if (g_player.play_sound)
        cur += snprintf(infobuf, 256, "Pos[%02X/%02X] Pat[%02X/%02X] Row[%02X/%02X]\nSpd%1X BPM%3d LC%1d Ss%1d/%1d\nChn[%02X/%02X] %s\n",
                        fi->pos, mi->mod->len - 1, fi->pattern, mi->mod->pat - 1, fi->row, fi->num_rows - 1,
                        fi->speed, fi->bpm, fi->loop_count, cur_subsong, mi->num_sequences - 1,
                        fi->virt_used, fi->virt_channels, timebuf);
    else
        gotoxy(7, 0);

    //cur += snprintf(&infobuf[cur], (256 - cur) < 0 ? 0 : (256 - cur), "%s\n%s\n", mi->mod->name, mi->mod->type); // maybe print once there
    cur += snprintf(&infobuf[cur], (256 - cur) < 0 ? 0 : (256 - cur), "RT%0.2fms ST%0.2fms MT%0.2fms     \n", g_player.render_time / SYS_TICK, g_player.screen_time / SYS_TICK, (g_player.render_time + g_player.screen_time) / SYS_TICK);
    cur += snprintf(&infobuf[cur], (256 - cur) < 0 ? 0 : (256 - cur), "Player: %-8s", !g_player.play_sound ? "Paused" : "Playing");
    write(STDOUT_FILENO, infobuf, cur > 256 ? 256 : cur);
}

void show_title(struct xmp_module_info *mi, const char *filename, PrintConsole *bot) {
    gotoxy(0, 0);
    consoleSelect(bot);
    printf("%s\n%s\n", mi->mod->name[0] == 0 ? filename : mi->mod->name, mi->mod->type);
}

// Seems like FT does have memory for each effects...
// I doubt not saving each one is bad.

void set_effect_memory(int ch, uint8_t fxp, uint8_t fxt, uint8_t ofxp[256][256]) {
    ofxp[fxt][ch] = fxp;
}

uint8_t get_effect_memory(int ch, uint8_t ofxp[256][256], uint8_t fxt) {
    return ofxp[fxt][ch];
}

void parse_fx(int ch, char *buf, uint8_t ofxp[256][256], uint8_t fxt,
              uint8_t fxp, bool isFT, bool isf2) {
    char _arg1[8];
    const char *p_arg1 = "-----";
    const char *p_arg2 = "\e[37m";
    bool isEFFM = false;
    //bool isNNA = false;
    uint8_t _fxp = fxp;
    bool isBufferMem = handleFX(fxt, _fxp, &p_arg1, &p_arg2, _arg1, isFT);
    if (isBufferMem) {
        if (fxp == 0) {
            _fxp = get_effect_memory(ch, ofxp, fxt);
            isEFFM = true;
            if (_fxp != 0) handleFX(fxt, _fxp, &p_arg1, &p_arg2, _arg1, isFT);  // calling once again with updated information if needed.
        } else {
            //isNNA = true;
            set_effect_memory(ch, fxp, fxt, ofxp);
        }
    }

    snprintf(buf, 32, "%s%-5.5s\e[0m%s%02X\e[0m", p_arg2, p_arg1,
             isEFFM ? "\e[36m" : "\e[0m", _fxp);
    // free(_arg1);
}

void show_channel_info(struct xmp_frame_info *fi, struct xmp_module_info *mi,
                       PrintConsole *top, PrintConsole *bot, int *f, int isFT, int hilight, int mode) {
    if (!g_player.play_sound) return;
    consoleSelect(top);
    gotoxy(0, 0);
    struct xmp_module *xm = mi->mod;
    int toscroll = *f;
    int cinf = xm->chn;
    int chmax = 0;
    char ind = ' ';
    bool isuind = false;
    bool isdind = false;
    if (cinf <= 29) {
        toscroll = 0;
        chmax = cinf;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < cinf - 29) {
        if (toscroll != 0) isuind = true;
        isdind = true;
        chmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= cinf - 29) {
        isuind = true;
        toscroll = cinf - 29;
        chmax = cinf;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }

    for (int i = toscroll; i < chmax; i++) {
        ind = ' ';
        if (i == toscroll && isuind) ind = '^';  // There is a lot more to write
        if (i == chmax - 1 && isdind) ind = 'v';
        // Parse note
        struct xmp_channel_info *ci = &fi->channel_info[i];
        struct xmp_event *ev = &ci->event;
        if (ev->note > 0x80)
            strcpy(buf, "===");
        else if (ev->note > 0) {
            snprintf(buf, 15, "\e[34;1m%s%d\e[0m", note_name[ev->note % 12],
                     ev->note / 12);
            old_note[i] = ev->note;
        } else if (ev->note == 0 && ci->volume != 0) {
            snprintf(buf, 15, "\e[36;1m%s%d\e[0m", note_name[old_note[i] % 12],
                     old_note[i] / 12);
        } else {
            strcpy(buf, "---");
            old_note[i] = 0;
        }
        parse_fx(i, fx_buf, old_fxp, ev->fxt, ev->fxp, isFT, false);
        parse_fx(i, fx2_buf, old_f2p, ev->f2t, ev->f2p, isFT, true);
        printf("%s%2d\e[0m:%c%02x%s", i == hilight ? "\e[36;1m" : "\e[0m", i,
               ev->note != 0 ? '!' : ci->volume == 0 ? ' ' : 'G', ci->instrument, buf);
        if (!mode)
            printf("%s%-4x %02x %02X%02X %s %s%c\n",
                   ci->pitchbend < 0 ? "-" : "+", ci->pitchbend < 0 ? -(unsigned)ci->pitchbend : ci->pitchbend,
                   ci->pan, ci->volume, ev->vol, fx_buf, fx2_buf, ind);
        else {
            char *n;
            char sign[8] = "";
            if(ci->sample < xm->smp) {
            struct xmp_sample *xs = &xm->xxs[ci->sample];
            n = xs->name;
            sign = "S\e[32m";
            }
            if(n[0] == 0 && ci->instrument < xm->ins) {
            struct xmp_instrument *xi = &xm->xxi[ci->instrument];
            n = xi->name;
            sign = "I\e[31m";
            }
            if(n[0] == 0) n = "";
            printf("%s%-32.32s\e[0m%c%c%c%c%c%c%c\n", sign, n,
                   xs->flg & XMP_SAMPLE_16BIT ? 'W' : '-',
                   xs->flg & XMP_SAMPLE_LOOP ? 'L' : '-',
                   xs->flg & XMP_SAMPLE_LOOP_BIDIR ? 'B' : '-',
                   xs->flg & XMP_SAMPLE_LOOP_REVERSE ? 'R' : '-',
                   xs->flg & XMP_SAMPLE_LOOP_FULL ? 'F' : '-',
                   xs->flg & XMP_SAMPLE_SYNTH ? 'S' : '-', ind);
        }
    }
}

void show_instrument_info(struct xmp_module_info *mi, PrintConsole *top,
                          PrintConsole *bot, int *f, int hilight) {
    consoleSelect(top);
    gotoxy(0, 0);
    int toscroll = *f;
    struct xmp_instrument *xi;
    struct xmp_module *xm = mi->mod;
    char ind = ' ';
    bool isuind = false;
    bool isdind = false;
    int insmax = 0;
    if (xm->ins <= 29) {
        toscroll = 0;
        insmax = xm->ins;
        // pat a (there's no exceeding limit. ignore f solely.)
    } else if (toscroll < xm->ins - 29) {
        if (toscroll != 0) isuind = true;
        isdind = true;
        insmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= xm->ins - 29) {
        isuind = true;
        toscroll = xm->ins - 29;
        insmax = xm->ins;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }
    for (int i = toscroll; i < insmax; i++) {
        ind = ' ';
        if (i == toscroll && isuind) ind = '^';  // There is a lot more to write
        if (i == insmax - 1 && isdind) ind = 'v';
        xi = &xm->xxi[i];
        //if (isind && toscroll == insmax - 1) ind_en = true;
        printf("%s%2x\e[0m:\"%-32.32s\" %02x%04x %c%c%c%c\n", i == hilight ? "\e[36;1m" : "\e[0m", i, xi->name, xi->vol,
               xi->rls, xi->aei.flg & 1 ? 'A' : '-', xi->pei.flg & 1 ? 'P' : '-',
               xi->fei.flg & 1 ? 'F' : '-', ind);
    }
}

void show_sample_info(struct xmp_module_info *mi, PrintConsole *top,
                      PrintConsole *bot, int *f) {
    int toscroll = *f;
    consoleSelect(top);
    gotoxy(0, 0);
    struct xmp_module *xm = mi->mod;
    int smpmax = 0;
    char ind = ' ';
    bool isuind = false;
    bool isdind = false;
    if (xm->smp <= 29) {
        toscroll = 0;
        smpmax = xm->smp;
        // pat a (there's no exceeding limit. ignore f solely.) // and not doing any indicator work
    } else if (toscroll < xm->smp - 29) {
        if (toscroll != 0) isuind = true;
        isdind = true;
        smpmax = 29 + toscroll;
        // pat b (there's scroll need. but f is under xm->ins-29)
    } else if (toscroll >= xm->smp - 29) {
        isuind = true;
        toscroll = xm->smp - 29;
        smpmax = xm->smp;
        *f = toscroll;
        // pat c (toscroll reaches (xm->ins-29))
    }
    for (int i = toscroll; i < smpmax; i++) {
        ind = ' ';
        struct xmp_sample *xs = &xm->xxs[i];
        if (i == toscroll && isuind) ind = '^';  // There is a lot more to write
        if (i == smpmax - 1 && isdind) ind = 'v';
        // ignore sample that doesn't have size
        // if(xs->len==0) continue;

        printf("%2d:\"%-16.16s\" %5x%5x%5x %c%c%c%c%c%c%c\n", i, xs->name, xs->len,
               xs->lps, xs->lpe, xs->flg & XMP_SAMPLE_16BIT ? 'W' : '-',
               xs->flg & XMP_SAMPLE_LOOP ? 'L' : '-',
               xs->flg & XMP_SAMPLE_LOOP_BIDIR ? 'B' : '-',
               xs->flg & XMP_SAMPLE_LOOP_REVERSE ? 'R' : '-',
               xs->flg & XMP_SAMPLE_LOOP_FULL ? 'F' : '-',
               xs->flg & XMP_SAMPLE_SYNTH ? 'S' : '-',  // Is Adlib even used?
               ind);
    }
}

void show_channel_instrument_info(struct xmp_frame_info *fi,
                                  struct xmp_module_info *mi, PrintConsole *top,
                                  PrintConsole *bot, int *s) {
    consoleSelect(bot);
    struct xmp_instrument *xi;
    // How many inst do I have
    if (mi->mod->ins - 1 < *s) {
        *s = mi->mod->ins - 1;
        return;
    }
    xi = &mi->mod->xxi[*s];
    gotoxy(7, 0);
    //if (xi == NULL) return;
    printf("n:%-32.32s\n", xi->name);
    printf("RLS %02x\n", xi->rls);
    printf("NSM %02x\n", xi->nsm);
}

void show_channel_info_btm(struct xmp_frame_info *fi, struct xmp_module_info *mi,
                           PrintConsole *top, PrintConsole *bot, int *s, int isFT) {
    consoleSelect(bot);
    gotoxy(10, 0);
    if (mi->mod->chn - 1 < *s) {
        *s = mi->mod->chn - 1;
        return;
    }
    int cur_smp_n = fi->channel_info[*s].sample;
    int cur_ins_n = fi->channel_info[*s].instrument;
    struct xmp_sample *cur_smp = &mi->mod->xxs[cur_smp_n];
    struct xmp_instrument *xi = &mi->mod->xxi[cur_ins_n];
    // does this cur_smp and xi exists?
    if (mi->mod->ins < cur_ins_n) return;
    if (mi->mod->smp < cur_smp_n) return;

    // Inital thing
    // Check loop flag
    char *loopflg;
    if (cur_smp->flg & XMP_SAMPLE_LOOP) {
        if (cur_smp->flg & XMP_SAMPLE_LOOP_BIDIR)
            loopflg = "BiDi";
        else if (cur_smp->flg & XMP_SAMPLE_LOOP_REVERSE)
            loopflg = "Revs";
        else if (cur_smp->flg & XMP_SAMPLE_LOOP_FULL)
            loopflg = "Full";
        else
            loopflg = "Forw";
    } else
        loopflg = "----";

    printf("\n=Info=\n");
    printf("Channel %2d: smp/ins%02d:%02d\n", *s, cur_smp_n, cur_ins_n);
    printf("s%-5x c%-5x ls%-5x le%-5x\n", cur_smp->len, fi->channel_info[*s].position, cur_smp->lps, cur_smp->lpe);
    printf("n:%-32.32s\n", cur_smp->name);
    printf("m> %s,%s\n", cur_smp->flg & XMP_SAMPLE_16BIT ? "16b" : " 8b", loopflg);
    printf("i:\"%-32.32s\"", xi->name);
    printf("%d\n", xi->vol);
}
