#pragma once
#include <3ds.h>
#include <stdio.h>
#include <xmp.h>

void show_generic_info(struct xmp_frame_info *, struct xmp_module_info *, PrintConsole *, PrintConsole *, int);
void show_title(struct xmp_module_info *, PrintConsole *);
void show_channel_info(struct xmp_frame_info *, struct xmp_module_info *, PrintConsole *, PrintConsole *, int *, int, int);
void show_instrument_info(struct xmp_module_info *, PrintConsole *, PrintConsole *, int *, int);
void show_sample_info(struct xmp_module_info *, PrintConsole *, PrintConsole *, int *);
void show_channel_instrument_info(struct xmp_frame_info *, struct xmp_module_info *, PrintConsole *, PrintConsole *, int *);
void show_channel_info_btm(struct xmp_frame_info *, struct xmp_module_info *, PrintConsole *, PrintConsole *, int *, int);