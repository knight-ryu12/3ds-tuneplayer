#include "sndthr.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <xmp.h>

#define N3DS_BLOCK 3072
#define O3DS_BLOCK 4096
// can someone find sweet spot?

extern int runSound, playSound;
extern struct xmp_frame_info fi;
extern uint64_t render_time;

extern volatile uint32_t _PAUSE_FLAG;

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

void soundThread(void *arg) {
    struct thread_data *td = (struct thread_data *)arg;
    int BLOCK = td->model ? N3DS_BLOCK : O3DS_BLOCK;
    xmp_context c = td->c;
    ndspWaveBuf waveBuf[2];
    int16_t *sample;
    int cur_wvbuf = 0;

    sample = linearAlloc(BLOCK * sizeof(int16_t) * 2);
    memset(waveBuf, 0, sizeof(waveBuf));
    waveBuf[0].data_vaddr = sample;
    waveBuf[1].data_vaddr = sample + BLOCK;

    waveBuf[0].status = waveBuf[1].status = NDSP_WBUF_DONE;

    if (xmp_start_player(c, SAMPLE_RATE, 0) != 0) {
        printf("Error on xmp_start\n");
        goto exit;
    }

    _PAUSE_FLAG = 0;
    uint64_t first = 0;
    //uint64_t second = 0;
    while (runSound) {
        first = svcGetSystemTick();
        if (!playSound) {
            _PAUSE_FLAG = 1;
            render_time = 0;
            while (runSound && !playSound) {
                svcSleepThread(20000);
            };
            // is sleep quit by runSound being off?
            if (!runSound) break;
            _PAUSE_FLAG = 0;
            continue;
        }

        xmp_get_frame_info(c, &fi);

        int16_t *sbuf = (int16_t *)waveBuf[cur_wvbuf].data_vaddr;
        xmp_play_buffer(c, sbuf, BLOCK, 0);
        waveBuf[cur_wvbuf].nsamples = BLOCK / 4;

        DSP_FlushDataCache(sbuf, BLOCK);
        ndspChnWaveBufAdd(CHANNEL, &waveBuf[cur_wvbuf]);
        cur_wvbuf ^= 1;
        render_time = svcGetSystemTick() - first;
        while (waveBuf[cur_wvbuf].status != NDSP_WBUF_DONE && runSound)
            //svcSleepThread(10e9 / (BLOCK / 2));
            svcSleepThread(10000000 / (BLOCK / 2));
    }
exit:
    ndspChnWaveBufClear(CHANNEL);
    ndspExit();
    linearFree(sample);
    runSound = 0;
    threadExit(0);
}