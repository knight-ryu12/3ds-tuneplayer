#include <3ds.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
#include "hidmapper.h"
#include "player.h"
#include "song_info.h"
#include "songhandler.h"
#include "songview.h"
#define gotoxy(x, y) printf("\033[%d;%dH", (x), (y))

// 3ds-tuneplayer backed by libxmp (CMatsuoka, thx), by Chromaryu
// "80's praise on 2012 hardware" - Chromaryu

//char *romfs_path = "romfs:/";

Player g_player;

int player_config = 0;

typedef struct {
    uint64_t first;
    int scroll;
    int subscroll;
    uint8_t info_flag;
    bool isPrint;
    bool isBottomScreenPrint;
} main_loop_data;

static HIDBind infoscreen[];
static HIDBind configscreen[];
static void printhelp();

static HIDFUNC(ButtonStop) {
    g_player.terminate_flag = 1;
    return 1;
}

static HIDFUNC(ButtonNextSong) {
    main_loop_data* data = (main_loop_data*)arg;

    if (frame.held & KEY_L) {
        Player_NextSubSong(&g_player);
    } else {
        Player_ClearConsoles(&g_player);
        gotoxy(0, 0);
        if (Player_NextSong(&g_player) != 0) {
            printf("Error on loadSong !!!?\n");
            return -1;
        };
        //_debug_pause();
        Player_ClearConsoles(&g_player);
        g_player.render_time = g_player.screen_time = 0;
        data->first = svcGetSystemTick();
        data->scroll = 0;
        data->isPrint = false;
        data->isBottomScreenPrint = false;
    }
    return 1;
}

static HIDFUNC(ButtonPrevSong) {
    main_loop_data* data = (main_loop_data*)arg;

    if (frame.held & KEY_L) {
        Player_PrevSubSong(&g_player);
    } else {
        Player_ClearConsoles(&g_player);
        gotoxy(0, 0);
        if (Player_PrevSong(&g_player) != 0) {
            printf("Error on loadSong !!!?\n");
            return -1;
        };
        Player_ClearConsoles(&g_player);
        g_player.render_time = g_player.screen_time = 0;
        data->first = svcGetSystemTick();
        data->scroll = 0;
        data->isPrint = false;
        data->isBottomScreenPrint = false;
    }
    return 1;
}

static HIDFUNC(ButtonUpScroll) {
    main_loop_data* data = (main_loop_data*)arg;

    data->isPrint = false;
    if (frame.held & KEY_L) {
        if (data->subscroll > 0) data->subscroll--;
    } else if (data->scroll > 0)
        data->scroll--;
    return 0;
}

static HIDFUNC(ButtonDownScroll) {
    main_loop_data* data = (main_loop_data*)arg;

    data->isPrint = false;
    if (frame.held & KEY_L) {
        data->subscroll++;
    } else {
        data->scroll++;
    }
    return 0;
}

static HIDFUNC(ButtonPause) {
    Player_TogglePause(&g_player);
    return 0;
}

static HIDFUNC(ButtonNextInfoScreen) {
    main_loop_data* data = (main_loop_data*)arg;

    int ret = 0;

    if (data->info_flag > 3 && data->info_flag != 8) {
        if (R_FAILED(HIDMapper_SetMapping(infoscreen, false))) return -1;
        ret = 1;
    }

    Player_ClearConsoles(&g_player);
    data->isPrint = false;
    if (++data->info_flag > 3) {
        // 012012012
        data->info_flag = 0;
    }
    data->isBottomScreenPrint = false;
    return ret;
}

static HIDFUNC(ButtonConfigSaveAndExit) {
    main_loop_data* data = (main_loop_data*)arg;

    // TODO

    if (R_FAILED(HIDMapper_SetMapping(infoscreen, false))) return -1;

    data->isPrint = false;
    data->isBottomScreenPrint = false;
    data->info_flag = 0;
    data->scroll = 0;

    return 1;
}

static HIDFUNC(ButtonPlaylistScreen) {
    main_loop_data* data = (main_loop_data*)arg;

    int ret = 0;

    // info and playlist screen share the same layout, only change if need be
    if (data->info_flag > 3 && data->info_flag != 8) {
        if (R_FAILED(HIDMapper_SetMapping(infoscreen, false))) return -1;
        ret = 1;
    }

    data->isPrint = false;
    data->isBottomScreenPrint = false;
    data->info_flag = 8;
    data->scroll = 0;

    return ret;
}

static HIDFUNC(ButtonConfigScreen) {
    main_loop_data* data = (main_loop_data*)arg;

    if (R_FAILED(HIDMapper_SetMapping(configscreen, false))) return -1;

    data->isPrint = false;
    data->isBottomScreenPrint = false;
    data->info_flag = 16;
    data->scroll = 0;

    return 1;
}

static HIDFUNC(HIDLoopInfoAction) {
    main_loop_data* data = (main_loop_data*)arg;

    switch (data->info_flag) {
        case 0:
            Player_PrintChannel(&g_player, &data->scroll, &data->subscroll);
            break;
        case 1:
            if (!data->isPrint) {
                Player_PrintInstruments(&g_player, &data->scroll, data->subscroll);
                data->isPrint = true;
            }
            Player_PrintChannelInstruments(&g_player, &data->subscroll);
            break;
        case 2:
            if (!data->isPrint) {
                Player_PrintSamples(&g_player, &data->scroll);
                data->isPrint = true;
            }
            break;
        case 3:
            if (!data->isPrint) {
                Player_ClearTop(&g_player);
                printhelp();
                data->isPrint = true;
            }
        case 8:
            if (!data->isPrint) {
                Player_ClearTop(&g_player);
                Player_PrintPlaylist(&g_player, &data->scroll, &data->subscroll);
                data->isPrint = true;
            }
        default:
            break;
    }

    return 0;
}

static HIDFUNC(HIDLoopConfigAction) {
    main_loop_data* data = (main_loop_data*)arg;

    // todo, do config control actions

    if (!data->isPrint) {
        Player_ClearTop(&g_player);
        Player_ConfigsScreen(&g_player, &data->subscroll);
        data->isPrint = true;
    }

    return 0;
}

static void printhelp() {
    printf("GREETZ TO ALL TUNE MAKERS!\n");
    printf("=CONTROLS=\n");
    printf("A = change screens\n");
    printf("DPAD LR = change songs\n");
    printf("DPAD UD = scroll\n");
    printf("Press UD with L pressed = change bottom screen info\n");
    printf("Start = Exit\n");
    printf("Select = Pause\n");
    printf("\n");
    printf("Thanks to all the people who helped me in different consoles\n");
    printf("See credit file for seeing credits included in ROMFS.\n");
    printf("\n3ds-tuneplayer made by Chromaryu\n");
    printf("using libxmp v\e[33m%s\e[0m\n", xmp_version);
}

static HIDBind infoscreen[] = {
    {0, true, &HIDLoopInfoAction, NULL},
    {KEY_START, true, &ButtonStop, NULL},
    {KEY_SELECT, true, &ButtonPause, NULL},
    {KEY_UP, true, &ButtonUpScroll, NULL},
    {KEY_DOWN, true, &ButtonDownScroll, NULL},
    {KEY_RIGHT, true, &ButtonNextSong, NULL},
    {KEY_LEFT, true, &ButtonPrevSong, NULL},
    {KEY_Y, true, &ButtonConfigScreen, NULL},
    {KEY_A, true, &ButtonNextInfoScreen, NULL},
    {KEY_R, true, &ButtonPlaylistScreen, NULL},
    {HIDBINDNULL}};

static HIDBind configscreen[] = {
    {0, true, &HIDLoopConfigAction, NULL},
    {KEY_START, true, &ButtonStop, NULL},
    {KEY_SELECT, true, &ButtonPause, NULL},
    {KEY_UP, true, &ButtonUpScroll, NULL},
    {KEY_DOWN, true, &ButtonDownScroll, NULL},
    {KEY_Y, true, &ButtonConfigSaveAndExit, NULL},
    {HIDBINDNULL}};

int main(int argc, char* argv[]) {
    if (Player_Init(&g_player))
        return 0;

    //uint64_t timer_cnt = 0;
    // Main loop

    main_loop_data data = {0, 0, 0, 0b00000000, false, false};

    infoscreen[0].arg = configscreen[0].arg = (void*)&data;
    infoscreen[3].arg = configscreen[3].arg = (void*)&data;
    infoscreen[4].arg = configscreen[4].arg = (void*)&data;
    infoscreen[5].arg = (void*)&data;
    infoscreen[6].arg = (void*)&data;
    infoscreen[7].arg = (void*)&data;
    infoscreen[8].arg = (void*)&data;
    infoscreen[9].arg = (void*)&data;
    configscreen[5].arg = (void*)&data;

    if (R_FAILED(HIDMapper_SetMapping(infoscreen, false))) {
        printf("HID Fail.\n");
        g_player.terminate_flag = true;
    }

    while (aptMainLoop() && !g_player.terminate_flag) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        data.first = svcGetSystemTick();
        // Check loop cnt
        if (g_player.playerConfig.loopcheck > 0) {
            if (g_player.finfo.loop_count > g_player.playerConfig.loopcheck) {
                Player_ClearConsoles(&g_player);
                gotoxy(0, 0);
                if (Player_NextSong(&g_player) != 0) {
                    // This should not happen.
                    printf("Error on loadSong !!!?\n");
                    sendError("Error on loadsong...?\n", 0xFFFF0003);
                    break;
                }
                //_debug_pause();
                Player_ClearConsoles(&g_player);
                g_player.render_time = g_player.screen_time = 0;
                data.isBottomScreenPrint = false;
                data.first = svcGetSystemTick();
                data.scroll = 0;
            }
        }
        if (!data.isBottomScreenPrint) {
            Player_PrintTitle(&g_player);
            data.isBottomScreenPrint = true;
        }
        Player_PrintGeneric(&g_player);

        if (R_FAILED(HIDMapper_RunFrame())) break;

        g_player.screen_time = svcGetSystemTick() - data.first;
    }

    HIDMapper_ClearMapping();
    Player_Exit(&g_player);
    return 0;
}
