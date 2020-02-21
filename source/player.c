#include "player.h"
#include <3ds.h>
#include <stdarg.h>
#include <unistd.h>
#include <wchar.h>
#include "fshelper.h"
#include "sndthr.h"

static inline int get_debug_testing_model() {
    int model = -1;
#ifdef DEBUG
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
#endif
    return model;
}

const char* default_search_path[] = {"romfs:/", "sdmc:/mod", "sdmc:/mods"};  //Check Path

void Player_AptHook(APT_HookType hook, void* param) {
    Player* player = (Player*)param;
    static uint32_t save;
    switch (hook) {
        case APTHOOK_ONSUSPEND:
        case APTHOOK_ONSLEEP:
            save = player->play_sound;
            player->play_sound = 0;
            break;

        case APTHOOK_ONRESTORE:
        case APTHOOK_ONWAKEUP:
            if (save == 1) player->play_sound = 1;
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

int Player_InitThread(Player* player, int model) {
    int32_t main_prio;
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
    if (R_FAILED(PlayerConfig_EnsuredLoad(&player->playerConfig))) return 2;
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

    uint32_t song_num = 0;

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

    player->audio_stream = linearAlloc(player->block_size * sizeof(int16_t) * 2);
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
    FSHelp_Cleanup();
    fsExit();
    aptExit();
    ndspExit();
    romfsExit();
    gfxExit();
}

enum configurable_types {
    T_CUSTOM = 0, // non specific type, implement proper functions
    T_BOOL, // bool
    T_UINT, // unsigned int
    T_SINT, // signed int
    T_UINT8, // uint8_t
    T_UINT16, // uint16_t
    T_UINT32, // uint32_t
    T_UINT64, // uint64_t
    T_SINT8, // int8_t
    T_SINT16, // int16_t
    T_SINT32, // int32_t
    T_SINT64, // int64_t
};

typedef struct configurable_variables {
    const char* printname;
    const char* description;
    enum configurable_types type;
    int data_off;
    int (*stringconvert)(const struct configurable_variables*, char*, int, const void*);
    void (*dataupdate)(const struct configurable_variables*, void*, int);
} configurable_variables;

static int default_basictype_to_buffer(const configurable_variables* var, char* buf, int size, const void* data) {
    enum configurable_types type = var->type;
    switch(type) {
        case T_BOOL:
            if (buf) strncpy(buf, *(const bool*)data ? "ON" : "OFF", size);
            return strlen(*(const bool*)data ? "ON" : "OFF");
        case T_UINT:
            return snprintf(buf, size, "%u", *(const unsigned int*)data);
        case T_SINT:
            return snprintf(buf, size, "%d", *(const int*)data);
        case T_UINT8:
            return snprintf(buf, size, "%u", *(const uint8_t*)data);
        case T_SINT8:
            return snprintf(buf, size, "%d", *(const int8_t*)data);
        case T_UINT16:
            return snprintf(buf, size, "%u", *(const uint16_t*)data);
        case T_SINT16:
            return snprintf(buf, size, "%d", *(const int16_t*)data);
        case T_UINT32:
            return snprintf(buf, size, "%lu", *(const uint32_t*)data);
        case T_SINT32:
            return snprintf(buf, size, "%ld", *(const int32_t*)data);
        case T_UINT64:
            return snprintf(buf, size, "%llu", *(const uint64_t*)data);
        case T_SINT64:
            return snprintf(buf, size, "%lld", *(const int64_t*)data);
        default:
            return -1;
    }
}

static void default_basictype_update(const configurable_variables* var, void* data, int inc) {
    enum configurable_types type = var->type;
    switch(type) {
        case T_BOOL:
            *(bool*)data ^= inc & 0x1;
            break;
        case T_UINT:
            *(unsigned int*)data += inc;
            break;
        case T_SINT:
            *(signed int*)data += inc;
            break;
        case T_UINT8:
            *(uint8_t*)data += inc;
            break;
        case T_SINT8:
            *(int8_t*)data += inc;
            break;
        case T_UINT16:
            *(uint16_t*)data += inc;
            break;
        case T_SINT16:
            *(int16_t*)data += inc;
            break;
        case T_UINT32:
            *(uint32_t*)data += inc;
            break;
        case T_SINT32:
            *(int32_t*)data += inc;
            break;
        case T_UINT64:
            *(uint64_t*)data += inc;
            break;
        case T_SINT64:
            *(int64_t*)data += inc;
            break;
        default:
            break;
    }        
}

static const configurable_variables cvars[] = {
    {"Loop Count", NULL, T_SINT, offsetof(PlayerConfig, loopcheck), &default_basictype_to_buffer, &default_basictype_update},
    {"DEBUG", NULL, T_BOOL, offsetof(PlayerConfig, debugmode), &default_basictype_to_buffer, &default_basictype_update},
    {"Loader Mode", NULL, T_BOOL, offsetof(PlayerConfig, loadMode), &default_basictype_to_buffer, &default_basictype_update},
};

static const int configurable_count = sizeof(cvars) / sizeof(cvars[0]);

static int _nullsnprintf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    int ret = vsnprintf(NULL, 0, format, va);
    va_end(va);
    return ret;
}

// return n of lines
static int cvar_to_line(const configurable_variables* var, const void* data, bool print, bool highlight) {
    int (*printcall)(const char*, ...) = print ? &printf : &_nullsnprintf;
    int initial_print = printcall("%s:",  var->printname);
    int count_n_start = initial_print < 25 ? 0 : ((initial_print + 49) / 50);
    if (count_n_start) printcall("\n");
    else printcall("%*s", 25 - initial_print, ""); // padding
    if (highlight) printcall("\e[36m");
    int bufsize = var->stringconvert(var, NULL, 0, data);
    if (bufsize < 0) return -1;
    char* buf = (char*)malloc(bufsize + 1);
    if (!buf) return -1;
    if (bufsize+2 > 25) printcall("\n");
    var->stringconvert(var, buf, bufsize+1, data);
    int n = printcall(highlight ? "<%s>" : " %s ", buf);
    if (highlight) printcall("\e[0m");
    printcall("\n");
    free(buf);
    return count_n_start + ((n + 49) / 50);
}

static void PrintConfigError(Player* player) {
    Player_ClearConsoles(player);
    Player_SelectTop(player);
    printf("CONFIG PRINT ERROR");
    Player_SelectBottom(player);
    printf("This shouldn't have happened.\n"
           "Press Y to exit config screen.");
}

void Player_ConfigsScreen(Player* player, int* listscroll, int* position, int update) {
    Player_ClearConsoles(player);
    Player_SelectTop(player);

    printf("=Config Screen v: %d=\n", player->playerConfig.version);
    //printf("Config version: %d\n", player->playerConfig.version);

    printf("\n");
    if (*position < 0) *position = 0;
    else if (*position >= configurable_count) *position = configurable_count - 1;
    if (*listscroll < 0) *listscroll = 0;
    else if (*listscroll >= configurable_count) *listscroll = configurable_count - 1;
    if (*listscroll >= *position) *listscroll = *position;

    do {
        int posn = cvar_to_line(&cvars[*position], ((uint8_t*)&player->playerConfig) + cvars[*position].data_off, false, false);

        if (posn < 0) {
            PrintConfigError(player);
            return;
        }

        if (posn >= 24) {
            *listscroll = *position;
            break;
        }

        for (int freen = 24-posn, pos = *position - 1; pos > 0; --pos) {
            const configurable_variables* cvar = &cvars[pos];
            int n = cvar_to_line(cvar, ((uint8_t*)&player->playerConfig) + cvar->data_off, false, false);
            if (n < 0) {
                PrintConfigError(player);
                return;
            }
            freen -= n;
            if (freen < 0) {
                if (*listscroll < pos+1) *listscroll = pos+1;
                break;
            }
        }
    } while(0);

    if (update)
        cvars[*position].dataupdate(&cvars[*position], ((uint8_t*)&player->playerConfig) + cvars[*position].data_off, update);

    printf("%28s\n", *listscroll ? "^^^^^^" : "");

    for (int freen = 24, pos = *listscroll; pos < configurable_count; ++pos) {
        const configurable_variables* cvar = &cvars[pos];
        int n = cvar_to_line(cvar, ((uint8_t*)&player->playerConfig) + cvar->data_off, false, false);
        if (n < 0) {
            PrintConfigError(player);
            return;
        }
        freen -= n;
        if (freen < 0 && pos != *position) {
            if (pos < configurable_count) printf("%28s", "vvvvvv");
            break;
        }
        n = cvar_to_line(cvar, ((uint8_t*)&player->playerConfig) + cvar->data_off, true, pos == *position);
        if (n < 0) {
            PrintConfigError(player);
            return;
        }
        // same check as before, but without position check
        if (freen < 0) {
            if (pos < configurable_count) printf("%28s", "vvvvvv");
            break;
        }
    }

    if (cvars[*position].description) {
        Player_SelectBottom(player);
        puts(cvars[*position].description);
    }
}
