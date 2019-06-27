#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmp.h>
#include "fastmode.h"
#include "sndthr.h"
#include "song_info.h"

#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))

#define ENABLE_ROMFS

char *romfs_path = "romfs:/";
char *track[19] = {"romfs:/A94FINAL.S3M",
                   "romfs:/nagz-xylan_orb_short.xm",
                   "romfs:/reed-imploder.xm",
                   "romfs:/12oz.s3m",
                   "romfs:/last step.xm",
                   "romfs:/mindrmr.xm",
                   "romfs:/seffren&jelly-o_m_g.xm",
                   "romfs:/resonance2.mod",
                   "romfs:/bz_fil.it",
                   "romfs:/ASTRAY.S3M",
                   "romfs:/seen_an_angel.xm",
                   "romfs:/sv_ttt.xm",
                   "romfs:/milky.xm",
                   "romfs:/ICEFRONT.S3M",
                   "romfs:/ghost_in_the_cli.mod",
                   "romfs:/universalnetwork2_real.xm",
                   "romfs:/pod.s3m",
                   "romfs:/vincent_voois_-_the_meaning_of_life.xm",
                   "romfs:/JEFF93.IT"};
int track_size = 19;
int track_quirk[1] = {XMP_MODE_ST3};

volatile int runSound, playSound;
struct xmp_frame_info fi;
// volatile int runRead, doRead;
volatile uint64_t d_t;
volatile uint32_t _PAUSE_FLAG;

char strbuf[255] = "romfs:/";

void _debug_pause() {
    printf("Press start.\n");
    while (1) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;
    }
}

void clean_console(PrintConsole *top, PrintConsole *bot) {
    consoleSelect(top);
    consoleClear();
    consoleSelect(bot);
    consoleClear();
}

int loadSong(xmp_context c, struct xmp_module_info *mi, char *path) {
    printf("Loading....\n");
    struct xmp_test_info ti;
    printf("%s\n", path);
    xmp_end_player(c);
    xmp_release_module(c);
    if (xmp_test_module(path, &ti) != 0) {
        printf("Illegal module detected.\n");
        return 2;
    }
    printf("%s\n%s\n", ti.name, ti.type);
    if (initXMP(path, c, mi) != 0) {
        printf("Error on initXMP!!\n");
        return 1;
    }
    svcSleepThread(1000000000);  // wait 1.5s
    xmp_start_player(c, SAMPLE_RATE, 0);
    return 0;
}

int main(int argc, char *argv[]) {
    Result res;
    PrintConsole top, bot;
    // uint64_t cur_tick,init_tick;
    int32_t main_prio;
    uint8_t info_flag = 0b00000000;
    int32_t i = 0;
    int model;
    Thread snd_thr = NULL;
    struct xmp_module_info mi;
    // cur_tick = svcGetSystemTick();
    gfxInitDefault();
    consoleInit(GFX_TOP, &top);
    consoleInit(GFX_BOTTOM, &bot);
    consoleSelect(&top);

    model = try_speedup();
    res = romfsInit();
    printf("romFSInit %08lx\n", res);
    res = setup_ndsp();
    printf("setup_ndsp: %08lx\n", res);
    if (R_FAILED(res)) {
        printf("Failed to initalize NDSP...\n");
        goto exit;
    }
    xmp_context c = xmp_create_context();

    printf("Registered Songs: %d\n", track_size);
    for (int i = 0; i < track_size; i++) {
        printf("%d: %s\n", i, track[i]);
    }

    hidScanInput();
    uint32_t h = hidKeysHeld();
    if (h & KEY_R) {
        model = 0;
        printf("Model force 0.\n");
        _debug_pause();
    }
    if (h & KEY_L) {
        model = 1;
        printf("Model force 1.\n");
        _debug_pause();
    }
    if (h & KEY_DUP) {
        _debug_pause();
    }

    if (loadSong(c, &mi, track[i]) != 0) {
        printf("Error on loadSong !!!?\n");
        goto exit;
    };
    clean_console(&top, &bot);
    xmp_get_frame_info(c, &fi);
    consoleSelect(&bot);
    // gotoxy(2,0); printf("%s\n%s\n",mi.mod->name,mi.mod->type);
    // s = APT_SetAppCpuTimeLimit(30);
    // toxy(0,0); printf("SetAppCpuTimeLimit(): %08lx\n",res);
    runSound = playSound = 1;
    svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
    struct thread_data a = {c, model};
    if (h & KEY_L) {
        model = 0;
    }
    snd_thr = threadCreate(soundThread, &a, 32768, main_prio + 1,
                           model == 0 ? 0 : 2, true);
    consoleSelect(&top);

    int scroll = 0;

    // Main loop
    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();

        // Check loop cnt
        if (fi.loop_count > 0) {
            i++;
            if (i > track_size - 1) {
                i = 0;
                continue;
            }
            playSound = 0;
            while (!_PAUSE_FLAG) {
                svcSleepThread(20000);
            }
            xmp_stop_module(c);
            clean_console(&top, &bot);
            gotoxy(0, 0);
            if (loadSong(c, &mi, track[i]) != 0) {
                printf("Error on loadSong !!!?\n");
                goto exit;
            }
            //_debug_pause();
            clean_console(&top, &bot);
            scroll = 0;
            playSound = 1;
        }

        show_generic_info(fi, mi, top, bot);
        /// 000 shows default info.
        if (info_flag & 1)
            show_instrument_info(mi, &top, &bot, &scroll);
        else if (info_flag & 2)
            show_sample_info(mi, &top, &bot, &scroll);
        else
            show_channel_info(fi, mi, &top, &bot, &scroll);  // Fall back
        hidScanInput();
        // Your code goes here
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A) {
            clean_console(&top, &bot);
            info_flag = 0b000;
        }
        if (kDown & KEY_B) {
        }
        if (kDown & KEY_X) {
            clean_console(&top, &bot);
            info_flag = 0b001;
        }
        if (kDown & KEY_Y) {
            clean_console(&top, &bot);
            info_flag = 0b010;
        }

        if (kDown & KEY_SELECT) playSound ^= 1;

        if (kDown & KEY_DOWN) {
            struct xmp_module *xm = mi.mod;
            //  does it have atleast 30 inst to show?
            if (xm->ins > 30) {
                scroll++;
            } else if (xm->chn > 30) {
                scroll++;
            } else if (xm->smp > 30) {
                scroll++;
            }
        }

        if (kDown & KEY_UP) {
            if (scroll != 0 && scroll >= 0) scroll--;
        }

        if (kDown & KEY_RIGHT) {
            i++;
            if (i > track_size - 1) {
                i = track_size - 1;
                continue;
            }
            playSound = 0;
            while (!_PAUSE_FLAG) {
                svcSleepThread(20000);
            }
            xmp_stop_module(c);
            clean_console(&top, &bot);
            gotoxy(0, 0);
            if (loadSong(c, &mi, track[i]) != 0) {
                printf("Error on loadSong !!!?\n");
                goto exit;
            };
            //_debug_pause();
            clean_console(&top, &bot);
            scroll = 0;
            playSound = 1;
        }

        if (kDown & KEY_LEFT) {
            i--;
            if (i < 0) {
                i = 0;
                continue;
            }
            playSound = 0;
            while (!_PAUSE_FLAG) {
                svcSleepThread(20000);
            }
            xmp_stop_module(c);
            clean_console(&top, &bot);
            gotoxy(0, 0);
            if (loadSong(c, &mi, track[i]) != 0) {
                printf("Error on loadSong !!!?\n");
                goto exit;
            };
            //_debug_pause();
            clean_console(&top, &bot);
            scroll = 0;
            playSound = 1;
        }

        if (kDown & KEY_START) break;  // break in order to return to hbmenu
    }
exit:
    playSound = 0;
    while (!_PAUSE_FLAG) {
        svcSleepThread(20000);
    }  // Sync
    xmp_stop_module(c);
    _debug_pause();
    runSound = 0;
    threadJoin(snd_thr, U64_MAX);
    xmp_end_player(c);
    xmp_release_module(c);
    xmp_free_context(c);
    ndspExit();
    romfsExit();
    gfxExit();
    return 0;
}
