#include "player.h"
#include <3ds.h>
#include <unistd.h>
#include <wchar.h>
#include "sndthr.h"

typedef struct PlayerConfiguration {
    uint8_t version;
    int loopcheck;
} PlayerConfiguration;

typedef struct __attribute__((packed, aligned(4))) {
    u32 type;
    u64 extdataId;
} Extdata_Path;

static inline int get_debug_testing_model() {
    int model = -1;
#ifdef DEBUG
    hidScanInput();
    u32 h = hidKeysHeld();
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
#endif
    return model;
}

const char* default_search_path[] = {"romfs:/", "sdmc:/mod", "sdmc:/mods"};  //Check Path

void Player_AptHook(APT_HookType hook, void* param) {
    Player* player = (Player*)param;
    switch (hook) {
        case APTHOOK_ONSUSPEND:
        case APTHOOK_ONSLEEP:
            player->play_sound = 0;
            break;

        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            //Persist pause state
            //playSound = 1;
            break;

        case APTHOOK_ONEXIT:
            player->terminate_flag = 1;
            break;

        default:
            break;
    }
}

void Player_InitConsoles(Player* player) {
    gfxInitDefault();
    consoleInit(GFX_TOP, &player->top);
    consoleInit(GFX_BOTTOM, &player->bot);
    consoleSelect(&player->top);
}

int Player_InitServices() {
    Result res = romfsInit();
    printf("romFSInit %08lx\n", res);
    // TODO: check those test to see if it will work.
    if (R_FAILED(res)) {
        sendError("Failed to initalize romfs... please check filesystem and such!\n", 0xFFFF0000);
        printf("Failed to initalize romfs...\n");
        return 1;
    }
    res = setup_ndsp();
    printf("setup_ndsp: %08lx\n", res);
    if (R_FAILED(res)) {
        printf("Failed to initalize NDSP...\n");
        sendError("Failed to initalize NDSP... have you dumped your DSP rom?\n", 0xFFFF0001);
        return 1;
    }
    res = aptInit();
    printf("aptInit %08lx\n", res);
    if (R_FAILED(res)) {
        printf("Error at aptInit()????");
        sendError("Error at aptInit()!?!", 0xFFFF0003);
        return 1;
    }
    res = fsInit();
    printf("fsInit %08lx\n", res);
    if (R_FAILED(res)) {
        printf("Error at fsInit()????");
        sendError("Error at fsInit()!?!", 0xFFFF0004);
        return 1;
    }
    return 0;
}

int Player_CheckConfig() {
    // And parsing.
    /*
        basically
        you create/hardcode FS_Path you need for an archive
        have an FS_Archive variable, stored some way you dont lose it
        call FSUSER_OpenArchive with &variable of that FS_Archive
        check if success
        have a Handle variable
        call FSUSER_OpenFile with &variable of that Handle, and FS_Archive for the variable, giving this FS_Archive will access the opened archive
        check success
        do whatever operation you need with other FSFILE_ calls for the file
        FSFILE_Close() that Handle when you're done
        and if you're done with the archive itself, FSUSER_CloseArchive with the saved FS_Archive
    */
    PlayerConfiguration default_pc = {1, 0};
    uint64_t fsa = 0x4152435A00000001LLU;
    Result r;
    Extdata_Path extdata = {.type = MEDIATYPE_SD, .extdataId = fsa};
    FS_Path path = {
        PATH_BINARY,
        sizeof(extdata),
        &extdata};
    FS_Archive extarc;
    FS_ExtSaveDataInfo esdi = {.mediaType = MEDIATYPE_SD, .saveId = fsa};
    const char* configpath = "/config.bin";
    FS_Path filepath = {PATH_ASCII, strlen(configpath) + 1, configpath};
    Handle hndl;
retry:
    r = FSUSER_OpenArchive(&extarc, ARCHIVE_EXTDATA, path);
    printf("FSUSER_OA %08lx\n", r);
    if (r == 0xc8a04478) {
        //Not found, creating
        printf("Creating extdata...");
        r = FSUSER_CreateExtSaveData(esdi, 1, 2, 131072, 0, NULL);
        if (r == 0) {
            printf("Done [%08lx]\n", r);
            goto retry;
        } else {
            printf("Failed [%08lx]\n", r);
            _debug_pause();
            return 1;
        }
    } else {
    file_retry:
        //Parse
        r = FSUSER_OpenFile(&hndl, extarc, filepath, FS_OPEN_READ | FS_OPEN_WRITE, 0);
        printf("FSUSER_OF %08lx\n", r);
        if (r == 0xc8804470) {
            // File not found
            r = FSUSER_CreateFile(extarc, filepath, 0, 128);
            printf("FSUSER_CF %08lx\n", r);
            goto file_retry;
        } else {
            //File exists, check for version
            uint32_t readsz;
            PlayerConfiguration pc = {};
            r = FSFILE_Read(hndl, &readsz, 0, &pc, sizeof(PlayerConfiguration));
            printf("FSFILE_R %08lx, %ld\n", r, readsz);
            printf("Version %d\n", pc.version);
            if (pc.version < 1) {
                //write new config, while preserving config contents;
                printf("Outdated/New creation, updating...");
                default_pc.loopcheck = pc.loopcheck;
                uint32_t writesz;
                r = FSFILE_Write(hndl, &writesz, 0, &default_pc, sizeof(default_pc), 0);
                printf("Done.\n");
                printf("FSFILE_W %08lx, %ld\n", r, writesz);
            }
        }
    }
    _debug_pause();
    return 0;
}

int Player_InitThread(Player* player, int model) {
    s32 main_prio;
    Result res = svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
    if (R_FAILED(res)) return 1;
    player->sound_thread = threadCreate(soundThread, player, 32768, main_prio + 1,
                                        model ? 2 : 0, true);
    if (!player->sound_thread) return 1;
    LightEvent_Wait(&player->playwaiting_event);
    return 0;
}

int Player_Init(Player* player) {
    Player_InitConsoles(player);
    if (Player_InitServices()) return 1;
    Player_CheckConfig();
    aptHook(&player->apthook, Player_AptHook, (void*)player);

    LightEvent_Init(&player->playwaiting_event, RESET_ONESHOT);
    LightEvent_Init(&player->playready_event, RESET_ONESHOT);
    LightEvent_Init(&player->resume_event, RESET_ONESHOT);
    LightEvent_Init(&player->pause_event, RESET_ONESHOT);
    //LightEvent_Init(&player->ndspcallback_event, RESET_ONESHOT);
    player->render_time = 0LLU;
    player->screen_time = 0LLU;
    player->run_sound = 0;
    player->play_sound = 0;
    player->terminate_flag = 0;

    player->ll = create_list();

    u32 song_num = 0;

    song_num += searchsong(default_search_path[0], &player->ll);  //Atleast, leave this one alone.
    song_num += searchsong(default_search_path[1], &player->ll);
    song_num += searchsong(default_search_path[2], &player->ll);

    printf("Songs: %ld\n", song_num);

    if (!song_num) {
        printf("There's no song at any of folders I've searched!\n");
        sendError("There's no songs playable!\n", 0xC0000000);
        return 2;
    }

    player->current_song = NULL;
    player->subsong = 0;

    //memset(&player->finfos, 0, sizeof(player->finfos));

    player->ctx = xmp_create_context();
    if (!player->ctx) {
        free_list(&player->ll);
        return 3;
    }

    int model = try_speedup();
    int dmodel = get_debug_testing_model();
    dmodel = dmodel < 0 ? model : dmodel;

    if (model < 0) {
        free_list(&player->ll);
        xmp_free_context(player->ctx);
        player->ctx = NULL;
        return 4;
    }

    //player->block_size = MS_TO_PCM16_SIZE(SAMPLE_RATE, 2, dmodel ? 50 : 100) & ~0x3;
    player->block_size = dmodel ? N3DS_BLOCK : O3DS_BLOCK;

    player->audio_stream = linearAlloc(player->block_size * sizeof(s16) * 2);
    if (!player->audio_stream) {
        free_list(&player->ll);
        xmp_free_context(player->ctx);
        player->ctx = NULL;
        return 5;
    }

    memset(&player->waveBuf, 0, sizeof(player->waveBuf));

    for (int i = 0; i < 2; i++) {
        player->waveBuf[i].data_pcm16 = &player->audio_stream[player->block_size * i];
        player->waveBuf[i].status = NDSP_WBUF_DONE;
    }

    //player->cur_wvbuf = 0;

    if (Player_NextSong(player) != 0) {
        free_list(&player->ll);
        xmp_free_context(player->ctx);
        linearFree(player->audio_stream);
        player->ctx = NULL;
        player->audio_stream = NULL;
        return 6;
    };

    Player_ClearConsoles(player);
    //xmp_get_frame_info(player->ctx, &player->finfos[0]);
    xmp_get_frame_info(player->ctx, &player->finfo);
    Player_SelectBottom(player);

    if (Player_InitThread(player, model) != 0) {
        free_list(&player->ll);
        xmp_free_context(player->ctx);
        linearFree(player->audio_stream);
        player->ctx = NULL;
        player->audio_stream = NULL;
        return 7;
    };

    return 0;
}

void Player_Exit(Player* player) {
    player->terminate_flag = 1;
    player->run_sound = 0;
    // wake up thread if waiting currently for anything
    LightEvent_Signal(&player->playready_event);
    LightEvent_Signal(&player->resume_event);
    LightEvent_Signal(&player->pause_event);
    //LightEvent_Signal(&player->ndspcallback_event);
    threadJoin(player->sound_thread, U64_MAX);
    if (player->ctx) xmp_stop_module(player->ctx);
    //_debug_pause();
    aptUnhook(&player->apthook);
    if (player->ctx) {
        xmp_end_player(player->ctx);
        if (!player->context_released)
            xmp_release_module(player->ctx);
        xmp_free_context(player->ctx);
    }
    linearFree(player->audio_stream);
    free_list(&player->ll);
    aptExit();
    ndspExit();
    romfsExit();
    gfxExit();
}
