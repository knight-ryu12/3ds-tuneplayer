#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <xmp.h>
#include "sndthr.h"
#include "player.h"

#define add_masked(x,y) (((x) + (y)) & 0x7)

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
    if (player->run_sound && player->waveBuf[player->cur_wvbuf].status == NDSP_WBUF_DONE) {
        LightEvent_Signal(&player->ndspcallback_event);
    }
}

void soundThread(void *arg) {
    Player* player = (Player*)arg;

    ndspWaveBuf *wave;

    u64 first = 0;
    //uint64_t second = 0;
    while (!player->terminate_flag) {
        LightEvent_Wait(&player->playready_event);
        if (player->terminate_flag) break;
        for (int i = 0; i < 8; i++) {
            xmp_get_frame_info(player->ctx, &player->finfos[add_masked(player->cur_wvbuf, i)]);

            wave = &player->waveBuf[add_masked(player->cur_wvbuf, i)];
            xmp_play_buffer(player->ctx, wave->data_pcm16, player->block_size, 0);
            wave->nsamples = player->block_size / 4;
        }

        ndspSetCallback(&_callback, (void*)player);
        DSP_FlushDataCache(player->waveBuf[player->cur_wvbuf].data_pcm16, player->block_size);
        ndspChnWaveBufAdd(CHANNEL, &player->waveBuf[player->cur_wvbuf]);

        while (player->run_sound) {
            LightEvent_Wait(&player->ndspcallback_event);

            first = svcGetSystemTick();
            if (!player->play_sound) {
                player->render_time = 0;
                LightEvent_Signal(&player->pause_event);
                LightEvent_Wait(&player->resume_event);
                // is sleep quit by runSound being off?
                player->play_sound = 1;
                LightEvent_Signal(&player->pause_event);
                if (!player->run_sound || player->terminate_flag) break;
                _callback((void*)player);
                continue;
            }

            u32 cur = player->cur_wvbuf;

            wave = &player->waveBuf[player->cur_wvbuf];

            DSP_FlushDataCache(wave->data_pcm16, player->block_size);
            ndspChnWaveBufAdd(CHANNEL, wave);

            player->cur_wvbuf = add_masked(player->cur_wvbuf, 1);

            xmp_get_frame_info(player->ctx, &player->finfos[cur]);

            wave = &player->waveBuf[cur];
            xmp_play_buffer(player->ctx, wave->data_pcm16, player->block_size, 0);
            wave->nsamples = player->block_size / 4; // Because of interleve

            

            player->render_time = svcGetSystemTick() - first;
        }
        ndspSetCallback(NULL, NULL);
        LightEvent_Clear(&player->ndspcallback_event);
    }
    ndspChnWaveBufClear(CHANNEL);
    threadExit(0);
}
