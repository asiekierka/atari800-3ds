#ifndef _3DS_VIDEO_H_
#define _3DS_VIDEO_H_

#include <stdio.h>
#include "config.h"
#include "videomode.h"

void N3DS_InitVideo(void);
void N3DS_ExitVideo(void);
void N3DS_DrawTexture(C3D_Tex* tex, int x, int y, int tx, int ty, int width, int height);

#endif /* _3DS_VIDEO_H_ */
