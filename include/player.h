#pragma once
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmp.h>
#include "error.h"
#include "fastmode.h"
#include "linkedlist.h"
#include "sndthr.h"
#include "song_info.h"
#include "songhandler.h"
#include "songview.h"

void Player_AptHook(APT_HookType hook, void* param);

typedef struct PlayerConfiguration {
    uint8_t version;
    uint8_t debugmode;  // 1= Enable Debug
    int loopcheck;
    int loadMode;  // LoadMode; using new method or xmp mode
} PlayerConfiguration;

typedef struct {
    PrintConsole top, bot;
    struct xmp_module_info minfo;
    //struct xmp_frame_info finfos[8];
    struct xmp_frame_info finfo;
    PlayerConfiguration playerConfig;
    xmp_context ctx;
    bool context_released;
    LinkedList ll;
    LLNode* current_song;
    uint8_t subsong;
    int current_isFT;
    aptHookCookie apthook;
    Thread sound_thread;
    //ndspWaveBuf waveBuf[8];
    ndspWaveBuf waveBuf[2];
    //volatile uint32_t cur_wvbuf;
    int16_t* audio_stream;
    uint32_t block_size;
    LightEvent playwaiting_event;
    LightEvent playready_event;
    LightEvent resume_event;
    LightEvent pause_event;
    //LightEvent ndspcallback_event;
    volatile uint64_t render_time;
    volatile uint64_t screen_time;
    volatile uint8_t run_sound;
    volatile uint8_t play_sound;
    volatile uint8_t terminate_flag;
} Player;

void Player_InitConsoles(Player* player);
int Player_InitServices();
int Player_InitThread(Player* player, int model);
int Player_Init(Player* player);
void Player_Exit(Player* player);
int Player_WriteConfig(PlayerConfiguration pc);
void Player_ConfigsScreen(Player*, int*);
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
    // already stopped
    if (!player->run_sound) return;
    // otherwise, pause if not yet
    // doing this in an already paused state will freeze, so check that
    if (player->play_sound) {
        player->play_sound = 0;
        LightEvent_Wait(&player->pause_event);
    }
    player->run_sound = 0;
    LightEvent_Signal(&player->resume_event);
    LightEvent_Wait(&player->pause_event);
    LightEvent_Wait(&player->playwaiting_event);
}

static inline int Player_NextSong(Player* player) {
    Player_StopSong(player);
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
    Player_StopSong(player);
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

static inline void Player_TogglePause(Player* player) {
    if (!player->play_sound) {
        LightEvent_Signal(&player->resume_event);
        LightEvent_Wait(&player->pause_event);
    } else {
        player->play_sound ^= 1;
        LightEvent_Wait(&player->pause_event);
    }
}

static inline void Player_PrintGeneric(Player* player) {
    show_generic_info(&player->finfo, &player->minfo, &player->top, &player->bot, player->subsong);
}

static inline void Player_PrintInstruments(Player* player, int* scroll, int subscroll) {
    show_instrument_info(&player->minfo, &player->top, &player->bot, scroll, subscroll);
}

static inline void Player_PrintChannelInstruments(Player* player, int* subscroll) {
    show_channel_instrument_info(&player->finfo, &player->minfo, &player->top, &player->bot, subscroll);
}

static inline void Player_PrintSamples(Player* player, int* scroll) {
    show_sample_info(&player->minfo, &player->top, &player->bot, scroll);
}

static inline void Player_PrintPlaylist(Player* player, int* scroll, int* subscroll) {
    show_playlist(&player->ll, player->current_song, &player->top, &player->bot, scroll, subscroll);
}

static inline void Player_PrintChannel(Player* player, int* scroll, int* subscroll) {
    show_channel_info(&player->finfo, &player->minfo, &player->top, &player->bot, scroll, player->current_isFT, *subscroll);  // Fall back
    show_channel_info_btm(&player->finfo, &player->minfo, &player->top, &player->bot, subscroll, player->current_isFT);
}
