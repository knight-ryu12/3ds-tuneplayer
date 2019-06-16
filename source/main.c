#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <xmp.h>
#include "fastmode.h"
#include "sndthr.h"
#include "song_info.h"

#define gotoxy(x,y) printf("\033[%d;%dH",(x),(y))

#define ENABLE_ROMFS

char *romfs_path = "romfs:/";
char *track[15] = {"romfs:/reed-imploder.xm","romfs:/amgorb-randomizer666.it",
		"romfs:/last step.xm","romfs:/mindrmr.xm",
		"romfs:/seffren&jelly-o_m_g.xm","romfs:/resonance2.mod",
		"romfs:/bz_fil.it","romfs:/ASTRAY.S3M",
		"romfs:/seen_an_angel.xm","romfs:/sv_ttt.xm",
		"romfs:/milky.xm","romfs:/ICEFRONT.S3M",
		"romfs:/ghost_in_the_cli.mod","romfs:/universalnetwork2_real.xm",
		"romfs:/JEFF93.IT"};
int track_size = 15;
int track_quirk[1] = {XMP_MODE_ST3};

volatile int runSound, playSound;
struct xmp_frame_info fi;
volatile int runRead, doRead;
volatile uint64_t d_t;
uint32_t REAL_CLK = SYSCLOCK_ARM11;
extern volatile uint32_t _PAUSE_FLAG;

char strbuf[255] = "romfs:/";

void _debug_pause() {
	printf("Press start.\n");
	while(1) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if(kDown & KEY_START) break;
	}
}

void clean_console(PrintConsole *top,PrintConsole *bot) {
	consoleSelect(top);
	consoleClear();
	consoleSelect(bot);
	consoleClear();
}

int main(int argc, char* argv[])
{
	Result res;
	PrintConsole top,bot;
	//uint64_t cur_tick,init_tick;
	int32_t main_prio;
	uint8_t info_flag = 0b00000000;
	int32_t i = 0;
	int model;
	Thread rd_thr,snd_thr;
	struct xmp_module_info mi;
	//cur_tick = svcGetSystemTick();
	gfxInitDefault();
	consoleInit(GFX_TOP, &top);
	consoleInit(GFX_BOTTOM, &bot);
	consoleSelect(&top);


	model = try_speedup(); 
	res = romfsInit(); printf("romFSInit %08lx\n",res);
	res = setup_ndsp(); printf("setup_ndsp: %08lx\n",res);
	xmp_context c = xmp_create_context();
	
	printf("Registered Songs: %d\n",track_size);
	for(int i=0;i<track_size;i++) {
		printf("%d: %s\n",i,track[i]);
	}

	hidScanInput();
	uint32_t h = hidKeysHeld();
	if(h & KEY_R) {model = 0; printf("Model force 0.\n");}

	printf("Loading....\n");
	if(initXMP(track[i],c,&mi) != 0) {
			printf("Error on initXMP!!\n");
			goto exit;
	}
	clean_console(&top,&bot);

	xmp_get_frame_info(c,&fi);
	consoleSelect(&bot);
	gotoxy(2,0); printf("%s\n%s\n",mi.mod->name,mi.mod->type);
	//s = APT_SetAppCpuTimeLimit(30);
	//toxy(0,0); printf("SetAppCpuTimeLimit(): %08lx\n",res);
	runSound = playSound = 1;
	runRead = doRead=1;
	svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);	
	struct thread_data a = {c,model};
	snd_thr = threadCreate(soundThread, &a,32768,main_prio+1, model==0?0:2, true);
	//rd_thr = threadCreate(readThread,c,32768,main_prio+1,2,true);
	consoleSelect(&top);
	//int chmax = mi.mod->chn<=32?mi.mod->chn:32;
	
	int scroll = 0;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();

		show_generic_info(fi,mi,top,bot);
		/// 000 shows default info.
		if(info_flag & 1) show_instrument_info(mi,top,bot,&scroll);
		else if(info_flag & 2) show_sample_info(mi,top,bot,model);
		//else if(info_flag & 4) show_sample_info(mi,top,bot,model);
		else show_channel_info(fi,mi,top,bot,&scroll); //Fall back
		hidScanInput();
		// Your code goes here
		u32 kDown = hidKeysDown();


		if (kDown & KEY_A) {
			clean_console(&top,&bot);
			info_flag = 0b000;
		}

		if (kDown & KEY_X) {
			clean_console(&top,&bot);
			info_flag = 0b001;
		}
		if (kDown & KEY_Y) {
			clean_console(&top,&bot);
			info_flag = 0b010;
		}


		if (kDown & KEY_SELECT) playSound ^= 1; 
		
		if(kDown & KEY_DOWN) {
			struct xmp_module *xm = mi.mod;
				//  does it have atleast 30 inst to show?
				if(xm->ins > 30) {
					scroll++;
				} else if(xm->chn > 30) {
					scroll++;
			}
		}

		if(kDown & KEY_UP) {
			if(scroll != 0 && scroll >= 0) scroll--;
		}

		if (kDown & KEY_RIGHT) {
			i++;
			if(i > track_size-1) {i=track_size-1; continue;}
			doRead=0;
			playSound = 0;
			xmp_stop_module(c);
			//while(!_PAUSE_FLAG);
			clean_console(&top,&bot);
			gotoxy(0,0);
			printf("Loading....\n");
			xmp_end_player(c);
			xmp_release_module(c);
			if(initXMP(track[i],c,&mi) != 0) {
				printf("Error on initXMP!!\n");
				goto exit;
			}
			//_debug_pause();
			xmp_start_player(c,32768,0);
			clean_console(&top,&bot);
			scroll = 0;
			playSound=1;
			doRead=1;
		}

		if (kDown & KEY_LEFT) {
			i--;
			if(i < 0){ i=0; continue; }
			doRead=0;
			playSound = 0;
			xmp_stop_module(c);
			//while(!_PAUSE_FLAG);
			clean_console(&top,&bot);
			gotoxy(0,0);
			printf("Loading....\n");
			xmp_end_player(c);
			xmp_release_module(c);
			if(initXMP(track[i],c,&mi) != 0) {
				printf("Error on initXMP!!\n");
				goto exit;
			}
			//_debug_pause();
			xmp_start_player(c,32768,0);
			clean_console(&top,&bot);
			scroll = 0;
			playSound=1;
			doRead=1;
		}

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}
exit:
	playSound = 0;
	xmp_stop_module(c);
	_debug_pause();
	runSound = 0;
	runRead = 0;
	//threadJoin(rd_thr,U64_MAX);
	threadJoin(snd_thr,U64_MAX);
	xmp_end_player(c);
	xmp_release_module(c);
	xmp_free_context(c);
	romfsExit();
	gfxExit();
	return 0;
}
