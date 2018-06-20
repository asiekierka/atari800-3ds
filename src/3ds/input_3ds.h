#ifndef _INPUT_3DS_H_
#define _INPUT_3DS_H_

#include <3ds.h>
#include <citro3d.h>
#include "akey.h"

bool N3DS_IsControlPressed();
void N3DS_DrawKeyboard(C3D_Tex *tex);

typedef struct
{
	u16 x, y, w, h;
	s16 keycode;
	u8 flags;
} touch_area_t;

#define TA_FLAG_SLANTED 1

#endif /* _INPUT_3DS_H */
