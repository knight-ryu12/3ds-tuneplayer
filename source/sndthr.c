#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <xmp.h>
#include "sndthr.h"
#include "player.h"

Result setup_ndsp() {
    Result res;
    res = ndspInit();
    if (R_FAILED(res)) return res;

    ndspChnReset(CHANNEL);
    ndspChnSetInterp(CHANNEL, NDSP_INTERP_NONE);
    ndspSetClippingMode(NDSP_CLIP_NORMAL);  //???
    ndspChnSetFormat(CHANNEL, NDSP_FORMAT_STEREO_PCM16);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetRate(CHANNEL, SAMPLE_RATE);
    return 0;
}

static void _callback(void* data) {
    Player* player = (Player*)data;
    if (player->run_sound && player->waveBuf[player->cur_wvbuf].status == NDSP_WBUF_DONE)
        LightEvent_Signal(&player->ndspcallback_event);
}

void soundThread(void *arg) {
    Player* player = (Player*)arg;

    u64 first = 0;
    //uint64_t second = 0;
    while (!player->terminate_flag) {
        for(int i = 0; i < 8; i++) {
            xmp_get_frame_info(player->ctx, &player->finfos[player->cur_wvbuf]);

            s16 *sbuf = player->waveBuf[player->cur_wvbuf].data_pcm16;
            xmp_play_buffer(player->ctx, sbuf, player->block_size, 0);
            player->waveBuf[player->cur_wvbuf].nsamples = player->block_size / 4;
        }
        ndspSetCallback(&_callback, (void*)player);
        while (player->run_sound) {
            first = svcGetSystemTick();
            if (!player->play_sound) {
                player->pause_flag = 1;
                player->render_time = 0;
                while (player->run_sound && !player->play_sound) {
                    LightEvent_Wait(&player->pausewakeup_event);
                };
                // is sleep quit by runSound being off?
                if (!player->run_sound) break;
                player->pause_flag = 0;
                continue;
            }

            xmp_get_frame_info(player->ctx, &player->finfos[player->cur_wvbuf]);

            s16 *sbuf = player->waveBuf[player->cur_wvbuf].data_pcm16;
            xmp_play_buffer(player->ctx, sbuf, player->block_size, 0);
            player->waveBuf[player->cur_wvbuf].nsamples = player->block_size / 4; // Because of interleve

            DSP_FlushDataCache(sbuf, player->block_size);
            ndspChnWaveBufAdd(CHANNEL, &player->waveBuf[player->cur_wvbuf]);
            player->cur_wvbuf += 1;
            player->cur_wvbuf &= 0x7;
            player->render_time = svcGetSystemTick() - first;
            
            LightEvent_Wait(&player->ndspcallback_event);
        }
        ndspSetCallback(NULL, NULL);
    }
    ndspChnWaveBufClear(CHANNEL);
    player->run_sound = 0;
    threadExit(0);
}
