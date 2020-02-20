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

typedef struct {
    main_loop_data *main;
    int listscroll;
} config_loop_data;

static HIDBind infoscreen[];
static HIDBind configscreen[];
static void printhelp();

static HIDFUNC(ButtonStop) {
    g_player.terminate_flag = 1;
    return HIDBINDCANCELFRAME;
}

static HIDFUNC(ButtonSwapSong) {
    main_loop_data* data = (main_loop_data*)arg;

    if (frame.held & KEY_L) {
        if (frame.pressed & KEY_RIGHT)
            Player_NextSubSong(&g_player);
        else
            Player_PrevSubSong(&g_player);
    } else {
        Player_ClearConsoles(&g_player);
        gotoxy(0, 0);
        if (((frame.pressed & KEY_RIGHT) ? Player_NextSong(&g_player) : Player_PrevSong(&g_player)) != 0) {
            printf("Error on loadSong !!!?\n");
            return HIDBINDERROR;
        };
        //_debug_pause();
        Player_ClearConsoles(&g_player);
        g_player.render_time = g_player.screen_time = 0;
        data->first = svcGetSystemTick();
        data->scroll = 0;
        data->isPrint = false;
        data->isBottomScreenPrint = false;
    }
    return HIDBINDCANCELFRAME;
}

static HIDFUNC(ButtonUpDownScroll) {
    main_loop_data* data = (main_loop_data*)arg;

    data->isPrint = false;
    if (frame.held & KEY_L) {
        if (frame.pressed & KEY_UP) {
            if (data->subscroll > 0)
                data->subscroll--;
        } else {
            data->subscroll++;
        }
    } else {
        if (frame.pressed & KEY_UP) {
            if (data->scroll > 0)
                data->scroll--;
        } else {
            data->scroll++;
        }
    }
    return HIDBINDOK;
}

static HIDFUNC(ButtonPause) {
    Player_TogglePause(&g_player);
    return HIDBINDOK;
}

static HIDFUNC(ButtonNextInfoScreen) {
    main_loop_data* data = (main_loop_data*)arg;

    int ret = HIDBINDOK;

    if (data->info_flag > 3 && data->info_flag != 8) {
        if (R_FAILED(HIDMapper_SetMapping(infoscreen, false)))
            return HIDBINDERROR;
        ret = HIDBINDCHANGEDFRAME;
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

    if (R_FAILED(HIDMapper_SetMapping(infoscreen, false)))
        return HIDBINDERROR;

    data->isPrint = false;
    data->isBottomScreenPrint = false;
    data->info_flag = 0;
    data->scroll = 0;
    data->subscroll = 0;

    Player_ClearConsoles(&g_player);

    return HIDBINDCHANGEDFRAME;
}

static HIDFUNC(ButtonConfigLeftRight) {
    main_loop_data* data = (main_loop_data*)arg;

    if (frame.pressed & KEY_LEFT) data->subscroll--;
    else data->subscroll++;
    return HIDBINDCANCELFRAME;
}

static HIDFUNC(ButtonPlaylistScreen) {
    main_loop_data* data = (main_loop_data*)arg;

    int ret = HIDBINDOK;

    // info and playlist screen share the same layout, only change if need be
    if (data->info_flag > 3 && data->info_flag != 8) {
        if (R_FAILED(HIDMapper_SetMapping(infoscreen, false)))
            return HIDBINDERROR;
        ret = HIDBINDCHANGEDFRAME;
    }

    data->isPrint = false;
    data->isBottomScreenPrint = false;
    data->info_flag = 8;
    data->scroll = 0;

    return ret;
}

static HIDFUNC(ButtonConfigScreen) {
    config_loop_data* data = (config_loop_data*)arg;

    if (R_FAILED(HIDMapper_SetMapping(configscreen, false)))
        return HIDBINDERROR;

    Player_ClearConsoles(&g_player);

    data->main->isPrint = false;
    data->main->isBottomScreenPrint = false;
    data->main->info_flag = 16;
    data->main->scroll = 0;
    data->main->subscroll = 1;
    data->listscroll = 0;

    return HIDBINDCHANGEDFRAME;
}

static HIDFUNC(HIDLoopInfoAction) {
    main_loop_data* data = (main_loop_data*)arg;

    switch (data->info_flag) {
        case 0:
            Player_PrintChannel(&g_player, &data->scroll, &data->subscroll, 0);
            break;

        case 1:
            Player_PrintChannel(&g_player, &data->scroll, &data->subscroll, 1);
            break;
        case 2:
            if (!data->isPrint) {
                Player_PrintInstruments(&g_player, &data->scroll, data->subscroll);
                data->isPrint = true;
            }
            Player_PrintChannelInstruments(&g_player, &data->subscroll);
            break;
        case 3:
            if (!data->isPrint) {
                Player_PrintSamples(&g_player, &data->scroll);
                data->isPrint = true;
            }
            break;
        case 4:
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

    return HIDBINDOK;
}

static HIDFUNC(HIDMainLoopStart) {
    main_loop_data* data = (main_loop_data*)arg;

    data->first = svcGetSystemTick();
    // Check loop cnt
    if (g_player.playerConfig.loopcheck > 0) {
        if (g_player.finfo.loop_count > g_player.playerConfig.loopcheck) {
            Player_ClearConsoles(&g_player);
            gotoxy(0, 0);
            if (Player_NextSong(&g_player) != 0) {
                // This should not happen.
                printf("Error on loadSong !!!?\n");
                sendError("Error on loadsong...?\n", 0xFFFF0003);
                return HIDBINDERROR;
            }
            //_debug_pause();
            Player_ClearConsoles(&g_player);
            g_player.render_time = g_player.screen_time = 0;
            data->isBottomScreenPrint = false;
            data->first = svcGetSystemTick();
            data->scroll = 0;
        }
    }
    if (!data->isBottomScreenPrint) {
        Player_PrintTitle(&g_player);
        data->isBottomScreenPrint = true;
    }
    Player_PrintGeneric(&g_player);

    return HIDBINDOK;
}

static HIDFUNC(HIDMainLoopEnd) {
    main_loop_data* data = (main_loop_data*)arg;

    g_player.screen_time = svcGetSystemTick() - data->first;

    return HIDBINDOK;
}

static HIDFUNC(HIDLoopConfigAction) {
    config_loop_data* data = (config_loop_data*)arg;

    if (!data->main->isPrint || data->main->subscroll != 1) {
        Player_ConfigsScreen(&g_player, &data->listscroll, &data->main->scroll, data->main->subscroll - 1);
        data->main->isPrint = true;
        data->main->subscroll = 1;
    }

    return HIDBINDOK;
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
    {0, true, false, &HIDMainLoopStart, NULL},
    {0, true, true, &HIDLoopInfoAction, NULL},
    {KEY_START, true, true, &ButtonStop, NULL},
    {KEY_SELECT, true, true, &ButtonPause, NULL},
    {KEY_UP | KEY_DOWN, true, true, &ButtonUpDownScroll, NULL},
    {KEY_RIGHT | KEY_LEFT, true, true, &ButtonSwapSong, NULL},
    {KEY_Y, true, true, &ButtonConfigScreen, NULL},
    {KEY_A, true, true, &ButtonNextInfoScreen, NULL},
    {KEY_R, true, true, &ButtonPlaylistScreen, NULL},
    {0, true, false, &HIDMainLoopEnd, NULL},
    {HIDBINDNULL}};

static HIDBind configscreen[] = {
    {0, true, true, &HIDLoopConfigAction, NULL},
    {KEY_START, true, true, &ButtonStop, NULL},
    {KEY_SELECT, true, true, &ButtonPause, NULL},
    {KEY_UP | KEY_DOWN, true, true, &ButtonUpDownScroll, NULL},
    {KEY_RIGHT | KEY_LEFT, true, true, &ButtonConfigLeftRight, NULL},
    {KEY_Y, true, true, &ButtonConfigSaveAndExit, NULL},
    {HIDBINDNULL}};

int main(int argc, char* argv[]) {
    if (Player_Init(&g_player))
        return 0;

    //uint64_t timer_cnt = 0;
    // Main loop

    main_loop_data data = {0, 0, 0, 0b00000000, false, false};
    config_loop_data confdata = {&data, 0};

    for (int i = 0; i < sizeof(infoscreen)/sizeof(infoscreen[0])-1; i++)
        infoscreen[i].arg = (void*)&data;

    for (int i = 0; i < sizeof(configscreen)/sizeof(configscreen[0])-1; i++)
        configscreen[i].arg = (void*)&data;

    infoscreen[6].arg = (void*)&confdata;
    configscreen[0].arg = (void*)&confdata;

    if (R_FAILED(HIDMapper_SetMapping(infoscreen, false))) {
        printf("HID Fail.\n");
        g_player.terminate_flag = true;
    }

    while (aptMainLoop() && !g_player.terminate_flag) {
        gspWaitForVBlank();
        gfxSwapBuffers();

        if (R_FAILED(HIDMapper_RunFrame())) break;
    }

    HIDMapper_ClearMapping();
    Player_Exit(&g_player);
    return 0;
}
