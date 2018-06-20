/*
 * 3ds/input.c - Nintendo 3DS input device support
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
#include <citro3d.h>

#include "config.h"
#include "akey.h"
#include "input.h"
#include "input_3ds.h"
#include "log.h"
#include "platform.h"
#include "ui.h"
#include "video.h"

int key_control;
int current_key_down = AKEY_NONE;
int dpad_as_keyboard = 1;
u64 key_down_time = 0;

#define WARMSTART_HOLD_TIME 3*1000
#define AKEY_WAS_SHIFT_CTRL -992

touch_area_t N3DS_touch_areas_key[] = {
	{ 2, 130, 22, 22, AKEY_ESCAPE, 0 },
	{ 23, 130, 22, 22, AKEY_1, 0 },
	{ 44, 130, 22, 22, AKEY_2, 0 },
	{ 65, 130, 22, 22, AKEY_3, 0 },
	{ 86, 130, 22, 22, AKEY_4, 0 },
	{ 107, 130, 22, 22, AKEY_5, 0 },
	{ 128, 130, 22, 22, AKEY_6, 0 },
	{ 149, 130, 22, 22, AKEY_7, 0 },
	{ 170, 130, 22, 22, AKEY_8, 0 },
	{ 191, 130, 22, 22, AKEY_9, 0 },
	{ 212, 130, 22, 22, AKEY_0, 0 },
	{ 233, 130, 22, 22, AKEY_LESS, 0 },
	{ 254, 130, 22, 22, AKEY_GREATER, 0 },
	{ 275, 130, 22, 22, AKEY_BACKSPACE, 0 },
	{ 296, 130, 22, 22, AKEY_BREAK, 0 },

	{ 2, 151, 34, 22, AKEY_TAB, 0 },
	{ 35, 151, 22, 22, AKEY_q, 0 },
	{ 56, 151, 22, 22, AKEY_w, 0 },
	{ 77, 151, 22, 22, AKEY_e, 0 },
	{ 98, 151, 22, 22, AKEY_r, 0 },
	{ 119, 151, 22, 22, AKEY_t, 0 },
	{ 140, 151, 22, 22, AKEY_y, 0 },
	{ 161, 151, 22, 22, AKEY_u, 0 },
	{ 182, 151, 22, 22, AKEY_i, 0 },
	{ 203, 151, 22, 22, AKEY_o, 0 },
	{ 224, 151, 22, 22, AKEY_p, 0 },
	{ 245, 151, 22, 22, AKEY_MINUS, 0 },
	{ 266, 151, 22, 22, AKEY_EQUAL, 0 },
	{ 287, 151, 31, 22, AKEY_RETURN, 0 },

	{ 2, 172, 37, 22, AKEY_CTRL, 0 },
	{ 38, 172, 22, 22, AKEY_a, 0 },
	{ 59, 172, 22, 22, AKEY_s, 0 },
	{ 80, 172, 22, 22, AKEY_d, 0 },
	{ 101, 172, 22, 22, AKEY_f, 0 },
	{ 122, 172, 22, 22, AKEY_g, 0 },
	{ 143, 172, 22, 22, AKEY_h, 0 },
	{ 164, 172, 22, 22, AKEY_j, 0 },
	{ 185, 172, 22, 22, AKEY_k, 0 },
	{ 206, 172, 22, 22, AKEY_l, 0 },
	{ 227, 172, 22, 22, AKEY_SEMICOLON, 0 },
	{ 248, 172, 22, 22, AKEY_PLUS, 0 },
	{ 269, 172, 22, 22, AKEY_ASTERISK, 0 },
	{ 290, 172, 28, 22, AKEY_CAPSTOGGLE, 0 },

	{ 2, 193, 43, 22, AKEY_SHFT, 0 },
	{ 44, 193, 22, 22, AKEY_z, 0 },
	{ 65, 193, 22, 22, AKEY_x, 0 },
	{ 86, 193, 22, 22, AKEY_c, 0 },
	{ 107, 193, 22, 22, AKEY_v, 0 },
	{ 128, 193, 22, 22, AKEY_b, 0 },
	{ 149, 193, 22, 22, AKEY_n, 0 },
	{ 170, 193, 22, 22, AKEY_m, 0 },
	{ 191, 193, 22, 22, AKEY_COMMA, 0 },
	{ 212, 193, 22, 22, AKEY_FULLSTOP, 0 },
	{ 233, 193, 22, 22, AKEY_SLASH, 0 },
	{ 254, 193, 43, 22, AKEY_SHFT, 0 },
	{ 296, 193, 22, 22, AKEY_ATARI, 0 },

	{ 64, 214, 190, 22, AKEY_SPACE, 0 },

	{ 153, 98, 32, 16, AKEY_HELP, TA_FLAG_SLANTED },
	{ 186, 98, 32, 16, AKEY_START, TA_FLAG_SLANTED },
	{ 219, 98, 32, 16, AKEY_SELECT, TA_FLAG_SLANTED },
	{ 252, 98, 32, 16, AKEY_OPTION, TA_FLAG_SLANTED },
	{ 285, 98, 32, 16, AKEY_WARMSTART, TA_FLAG_SLANTED }
};

#define N3DS_TOUCH_AREA_KEY_MAX (sizeof(N3DS_touch_areas_key) / sizeof(touch_area_t))

touch_area_t N3DS_touch_areas_5200[] = {
	{ 95, 73, 38, 22, AKEY_5200_1, 0 },
	{ 141, 73, 38, 22, AKEY_5200_2, 0 },
	{ 187, 73, 38, 22, AKEY_5200_3, 0 },
	{ 95, 109, 38, 22, AKEY_5200_4, 0 },
	{ 141, 109, 38, 22, AKEY_5200_5, 0 },
	{ 187, 109, 38, 22, AKEY_5200_6, 0 },
	{ 95, 145, 38, 22, AKEY_5200_7, 0 },
	{ 141, 145, 38, 22, AKEY_5200_8, 0 },
	{ 187, 145, 38, 22, AKEY_5200_9, 0 },
	{ 95, 181, 38, 22, AKEY_5200_ASTERISK, 0 },
	{ 141, 181, 38, 22, AKEY_5200_0, 0 },
	{ 187, 181, 38, 22, AKEY_5200_HASH, 0 }
};

#define N3DS_TOUCH_AREA_5200_MAX (sizeof(N3DS_touch_areas_5200) / sizeof(touch_area_t))

#define N3DS_TOUCH_AREAS ((Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active) \
		? N3DS_touch_areas_5200 : N3DS_touch_areas_key)
#define N3DS_TOUCH_AREA_MAX ((Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active) \
		? N3DS_TOUCH_AREA_5200_MAX : N3DS_TOUCH_AREA_KEY_MAX)

bool N3DS_IsControlPressed()
{
	return key_control != 0;
}

static bool isKeyTouched(touchPosition* pos, touch_area_t* area)
{
	if (area->flags & TA_FLAG_SLANTED)
	{
		if (pos->py >= area->y && pos->py < (area->y + area->h))
		{
			int slx = pos->px + (pos->py - area->y);
			return slx >= area->x && slx < (area->x + area->w);
		}

		return false;
	} else
	{
		return pos->px >= area->x && pos->py >= area->y
			&& pos->px < (area->x + area->w)
			&& pos->py < (area->y + area->h);
	}
}

void N3DS_DrawKeyboard(C3D_Tex *tex)
{
	touch_area_t* keyTable = N3DS_TOUCH_AREAS;
	int keyTableLen = N3DS_TOUCH_AREA_MAX;

	if (keyTable == N3DS_touch_areas_5200)
	{
		N3DS_DrawTexture(tex, 0, 0, 640, 0, 320, 240);
		return;
	}

	touchPosition pos;
	bool isTouch = ((hidKeysDown() | hidKeysHeld()) & KEY_TOUCH) != 0;

	N3DS_DrawTexture(tex, 0, 0, 0, 0, 320, 240);

	int key_down = current_key_down;
	if (key_down >= 0) key_down &= ~AKEY_SHFTCTRL;

	hidTouchRead(&pos);
	for (int i = 0; i < keyTableLen; i++)
	{
		touch_area_t* area = &keyTable[i];
		if (key_down == area->keycode
                        || (INPUT_key_shift != 0 && area->keycode == AKEY_SHFT)
                        || (N3DS_IsControlPressed() && area->keycode == AKEY_CTRL)
			|| (isTouch && (
				(area->keycode == AKEY_START) ||
				(area->keycode == AKEY_SELECT) ||
				(area->keycode == AKEY_OPTION)
			) && isKeyTouched(&pos, area))
		)
		{
			if (area->flags & TA_FLAG_SLANTED)
			{
				for (int i = 0; i < area->h; i++)
					N3DS_DrawTexture(tex, area->x - i, area->y + i,
						area->x + 320 - i, area->y + i, area->w, 1);
			}
			else
				N3DS_DrawTexture(tex, area->x, area->y,
					area->x + 320, area->y, area->w, area->h);
		}
	}
}

void PLATFORM_SetJoystickKey(int joystick, int direction, int value)
{
}

void PLATFORM_GetJoystickKeyName(int joystick, int direction, char *buffer, int bufsize)
{
}

int PLATFORM_GetRawKey(void)
{
	return 0;
}

int PLATFORM_Keyboard(void)
{
	u32 kDown, kHeld, kUp;
	touchPosition pos;
	int shiftctrl = (INPUT_key_shift != 0 ? AKEY_SHFT : 0) | (key_control != 0 ? AKEY_CTRL : 0);

	hidScanInput();
	kDown = hidKeysDown();
	kHeld = hidKeysHeld();
	kUp = hidKeysUp();

	// reset
	INPUT_key_consol = INPUT_CONSOL_NONE;

	if (kUp & KEY_TOUCH)
	{
		if (current_key_down != AKEY_WAS_SHIFT_CTRL)
		{
			INPUT_key_shift = 0;
			key_control = 0;
		}
		current_key_down = AKEY_NONE;
		Atari800_display_screen = TRUE;
	}

	if (UI_is_active)
	{
		if (kDown & KEY_A)
			return AKEY_RETURN;
		if (kDown & KEY_B)
			return AKEY_ESCAPE;
		if (kDown & KEY_X)
			return AKEY_SPACE;
		if (kDown & KEY_Y)
			return AKEY_BACKSPACE;
		if (kHeld & KEY_UP)
			return AKEY_UP;
		if (kHeld & KEY_LEFT)
			return AKEY_LEFT;
		if (kHeld & KEY_RIGHT)
			return AKEY_RIGHT;
		if (kHeld & KEY_DOWN)
			return AKEY_DOWN;
		if (kDown & KEY_L)
			return AKEY_COLDSTART;
		if (kDown & KEY_R)
			return AKEY_WARMSTART;
	} else if (Atari800_machine_type == Atari800_MACHINE_5200) {
		// second fire key
		INPUT_key_shift = (kDown & KEY_B) ? 1 : 0;

		if (kHeld & KEY_START)
			return AKEY_5200_START;
		if (kHeld & KEY_SELECT)
			return AKEY_5200_PAUSE;
		if (kHeld & KEY_R)
			return AKEY_5200_RESET;
		if (kHeld & KEY_L)
			return AKEY_UI;
	} else
	{
		if (kHeld & KEY_START)
			INPUT_key_consol &= ~INPUT_CONSOL_START;
		if (kHeld & KEY_SELECT)
			INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
		if (kHeld & KEY_R)
			INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
		if (kHeld & KEY_B)
			return AKEY_RETURN;
		if (kHeld & KEY_Y)
			return AKEY_SPACE;
		if (kHeld & KEY_L)
			return AKEY_UI;
		if (dpad_as_keyboard) {
			if (kHeld & KEY_DUP)
				return AKEY_UP;
			if (kHeld & KEY_DLEFT)
				return AKEY_LEFT;
			if (kHeld & KEY_DRIGHT)
				return AKEY_RIGHT;
			if (kHeld & KEY_DDOWN)
				return AKEY_DOWN;
		}
	}

	if ((kDown | kHeld) & KEY_TOUCH)
	{
		hidTouchRead(&pos);
		touch_area_t* keyTable = N3DS_TOUCH_AREAS;
		bool down = (kDown & KEY_TOUCH) != 0;
		bool touching = ((kDown | kHeld) & KEY_TOUCH) != 0;
		bool refresh = false;

		if (down) current_key_down = AKEY_NONE;

		for (int i = 0; i < N3DS_TOUCH_AREA_MAX; i++)
		{
			touch_area_t* area = &keyTable[i];
			if (isKeyTouched(&pos, area))
			{
				refresh |= down;
				switch (area->keycode)
				{
					case AKEY_SHFT:
						if (down) Atari800_display_screen = TRUE;
						INPUT_key_shift = 1;
						current_key_down = AKEY_WAS_SHIFT_CTRL;
						return AKEY_NONE;
					case AKEY_CTRL:
						if (down) Atari800_display_screen = TRUE;
						key_control = 1;
						current_key_down = AKEY_WAS_SHIFT_CTRL;
						return AKEY_NONE;
					case AKEY_START:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_START;
						break;
					case AKEY_SELECT:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
						break;
					case AKEY_OPTION:
						if (touching) INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
						break;
					case AKEY_WARMSTART:
						if (down)
						{
							key_down_time = osGetTime();
							current_key_down = AKEY_NONE;
							break;
						} else if (touching && (osGetTime() - key_down_time) > WARMSTART_HOLD_TIME)
						{
							Atari800_display_screen = TRUE;
							return AKEY_WARMSTART;
						}
					default:
						if (down) current_key_down = area->keycode >= 0
							? (area->keycode | shiftctrl)
							: area->keycode;
						break;
				}
				break;
			}
		}

		if (refresh) Atari800_display_screen = TRUE;
		return current_key_down;
	}

	return AKEY_NONE;
}

int Atari_POT(int num)
{
	circlePosition pos;

	if (Atari800_machine_type == Atari800_MACHINE_5200)
	{
		int cid = num >> 1;
		int caxis = num & 1;
		if (cid != 0)
			return INPUT_joy_5200_center;

		hidScanInput();
		hidCircleRead(&pos);
		int val = caxis ? -pos.dy : pos.dx;
		if (val > 10) {
			return (INPUT_joy_5200_max - INPUT_joy_5200_center) * val / 0x9C + INPUT_joy_5200_center;
		} else if (val < -10) {
			return (INPUT_joy_5200_min - INPUT_joy_5200_center) * (-val) / 0x9C + INPUT_joy_5200_center;
		} else {
			return INPUT_joy_5200_center;
		}
	} else
	{
		return 228;
	}
}

int PLATFORM_PORT(int num)
{
	int ret = 0xff;
	if (num == 0) {
		hidScanInput();
		u32 kDown = hidKeysHeld();
		if ((!dpad_as_keyboard || Atari800_machine_type == Atari800_MACHINE_5200) && !UI_is_active)
		{
			if (kDown & KEY_LEFT)
				ret &= 0xf0 | INPUT_STICK_LEFT;
			if (kDown & KEY_RIGHT)
				ret &= 0xf0 | INPUT_STICK_RIGHT;
			if (kDown & KEY_UP)
				ret &= 0xf0 | INPUT_STICK_FORWARD;
			if (kDown & KEY_DOWN)
				ret &= 0xf0 | INPUT_STICK_BACK;
		} else
		{
			if (kDown & KEY_CPAD_LEFT)
				ret &= 0xf0 | INPUT_STICK_LEFT;
			if (kDown & KEY_CPAD_RIGHT)
				ret &= 0xf0 | INPUT_STICK_RIGHT;
			if (kDown & KEY_CPAD_UP)
				ret &= 0xf0 | INPUT_STICK_FORWARD;
			if (kDown & KEY_CPAD_DOWN)
				ret &= 0xf0 | INPUT_STICK_BACK;
		}
	}
	return ret;
}

int PLATFORM_TRIG(int num)
{
	if (num == 0) {
		hidScanInput();
		if (hidKeysHeld() & KEY_A)
			return 0;
	}
	return 1;
}
