#include "song_info.h"
#include <3ds.h>
#include <stdio.h>
#include <xmp.h>
#include <stdlib.h>
#define gotoxy(x,y) printf("\033[%d;%dH",(x),(y))

static char *note_name[] = {
	"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
};
static char buf[16];
static char fx_buf[32];
static uint8_t old_note[256]; 
static uint8_t old_fxt[256];
static uint8_t old_fxp[256];

void show_generic_info(struct xmp_frame_info fi,struct xmp_module_info mi,PrintConsole top,PrintConsole bot) {
        consoleSelect(&bot);
		gotoxy(0,0);
		printf("%02d %02x %02x %1x %3d %3d/%3d\n",fi.pos,fi.pattern,fi.row,fi.speed,fi.bpm,
				fi.virt_used,fi.virt_channels);
		printf("%s\n%s\n",mi.mod->name,mi.mod->type);
}

void parse_fx(int ch,char *buf,uint8_t fxt,uint8_t fxp,bool isFT) {
	char _arg1[8];
	uint8_t _fxp = fxp;
	if((fxt == 1 || fxt == 2 || fxt == 3 || fxt == 4) && fxp != 0) {
		old_fxp[ch] = fxp;
	} else if ((fxt == 1 ||fxt == 2 || fxt == 3 || fxt == 4) && fxp == 0) {
		_fxp = old_fxp[ch];
	} else {old_fxp[ch] = 0;}

	snprintf(_arg1,6,"-----");
	switch(fxt) {
		case 0:
			if(!_fxp) {
				break;
			}
			snprintf(_arg1,6,"ARPeG");
			break;
		case 1:
			snprintf(_arg1,6,"PORtU");
			break;
		case 2:
			snprintf(_arg1,6,"PORtD");
			break;
		case 3:
			snprintf(_arg1,6,"TPOrT");
			break;
		case 4:
			snprintf(_arg1,6,"VIBrT");
			break;
		case 9:
			snprintf(_arg1,6,"OFStS");
			break;
		case 0xa:
			if((_fxp&0x0F)==0 && (_fxp>>4&0xF) > 0) // Up
				snprintf(_arg1,6,"VOLsU");
			else if(((_fxp>>4)&0xF)==0 && (_fxp&0x0F)>0) // Down
				snprintf(_arg1,6,"VOLsD");
			break;
		case 0xc:
			snprintf(_arg1,6,"VOLsT");
		default:
			snprintf(_arg1,6,"-----");
	}
	snprintf(buf,20,"%-5.5s%02X%02X",_arg1,fxt,_fxp);
	//free(_arg1);
}


void show_channel_info(struct xmp_frame_info fi,struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int *f) {
    	//int line_lim = isN3DS?29:16;
		consoleSelect(&top);
		gotoxy(0,0);
		struct xmp_module *xm = mi.mod;
		int toscroll = *f;
		int cinf = xm->chn;
		int chmax = 0;
		if(cinf <= 29) {
			toscroll=0; chmax=cinf;
			// pat a (there's no exceeding limit. ignore f solely.)
		} else if (toscroll < cinf-29) {
			chmax=29+toscroll;
			//pat b (there's scroll need. but f is under xm->ins-29)
		} else if (toscroll >= cinf-29) {
			toscroll = cinf-29;
			chmax = cinf;
			*f = toscroll;
			//pat c (toscroll reaches (xm->ins-29))
		}

		for (int i=toscroll;i<chmax;i++) {
			// Does it o/f?
			/*if(i>=line_lim) { 
				consoleSelect(&bot);
				gotoxy(i+4-line_lim,0);
			}*/
            //Parse note
			struct xmp_channel_info ci = fi.channel_info[i];
            struct xmp_event ev = ci.event;
            if (ev.note > 0x80) snprintf(buf,15,"===");
            else if(ev.note > 0) {
                snprintf(buf,15,"\e[34;1m%s%d\e[0m",note_name[ev.note%12],ev.note/12);
                old_note[i] = ev.note;
            }
            else if(ev.note == 0 && ci.volume!=0) {
                snprintf(buf,15,"\e[36;1m%s%d\e[0m",note_name[old_note[i]%12],old_note[i]/12);
            }
            else {snprintf(buf,15,"---"); old_note[i] = 0;}
			parse_fx(i,fx_buf,ev.fxt,ev.fxp,0);
			printf("%2d:%c%02x %s %02x %02d%02d %5x %s %02x:%02x\n"
					,i+1,ev.note!=0?'!':ci.volume==0?' ':'G'
					,ci.instrument,buf,ci.pan
					,ci.volume,ev.vol,ci.position
					,fx_buf
					,ev.f2t,ev.f2p
					);
		}
}

void show_instrument_info(struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int *f) {
	    //int line_lim = isN3DS?29:16;
		consoleSelect(&top);
		gotoxy(0,0);
		int toscroll = *f;
		struct xmp_module *xm = mi.mod;
		//char *ind = (char*)' ';
		bool ind_en=false;
		bool isind = false;
		int insmax = 0;
		if(xm->ins <= 29) {
			toscroll=0; insmax=xm->ins;
			// pat a (there's no exceeding limit. ignore f solely.)
		} else if (toscroll < xm->ins-29) {
			insmax=29+toscroll;
			isind = true;
			//pat b (there's scroll need. but f is under xm->ins-29)
		} else if (toscroll >= xm->ins-29) {
			toscroll = xm->ins-29;
			insmax = xm->ins;
			*f = toscroll;
			//pat c (toscroll reaches (xm->ins-29))
		}
		struct xmp_instrument *xi;
		for (int i=toscroll;i<insmax;i++) {
			xi = &xm->xxi[i];
            if(isind && toscroll == insmax-1) ind_en = true;
			printf("%2x:\"%-32.32s\" %02x%04x %c%c%c%c\n",
						i,xi->name,xi->vol,xi->rls,
						xi->aei.flg&1?'A':'-',
						xi->pei.flg&1?'P':'-',
						xi->fei.flg&1?'F':'-',ind_en?'v':' ');
		}

}

void show_sample_info(struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int *f) {
	    int toscroll = *f;
		consoleSelect(&top);
		gotoxy(0,0);
		struct xmp_module *xm = mi.mod;
		int smpmax = 0;
		if(xm->smp <= 29) {
			toscroll=0; smpmax=xm->smp;
			// pat a (there's no exceeding limit. ignore f solely.)
		} else if (toscroll < xm->smp-29) {
			smpmax=29+toscroll;
			//pat b (there's scroll need. but f is under xm->ins-29)
		} else if (toscroll >= xm->smp-29) {
			toscroll = xm->smp-29;
			smpmax = xm->smp;
			*f = toscroll;
			//pat c (toscroll reaches (xm->ins-29))
		}
		for (int i=toscroll;i<smpmax;i++) {
			struct xmp_sample *xs = &xm->xxs[i];
            // ignore sample that doesn't have size
			//if(xs->len==0) continue;
			
			printf("%2d:\"%-16.16s\" %5x%5x%5x %c%c%c%c%c%c\n",
						i,xs->name,xs->len,xs->lps,xs->lpe,
						xs->flg & XMP_SAMPLE_16BIT ? 'W' : '-',
						xs->flg & XMP_SAMPLE_LOOP ? 'L' : '-',
						xs->flg & XMP_SAMPLE_LOOP_BIDIR ? 'B' : '-',
						xs->flg & XMP_SAMPLE_LOOP_REVERSE ? 'R' : '-',
						xs->flg & XMP_SAMPLE_LOOP_FULL ? 'F' : '-',
						xs->flg & XMP_SAMPLE_SYNTH ? 'S' : '-');
		}

}