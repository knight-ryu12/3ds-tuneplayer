#include <3ds.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "fastmode.h"
#include "linkedlist.h"
#include "sndthr.h"
#include "song_info.h"
#include "songhandler.h"
#include "songview.h"
#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))

#define DISABLE_LOOPCHK

//char *romfs_path = "romfs:/";

volatile uint64_t render_time = 0;
volatile uint64_t screen_time = 0;
volatile int runSound, playSound;
volatile int runMain;
struct xmp_frame_info fi;
volatile uint32_t _PAUSE_FLAG;

int player_config = 0;

char *search_path[] = {"romfs:/", "sdmc:/mod", "sdmc:/mods", "sdmc:/3ds/3ds-tuneplayer/mod"};

void threadPlayStopHook(APT_HookType hook, void *param) {
    switch (hook) {
        case APTHOOK_ONSUSPEND:
        case APTHOOK_ONSLEEP:
            playSound = 0;
            break;

        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            //Persist pause state
            //playSound = 1;
            break;

        case APTHOOK_ONEXIT:
            runSound = playSound = 0;
            runMain = 0;
            break;

        default:
            break;
    }
}

void printhelp() {
    printf("GREETZ TO ALL TUNE MAKERS!\n");
    printf("=CONTROLLS=\n");
    printf("A = show channel info\n");
    printf("B = show help\n");
    printf("X = show instrument info\n");
    printf("Y = show sample info\n");
    printf("DPAD LR = change songs\n");
    printf("DPAD UD = scroll\n");
    printf("Press UD with L pressed = change bottom screen info\n");
    printf("Start = Exit\n");
    printf("Select = Pause\n");
    printf("\n");
    printf("Thanks to all the people who helped me in different consoles\n");
    //printf("");
    printf("\n3ds-tuneplayer made by Chromaryu\n");
    printf("using libxmp v%s\n", xmp_version);
}

void sendError(char *error, int code) {
    // Seems like it doesn't like error message;
    errorConf err;
    errorInit(&err, ERROR_TEXT, CFG_LANGUAGE_EN);
    errorText(&err, error);
    errorCode(&err, code);
    errorDisp(&err);
}

void clean_console(PrintConsole *top, PrintConsole *bot) {
    consoleSelect(top);
    consoleClear();
    consoleSelect(bot);
    consoleClear();
}

int main(int argc, char *argv[]) {
    Result res;
    PrintConsole top, bot;
    uint32_t song_num = 0;
    int32_t main_prio;
    uint8_t info_flag = 0b00000000;
    uint8_t subsong = 0;
    int model;
    Thread snd_thr = NULL;
    struct xmp_module_info mi;
    static int isFT = 0;
    LLNode *current_song = NULL;
    aptHookCookie thr_playhook;

    //char status_msg[32];
    //snprintf(&status_msg, 31, "");
    // cur_tick = svcGetSystemTick();
    gfxInitDefault();
    consoleInit(GFX_TOP, &top);
    consoleInit(GFX_BOTTOM, &bot);
    consoleSelect(&top);

    model = try_speedup();

    res = romfsInit();
    printf("romFSInit %08lx\n", res);
    if (R_FAILED(res)) {
        sendError("Failed to initalize romfs... please check filesystem and such!\n", 0xFFFF0000);
        //printf("Failed to initalize romfs...\n");
        goto exit;
    }
    res = setup_ndsp();
    printf("setup_ndsp: %08lx\n", res);
    if (R_FAILED(res)) {
        //printf("Failed to initalize NDSP...\n");
        sendError("Failed to initalize NDSP... have you dumped your DSP rom?\n", 0xFFFF0001);
        goto exit;
    }
    res = aptInit();
    printf("aptInit %08lx\n", res);
    if (R_FAILED(res)) {
        printf("Error at aptInit()???? res %08lx\n", res);
        sendError("Error at aptInit()!?!", 0xFFFF0003);
        goto exit;
    }
    aptHook(&thr_playhook, threadPlayStopHook, NULL);
    // Forcing Errdisp
    //ERRF_ThrowResultWithMessage(0xd800456a, "FailTest!");
    //sendError("TestError.\n", 0xFFFFFFFF);
    xmp_context c = xmp_create_context();

    LinkedList ll = create_list();
    // Saerch for songs; first.
    song_num += searchsong(search_path[0], &ll);
    song_num += searchsong(search_path[1], &ll);
    song_num += searchsong(search_path[2], &ll);
    song_num += searchsong(search_path[3], &ll);

    printf("Songs: %d\n", song_num);
    //_debug_pause();
    // TODO: do better job at this
    if (song_num == 0) {
        //printf("There's no song at any of folders I've searched!\n");
        sendError("There's no songs playable!\n", 0xC0000000);
        goto exit;
    }

    current_song = ll.front;

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
    //if (loadSong(c, &mi, current_song->track_path, current_song->directory, &isFT) != 0) {
    if (loadSongMemory(c, &mi, current_song->track_path, current_song->directory, &isFT) != 0) {
        printf("Error on loadSong !!!?\n");

        goto exit;
    };
    clean_console(&top, &bot);
    xmp_get_frame_info(c, &fi);
    consoleSelect(&bot);
    runSound = playSound = 1;  // Get Ready.

    svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
    struct thread_data a = {c, model};
    if (h & KEY_L) {
        model = 0;
    }
    snd_thr = threadCreate(soundThread, &a, 32768, main_prio + 1,
                           model == 0 ? 0 : 2, true);
    consoleSelect(&top);

    int scroll = 0;
    int subscroll = 0;
    uint64_t timer_cnt = 0;
    // Main loop
    uint64_t first = 0;
    runMain = 1;
    bool isPrint = false;
    while (aptMainLoop()) {
        if (!runMain) {
            break;
        }  // Break to exit.
        //gspWaitForVBlank();
        //gfxSwapBuffers();
        first = svcGetSystemTick();
        // Check loop cnt
#ifndef DISABLE_LOOPCHK
        if (fi.loop_count > 0) {
            current_song = current_song->next;
            if (current_song == NULL)
                current_song = ll.front;
            playSound = 0;
            while (!_PAUSE_FLAG) {
                svcSleepThread(20000);
            }
            xmp_stop_module(c);
            clean_console(&top, &bot);
            gotoxy(0, 0);
            if (loadSongMemory(c, &mi, current_song->track_path, current_song->directory, &isFT) != 0) {
                printf("Error on loadSong !!!?\n");
                sendError("Error on loadsong...?\n", 0xFFFF0003);
                //ERRF_ThrowResultWithMessage();
                goto exit;
            }
            //_debug_pause();
            clean_console(&top, &bot);
            render_time = screen_time = 0;
            first = svcGetSystemTick();

            scroll = 0;
            playSound = 1;
        }
#endif

        show_generic_info(&fi, &mi, &top, &bot, model, subsong);
        /// 000 shows default info.
        if (info_flag & 1) {
            show_instrument_info(&mi, &top, &bot, &scroll, subscroll);
            show_channel_intrument_info(&fi, &mi, &top, &bot, &subscroll);
        } else if (info_flag & 2) {
            show_sample_info(&mi, &top, &bot, &scroll);
        } else if (info_flag & 4) {
            //Help.
            if (!isPrint) {
                consoleSelect(&top);
                printhelp();
                isPrint = true;
            }
        } else if (info_flag & 8) {
            if (!isPrint) {
                consoleSelect(&top);
                show_playlist(ll, current_song, &top, &bot, 0);
                isPrint = true;
            }
        } else {
            show_channel_info(&fi, &mi, &top, &bot, &scroll, isFT, subscroll);  // Fall back
            show_channel_info_btm(&fi, &mi, &top, &bot, &subscroll, isFT);
        }

        hidScanInput();

        // Your code goes here
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kHeld) {
            timer_cnt++;
            //printf("%d\n", timer_cnt);
        } else
            timer_cnt = 0;

        if (kDown & KEY_R) {
            clean_console(&top, &bot);
            isPrint = false;
            info_flag = 0b1000;
        }

        if (kDown & KEY_A) {
            clean_console(&top, &bot);
            info_flag = 0b000;
        }
        if (kDown & KEY_B) {
            clean_console(&top, &bot);
            isPrint = false;
            info_flag = 0b100;
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

        if (kDown & KEY_DOWN) {  //|| (kHeld & KEY_DOWN && timer_cnt >= 50)) {
            if (kHeld & KEY_L) {
                subscroll++;
            } else {
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
        }

        if (kDown & KEY_UP) {  //|| (kHeld & KEY_UP && timer_cnt >= 50)) {
            if (kHeld & KEY_L)
                if (subscroll != 0 && subscroll >= 0) subscroll--;
            if (scroll != 0 && scroll >= 0) scroll--;
        }

        if (kDown & KEY_RIGHT) {
            if (kHeld & KEY_L) {
                // Does it even have a subsong to play?
                if (mi.num_sequences >= 2) {
                    if (mi.num_sequences >= subsong + 1) {
                        playSound = 0;
                        while (!_PAUSE_FLAG) {
                            svcSleepThread(20000);
                        }
                        if (mi.num_sequences > subsong) subsong++;
                        //xmp_stop_module(c);
                        xmp_set_position(c, mi.seq_data[subsong].entry_point);
                        playSound = 1;
                    }
                }
            } else {
                current_song = current_song->next;
                if (current_song == NULL) current_song = ll.front;
                playSound = 0;
                while (!_PAUSE_FLAG) {
                    svcSleepThread(20000);
                }
                xmp_stop_module(c);
                clean_console(&top, &bot);
                gotoxy(0, 0);
                if (loadSongMemory(c, &mi, current_song->track_path, current_song->directory, &isFT) != 0) {
                    printf("Error on loadSong !!!?\n");
                    goto exit;
                };
                //_debug_pause();
                clean_console(&top, &bot);
                render_time = screen_time = 0;
                first = svcGetSystemTick();
                scroll = 0;
                subsong = 0;
                isPrint = false;
                playSound = 1;
            }
        }

        if (kDown & KEY_LEFT) {
            if (kHeld & KEY_L) {
                // Does it even have a subsong to play?
                if (mi.num_sequences >= 2) {
                    if (subsong != 0) {
                        playSound = 0;
                        while (!_PAUSE_FLAG) {
                            svcSleepThread(20000);
                        }
                        //xmp_stop_module(c);
                        if (subsong > 0) subsong--;
                        xmp_set_position(c, mi.seq_data[subsong].entry_point);
                        playSound = 1;
                        //subsong++;
                    }
                }
            } else {
                current_song = current_song->prev;
                if (current_song == NULL) current_song = ll.back;
                playSound = 0;
                while (!_PAUSE_FLAG) {
                    svcSleepThread(20000);
                }
                xmp_stop_module(c);
                clean_console(&top, &bot);
                gotoxy(0, 0);
                if (loadSongMemory(c, &mi, current_song->track_path, current_song->directory, &isFT) != 0) {
                    printf("Error on loadSong !!!?\n");
                    goto exit;
                };
                clean_console(&top, &bot);
                render_time = screen_time = 0;
                first = svcGetSystemTick();
                scroll = 0;
                subsong = 0;
                isPrint = false;
                playSound = 1;
            }
        }
        if (kDown & KEY_START) break;  // break in order to return to hbmenu
        screen_time = svcGetSystemTick() - first;
        gspWaitForVBlank();
    }
exit:
    playSound = 0;
    while (!_PAUSE_FLAG) {
        svcSleepThread(20000);
    }  // Sync
    xmp_stop_module(c);
    //_debug_pause();
    aptUnhook(&thr_playhook);
    runSound = 0;
    threadJoin(snd_thr, U64_MAX);
    xmp_end_player(c);
    xmp_release_module(c);
    xmp_free_context(c);
    free_list(&ll);
    aptExit();
    ndspExit();
    romfsExit();
    gfxExit();
    return 0;
}
