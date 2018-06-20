/*
 * 3ds/main.c - Nintendo 3DS port main code
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
#include <stdio.h>
#include <string.h>

/* Atari800 includes */
#include "atari.h"
#include "config.h"
#include "../input.h"
#include "log.h"
#include "monitor.h"
#include "platform.h"
#include "pokeysnd.h"
#ifdef SOUND
#include "../sound.h"
#endif
#include "video.h"
#include "videomode.h"

static bool PLATFORM_IsNew3DS;

extern int dpad_as_keyboard;

int PLATFORM_Configure(char *option, char *parameters)
{
	if (strcmp(option, "N3DS_DPAD_MODE") == 0)
	{
		sscanf(parameters, "%d", &dpad_as_keyboard);
		return 1;
	}
	return 0;
}

void PLATFORM_ConfigSave(FILE *fp)
{
	fprintf(fp, "N3DS_DPAD_MODE=%d\n", dpad_as_keyboard);
}

int PLATFORM_Initialise(int *argc, char *argv[])
{
	return TRUE;
}

void PLATFORM_Sleep(double s)
{
	if (s > 0) {
		svcSleepThread(s * 1e9);
	}
}

double PLATFORM_Time(void)
{
	return osGetTime() * 1e-3;
}

int PLATFORM_Exit(int run_monitor)
{
	Log_flushlog();

	if (run_monitor) {
		return 1;
	} else {
		N3DS_ExitVideo();

		acExit();

		ptmSysmExit();
		romfsExit();
	}

	return 0;
}

int main(int argc, char **argv)
{
	romfsInit();

	ptmSysmInit();
	osSetSpeedupEnable(1);
	APT_SetAppCpuTimeLimit(80);

	acInit();

	// set config defaults
	PLATFORM_IsNew3DS = PTMSYSM_CheckNew3DS();
	POKEYSND_enable_new_pokey = PLATFORM_IsNew3DS;

	N3DS_InitVideo();

	if (!Atari800_Initialise(&argc, argv))
	{
		printf("Atari initialisation failed!");
		return 0;
	}

	while (aptMainLoop()) {
		INPUT_key_code = PLATFORM_Keyboard();
		Atari800_Frame();
		if (Atari800_display_screen)
			PLATFORM_DisplayScreen();
	}
}

/*
vim:ts=4:sw=4:
*/
