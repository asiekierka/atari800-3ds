/*
 * 3ds/video.c - Nintendo 3DS video backend
 *
 * Copyright (c) 2001-2002 Jacek Poplawski
 * Copyright (C) 2001-2016 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <3ds.h>

#include "artifact.h"
#include "atari.h"
#include "colours.h"
#include "config.h"
#include "input.h"
#include "input_3ds.h"
#include "log.h"
#ifdef PAL_BLENDING
#include "pal_blending.h"
#endif /* PAL_BLENDING */
#include "platform.h"
#include "screen.h"
#include "util.h"
#include "videomode.h"

#include "grapefruit.h"
#include "shader_shbin.h"

static C3D_Tex tex, kbd_display;
static C3D_Mtx proj_top, proj_bottom;
static C3D_RenderTarget *target_top, *target_bottom;
static struct ctr_shader_data shader;

static u32 *texBuf;
VIDEOMODE_MODE_t N3DS_VIDEO_mode;
static int ctable[256];

// #define SOFTWARE_INTERLAVE

#ifdef SOFTWARE_INTERLEAVE
static u8 morton_lut[64] = {
        0x00, 0x01, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15,
	0x02, 0x03, 0x06, 0x07, 0x12, 0x13, 0x16, 0x17,
        0x08, 0x09, 0x0c, 0x0d, 0x18, 0x19, 0x1c, 0x1d,
	0x0a, 0x0b, 0x0e, 0x0f, 0x1a, 0x1b, 0x1e, 0x1f,
        0x20, 0x21, 0x24, 0x25, 0x30, 0x31, 0x34, 0x35,
	0x22, 0x23, 0x26, 0x27, 0x32, 0x33, 0x36, 0x37,
        0x28, 0x29, 0x2c, 0x2d, 0x38, 0x39, 0x3c, 0x3d,
	0x2a, 0x2b, 0x2e, 0x2f, 0x3a, 0x3b, 0x3e, 0x3f
};

static s8 morton_delta_y[8] = {
        0x02, 0x06, 0x02, 0x16, 0x02, 0x06, 0x02, 0x16
};

static void N3DS_RenderMorton8to32(u8 *src, u32 *dest)
{
	int x, y, xm, ym;
	u8 *rsrc;
	u32 *rdest;

	for (y = 0; y < VIDEOMODE_src_height; y+=8) {
		rdest = dest + (y << 9);
		for (x = 0; x < VIDEOMODE_src_width; x+=8) {
			rsrc = src + (Screen_WIDTH * y) + x;
			for (ym = 0; ym < 8; ym++) {
				rdest[0x00] = ctable[*(rsrc++)];
				rdest[0x01] = ctable[*(rsrc++)];
				rdest[0x04] = ctable[*(rsrc++)];
				rdest[0x05] = ctable[*(rsrc++)];
				rdest[0x10] = ctable[*(rsrc++)];
				rdest[0x11] = ctable[*(rsrc++)];
				rdest[0x14] = ctable[*(rsrc++)];
				rdest[0x15] = ctable[*(rsrc)];

				rsrc += (Screen_WIDTH - 7);
				rdest += morton_delta_y[ym];
			}
		}
	}
}

static void N3DS_RenderMorton32to32(u32 *src, u32 *dest, u32 pitch, u32 width, u32 height)
{
	int x, y, xm, ym;
	u32 *rsrc;
	u32 *rdest;

	for (y = 0; y < height; y+=8) {
		rdest = dest + (y << 9);
		for (x = 0; x < width; x+=8) {
			rsrc = src + (pitch * y) + x;
			for (ym = 0; ym < 8; ym++) {
				rdest[0x00] = *(rsrc++);
				rdest[0x01] = *(rsrc++);
				rdest[0x04] = *(rsrc++);
				rdest[0x05] = *(rsrc++);
				rdest[0x10] = *(rsrc++);
				rdest[0x11] = *(rsrc++);
				rdest[0x14] = *(rsrc++);
				rdest[0x15] = *(rsrc);

				rsrc += (pitch - 7);
				rdest += morton_delta_y[ym];
			}
		}
	}
}
#endif

void N3DS_VIDEO_PaletteUpdate()
{
	int i;

	for (i = 0; i < 256; i++)
	{
		ctable[i] = Colours_table[i] << 8 | 0xFF;
	}
}

static void N3DS_RenderNormal(u8 *src, u32 *dest)
{
	int x, y;
	int spitch = Screen_WIDTH - VIDEOMODE_src_width;
	int dpitch = 512 - VIDEOMODE_src_width;

	for (y = 0; y < VIDEOMODE_src_height; y++) {
		for (x = 0; x < VIDEOMODE_src_width; x++) {
			*(dest++) = ctable[*(src++)];
		}
		src += spitch;
		dest += dpitch;
	}
}

void N3DS_InitVideo(void)
{
	C3D_TexEnv* texEnv;

	gfxInitDefault();
	gfxSet3D(false);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	target_top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	target_bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetSetClear(target_top, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetClear(target_bottom, C3D_CLEAR_ALL, 0, 0);
	C3D_RenderTargetSetOutput(target_top, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
	C3D_RenderTargetSetOutput(target_bottom, GFX_BOTTOM, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));

#ifdef SOFTWARE_INTERLAVE
	C3D_TexInit(&tex, 512, 256, GPU_RGBA8);
#else
	C3D_TexInit(&tex, 512, 256, GPU_RGBA8);
#endif
	texBuf = linearAlloc(512 * 256 * 4);

	ctr_load_png(&kbd_display, "romfs:/kbd_display.png", TEXTURE_TARGET_VRAM);

	ctr_init_shader(&shader, shader_shbin, shader_shbin_size);
	AttrInfo_AddLoader(&(shader.attr), 0, GPU_FLOAT, 3); // v0 = position
	AttrInfo_AddLoader(&(shader.attr), 1, GPU_FLOAT, 2); // v1 = texcoord
	ctr_bind_shader(&shader);

	Mtx_OrthoTilt(&proj_top, 0.0, 400.0, 0.0, 240.0, -1.0, 1.0, true);
	Mtx_OrthoTilt(&proj_bottom, 0.0, 320.0, 0.0, 240.0, -1.0, 1.0, true);

	texEnv = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(texEnv, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(texEnv, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(texEnv, C3D_Both, GPU_MODULATE);

	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

void N3DS_ExitVideo(void)
{
	linearFree(texBuf);

	C3D_TexDelete(&kbd_display);
	C3D_TexDelete(&tex);

	C3D_RenderTargetDelete(target_top);
	C3D_RenderTargetDelete(target_bottom);

	C3D_Fini();
	gfxExit();
}

void PLATFORM_PaletteUpdate(void)
{
#ifdef PAL_BLENDING
	if (N3DS_VIDEO_mode == VIDEOMODE_MODE_NORMAL && ARTIFACT_mode == ARTIFACT_PAL_BLEND)
		PAL_BLENDING_UpdateLookup();
#endif
	N3DS_VIDEO_PaletteUpdate();
}

void PLATFORM_GetPixelFormat(PLATFORM_pixel_format_t *format)
{
	format->bpp = 32;
	format->rmask = 0xFF000000;
	format->gmask = 0x00FF0000;
	format->bmask = 0x0000FF00;
}

void PLATFORM_MapRGB(void *dest, int const *palette, int size)
{
	int i;
	u32* target = (u32*) dest;
	for (i = 0; i < 256; i++)
	{
		target[i] = (palette[i] << 8) | 0xFF;
	}
}

void PLATFORM_SetVideoMode(VIDEOMODE_resolution_t const *res, int windowed, VIDEOMODE_MODE_t mode, int rotate90)
{
	N3DS_VIDEO_mode = mode;

	PLATFORM_PaletteUpdate();
	PLATFORM_DisplayScreen();
}

VIDEOMODE_resolution_t *PLATFORM_AvailableResolutions(unsigned int *size)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 400;
	resolutions[0].height = 240;
	*size = 1;
	return resolutions;
}

VIDEOMODE_resolution_t *PLATFORM_DesktopResolution(void)
{
	VIDEOMODE_resolution_t *resolutions;
	resolutions = Util_malloc(1 * sizeof(VIDEOMODE_resolution_t));
	resolutions[0].width = 400;
	resolutions[0].height = 240;
	return resolutions;
}

int PLATFORM_SupportsVideomode(VIDEOMODE_MODE_t mode, int stretch, int rotate90)
{
	if (rotate90 != 0)
		return false;

	return mode == VIDEOMODE_MODE_NORMAL;
}

int PLATFORM_WindowMaximised(void)
{
	return 1;
}

void N3DS_DrawTexture(C3D_Tex* tex, int x, int y, int tx, int ty, int width, int height) {
	float txmin, tymin, txmax, tymax;
	txmin = (float) tx / tex->width;
	tymax = (float) ty / tex->height;
	txmax = (float) (tx+width) / tex->width;
	tymin = (float) (ty+height) / tex->height;

	C3D_TexBind(0, tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib((float) x, (float) 240 - y - height, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmin, (float) tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x + width, (float) 240 - y - height, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmax, (float) tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x, (float) 240 - y, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmin, (float) tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib((float) x + width, (float) 240 - y, 0.0f, 0.0f);
		C3D_ImmSendAttrib((float) txmax, (float) tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();
}

void PLATFORM_DisplayScreen(void)
{
	u8 *src;
#ifdef SOFTWARE_INTERLAVE
	u32 *dest;
#endif
	float xmin, ymin, xmax, ymax, txmin, tymin, txmax, tymax;

	src = (u8*) Screen_atari;
	src += Screen_WIDTH * VIDEOMODE_src_offset_top + VIDEOMODE_src_offset_left;
#ifdef SOFTWARE_INTERLAVE
	dest = (u32*) tex.data;

	if (!C3D_FrameBegin(0))
		return;
#endif

#ifdef PAL_BLENDING
	if (N3DS_VIDEO_mode == VIDEOMODE_MODE_NORMAL && ARTIFACT_mode == ARTIFACT_PAL_BLEND)
	{
#ifdef SOFTWARE_INTERLAVE
		PAL_BLENDING_Blit32(texBuf, src, Screen_WIDTH, VIDEOMODE_src_width, VIDEOMODE_src_height, VIDEOMODE_src_offset_top % 2);
		N3DS_RenderMorton32to32(texBuf, dest, Screen_WIDTH, VIDEOMODE_src_width, VIDEOMODE_src_height);
#else
		PAL_BLENDING_Blit32(texBuf, src, tex.width, VIDEOMODE_src_width, VIDEOMODE_src_height, VIDEOMODE_src_offset_top % 2);
#endif
	}
	else
#endif
	{
#ifdef SOFTWARE_INTERLAVE
		N3DS_RenderMorton8to32(src, dest);
#else
		N3DS_RenderNormal(src, texBuf);
#endif
	}

#ifdef SOFTWARE_INTERLAVE
	GSPGPU_FlushDataCache(dest, 512 * 256 * 4);
#else
	GSPGPU_FlushDataCache(texBuf, 512 * 256 * 4);
	C3D_SafeDisplayTransfer(texBuf, GX_BUFFER_DIM(512, 256), tex.data, GX_BUFFER_DIM(tex.width, tex.height),
		(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) |
		GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
	);
	gspWaitForPPF();

	if (!C3D_FrameBegin(0))
		return;

	GSPGPU_FlushDataCache(tex.data, 512 * 256 * 4);
#endif

	C3D_FrameDrawOn(target_bottom);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_bottom);
	C3D_TexBind(0, &kbd_display);
	N3DS_DrawKeyboard(&kbd_display);

	xmin = (400 - VIDEOMODE_dest_width) / 2.0f;
	ymin = (240 - VIDEOMODE_dest_height) / 2.0f;
	xmax = xmin + VIDEOMODE_dest_width;
	ymax = ymin + VIDEOMODE_dest_height;
	txmax = ((float) VIDEOMODE_src_width / tex.width);
	txmin = 0.0f;
#ifdef SOFTWARE_INTERLEAVE
	tymin = 1.0f - ((float) VIDEOMODE_src_height / tex.height);
	tymax = 1.0f;
#else
	tymin = ((float) VIDEOMODE_src_height / tex.height);
	tymax = 0.0f;
#endif

	C3D_FrameDrawOn(target_top);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shader.proj_loc, &proj_top);

	C3D_TexBind(0, &tex);
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib(xmin, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymin, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymin, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmin, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmin, tymax, 0.0f, 0.0f);

		C3D_ImmSendAttrib(xmax, ymax, 0.0f, 0.0f);
		C3D_ImmSendAttrib(txmax, tymax, 0.0f, 0.0f);
	C3D_ImmDrawEnd();

	C3D_FrameEnd(0);
}
