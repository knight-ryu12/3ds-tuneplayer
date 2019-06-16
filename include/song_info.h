#pragma once
#include <3ds.h>
#include <stdio.h>
#include <xmp.h>

void show_generic_info(struct xmp_frame_info fi,struct xmp_module_info mi,PrintConsole top,PrintConsole bot);
void show_channel_info(struct xmp_frame_info fi,struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int *f);
void show_instrument_info(struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int *f);
void show_sample_info(struct xmp_module_info mi,PrintConsole top,PrintConsole bot,int isN3DS);
