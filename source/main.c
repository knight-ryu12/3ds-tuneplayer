#include <3ds.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xmp.h>
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

void printhelp() {
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
    //printf("");
    printf("\n3ds-tuneplayer made by Chromaryu\n");
    printf("using libxmp v%s\n", xmp_version);
}

int main(int argc, char *argv[]) {
    uint8_t info_flag = 0b00000000;

    if (Player_Init(&g_player))
        return 0;

    int scroll = 0;
    int subscroll = 0;
    //uint64_t timer_cnt = 0;
    // Main loop
    uint64_t first = 0;
    bool isPrint = false;

    while (aptMainLoop()) {
        if (g_player.terminate_flag) {
            break;
        }  // Break to exit.
        gspWaitForVBlank();
        gfxSwapBuffers();
        first = svcGetSystemTick();
        // Check loop cnt
        if (g_player.playerConfig.loopcheck >= 0) {
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
                first = svcGetSystemTick();
                scroll = 0;
            }
        }

        Player_PrintGeneric(&g_player);
        switch (info_flag) {
            case 1:
                if (!isPrint) {
                    Player_PrintInstruments(&g_player, &scroll, subscroll);
                    isPrint = true;
                }
                Player_PrintChannelInstruments(&g_player, &subscroll);
                break;
            case 2:
                if (!isPrint) {
                    Player_PrintSamples(&g_player, &scroll);
                    isPrint = true;
                }
                break;
            case 3:
                if (!isPrint) {
                    Player_SelectTop(&g_player);
                    printhelp();
                    isPrint = true;
                }
                break;
            case 8:
                if (!isPrint) {
                    //Player_SelectTop(&g_player);
                    //consoleClear(); // This'll stop garbage
                    Player_PrintPlaylist(&g_player, &scroll, &subscroll);
                    isPrint = true;
                }
                break;
            case 16:
                if (!isPrint) {
                    Player_SelectTop(&g_player);
                    Player_ConfigsScreen(&g_player, &subscroll);
                    isPrint = true;
                }
                break;
            default:
                Player_PrintChannel(&g_player, &scroll, &subscroll);
        }

        hidScanInput();

        // Your code goes here
        uint32_t kDown = hidKeysDown();
        uint32_t kHeld = hidKeysHeld();

        if (kDown & KEY_R) {
            Player_ClearConsoles(&g_player);
            isPrint = false;
            info_flag = 0b1000;
        }

        if (kDown & KEY_A) {
            Player_ClearConsoles(&g_player);
            isPrint = false;
            if (++info_flag > 3) {
                // 012012012
                info_flag = 0;
            }
        }
        /*
        if (kDown & KEY_B) {
            Player_ClearConsoles(&g_player);
            isPrint = false;
            info_flag = 0b100;
        }
        if (kDown & KEY_X) {
            Player_ClearConsoles(&g_player);
            isPrint = false;
            info_flag = 0b001;
        }
        */
        if (kDown & KEY_Y) {
            Player_ClearConsoles(&g_player);
            isPrint = false;
            info_flag = 16;
        }

        if (kDown & KEY_SELECT) {
            Player_TogglePause(&g_player);
        }

        if (info_flag != 16) {
            if (kDown & KEY_DOWN) {  //|| (kHeld & KEY_DOWN && timer_cnt >= 50)) {
                isPrint = false;
                if (kHeld & KEY_L) {
                    subscroll++;
                } else {
                    scroll++;
                }
            }

            if (kDown & KEY_UP) {  //|| (kHeld & KEY_UP && timer_cnt >= 50)) {
                isPrint = false;
                if (kHeld & KEY_L)
                    if (subscroll != 0 && subscroll >= 0) subscroll--;
                if (scroll != 0 && scroll >= 0) scroll--;
            }

            if (kDown & KEY_RIGHT) {
                if (kHeld & KEY_L) {
                    Player_NextSubSong(&g_player);
                } else {
                    Player_ClearConsoles(&g_player);
                    gotoxy(0, 0);
                    if (Player_NextSong(&g_player) != 0) {
                        printf("Error on loadSong !!!?\n");
                        break;
                    };
                    //_debug_pause();
                    Player_ClearConsoles(&g_player);
                    g_player.render_time = g_player.screen_time = 0;
                    first = svcGetSystemTick();
                    scroll = 0;
                    isPrint = false;
                }
            }

            if (kDown & KEY_LEFT) {
                if (kHeld & KEY_L) {
                    Player_PrevSubSong(&g_player);
                } else {
                    Player_ClearConsoles(&g_player);
                    gotoxy(0, 0);
                    if (Player_PrevSong(&g_player) != 0) {
                        printf("Error on loadSong !!!?\n");
                        break;
                    };
                    Player_ClearConsoles(&g_player);
                    g_player.render_time = g_player.screen_time = 0;
                    first = svcGetSystemTick();
                    scroll = 0;
                    isPrint = false;
                }
            }
        }
        if (kDown & KEY_START) break;  // break in order to return to hbmenu
        g_player.screen_time = svcGetSystemTick() - first;
    }

    Player_Exit(&g_player);
    return 0;
}
