#ifndef _3DS_VIDEO_H_
#define _3DS_VIDEO_H_

#include <stdio.h>

#include <3ds.h>
#include <citro3d.h>

#include "config.h"
#include "videomode.h"

void N3DS_InitVideo(void);
void N3DS_ExitVideo(void);
void N3DS_DrawTexture(C3D_Tex* tex, int x, int y, int tx, int ty, int width, int height);

void N3DS_ToggleVsync(void);
bool N3DS_IsVsyncEnabled(void);

#endif /* _3DS_VIDEO_H_ */
