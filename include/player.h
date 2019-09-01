#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <xmp.h>
#include "fastmode.h"
#include "linkedlist.h"
#include "songhandler.h"
#include "error.h"
#include "sndthr.h"

void Player_AptHook(APT_HookType hook, void *param);

typedef struct {
    PrintConsole top, bot;
    struct xmp_module_info minfo;
    struct xmp_frame_info finfos[8];
    xmp_context ctx;
    bool context_released;
    LinkedList ll;
    LLNode *current_song;
    u8 subsong;
    int current_isFT;
    aptHookCookie apthook;
    Thread sound_thread;
    ndspWaveBuf waveBuf[8];
    volatile u32 cur_wvbuf;
    s16* audio_stream;
    u32 block_size;
    LightEvent playready_event;
    LightEvent resume_event;
    LightEvent pause_event;
    LightEvent ndspcallback_event;
    volatile u64 render_time;
    volatile u64 screen_time;
    volatile u8 run_sound;
    volatile u8 play_sound;
    volatile u8 terminate_flag;
} Player;

void Player_InitConsoles(Player* player);
int Player_InitServices();
int Player_InitThread(Player* player, int model);
int Player_Init(Player* player);
void Player_Exit(Player* player);

static inline void Player_SelectTop(Player* player) {
    consoleSelect(&player->top);
}

static inline void Player_SelectBottom(Player* player) {
    consoleSelect(&player->bot);
}

static inline void Player_ClearConsoles(Player* player) {
    Player_SelectTop(player);
    consoleClear();
    Player_SelectBottom(player);
    consoleClear();
}

static inline void Player_StopSong(Player* player) {
    player->play_sound = 0;
    LightEvent_Wait(&player->pause_event);
    player->run_sound = 0;
    LightEvent_Signal(&player->resume_event);
    LightEvent_Wait(&player->pause_event);
}

static inline int Player_NextSong(Player* player) {
    int ret = load_song(player->ctx, &player->minfo, &player->ll, &player->current_song, &player->current_isFT, &player->context_released, true);
    if (!ret) {
        player->play_sound = 1;
        player->run_sound = 1;
        player->subsong = 0;
        LightEvent_Signal(&player->playready_event);
    }
    return ret;
}

static inline int Player_PrevSong(Player* player) {
    int ret = load_song(player->ctx, &player->minfo, &player->ll, &player->current_song, &player->current_isFT, &player->context_released, false);
    if (!ret) {
        player->play_sound = 1;
        player->run_sound = 1;
        player->subsong = 0;
        LightEvent_Signal(&player->playready_event);
    }
    return ret;
}

static inline void Player_NextSubSong(Player* player) {
    // Does it even have a subsong to play?
    if (player->minfo.num_sequences < 2 || player->minfo.num_sequences <= player->subsong + 1) return;
    Player_StopSong(player);
    player->subsong++;
    //xmp_stop_module(player->ctx);
    xmp_set_position(player->ctx, player->minfo.seq_data[player->subsong].entry_point);
    player->play_sound = 1;
    player->run_sound = 1;
    LightEvent_Signal(&player->playready_event);
}

static inline void Player_PrevSubSong(Player* player) {
    // Does it even have a subsong to play?
    if (player->minfo.num_sequences < 2 || player->subsong == 0) return;
    Player_StopSong(player);
    player->subsong--;
    //xmp_stop_module(player->ctx);
    xmp_set_position(player->ctx, player->minfo.seq_data[player->subsong].entry_point);
    player->play_sound = 1;
    player->run_sound = 1;
    LightEvent_Signal(&player->playready_event);
}
