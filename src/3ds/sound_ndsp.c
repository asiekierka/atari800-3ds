/*
 * 3ds/sound_ndsp.c - Nintendo 3DS NDSP audio backend
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
#include <stdlib.h>
#include <string.h>

#include "atari.h"
#include "log.h"
#include "platform.h"
#include "sound.h"

int N3DS_soundFreq;
bool N3DS_soundFillBlock;
ndspWaveBuf N3DS_audioBuf[2];
u8*	N3DS_audioData;
u32	N3DS_bufferSize, N3DS_sampleSize;
Sound_setup_t *N3DS_sound;

void N3DS_ClearAudioData(void);

int PLATFORM_SoundSetup(Sound_setup_t *setup)
{
	setup->buffer_frames = (setup->buffer_frames + 7) & (~0x07);

	if (setup->buffer_frames == 0)
	{
		setup->buffer_frames = 4096;
	}

	N3DS_bufferSize = setup->buffer_frames;
	N3DS_sampleSize = setup->sample_size * setup->channels;

	N3DS_audioData = (u8*) linearAlloc(N3DS_bufferSize * 2 * N3DS_sampleSize);
	N3DS_ClearAudioData();

	ndspInit();

	N3DS_sound = setup;
	PLATFORM_SoundContinue();

	return TRUE;
}

void N3DS_SoundCallback(void* dud)
{
	if (N3DS_audioBuf[N3DS_soundFillBlock].status == NDSP_WBUF_DONE) {
		u32 flen = (N3DS_bufferSize * N3DS_sampleSize);
		u32 ilen = flen >> 2;
		u32* invbuf = (u32*) N3DS_audioBuf[N3DS_soundFillBlock].data_vaddr;

		if (N3DS_sound->sample_size > 1)
		{
			Sound_Callback((u8*) N3DS_audioBuf[N3DS_soundFillBlock].data_pcm8, flen);
		} else
		{
			Sound_Callback((u8*) N3DS_audioBuf[N3DS_soundFillBlock].data_pcm8, flen);
			for(int i = 0; i < ilen; i++)
				invbuf[i] ^= 0x80808080;
		}

		DSP_FlushDataCache(N3DS_audioBuf[N3DS_soundFillBlock].data_pcm8, flen);

		ndspChnWaveBufAdd(0, &N3DS_audioBuf[N3DS_soundFillBlock]);
		N3DS_soundFillBlock = !N3DS_soundFillBlock;
	}
}

void N3DS_ClearAudioData(void)
{
	memset(N3DS_audioData, 0, N3DS_bufferSize * 2 * N3DS_sampleSize);
	N3DS_soundFillBlock = false;
}

void PLATFORM_SoundLock(void)
{
	// TODO
}

void PLATFORM_SoundUnlock(void)
{
	// TODO
}

void PLATFORM_SoundExit(void)
{
	linearFree(N3DS_audioData);
	ndspExit();
}

void PLATFORM_SoundPause(void)
{
	ndspChnWaveBufClear(0);
	N3DS_ClearAudioData();
}

void PLATFORM_SoundContinue(void)
{
	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;

	ndspSetOutputMode(N3DS_sound->channels == 2 ? NDSP_OUTPUT_STEREO : NDSP_OUTPUT_MONO);

	ndspChnReset(0);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, N3DS_sound->freq);
	ndspChnSetFormat(0, NDSP_CHANNELS(N3DS_sound->channels) | NDSP_ENCODING(N3DS_sound->sample_size == 2 ? NDSP_ENCODING_PCM16 : NDSP_ENCODING_PCM8));
	ndspChnSetMix(0, mix);

	ndspSetOutputCount(1);
	ndspSetMasterVol(1.0f);
	ndspSetCallback(N3DS_SoundCallback, N3DS_audioBuf);

	memset(N3DS_audioBuf, 0, sizeof(N3DS_audioBuf));
	N3DS_audioBuf[0].data_vaddr = &N3DS_audioData[0];
	N3DS_audioBuf[0].nsamples = N3DS_bufferSize;
	N3DS_audioBuf[1].data_vaddr = &N3DS_audioData[N3DS_bufferSize * N3DS_sampleSize];
	N3DS_audioBuf[1].nsamples = N3DS_bufferSize;

	N3DS_ClearAudioData();

	ndspChnWaveBufAdd(0, &N3DS_audioBuf[0]);
	ndspChnWaveBufAdd(0, &N3DS_audioBuf[1]);
}
