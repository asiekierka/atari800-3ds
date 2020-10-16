#include "grapefruit.h"
#include <png.h>
#include <stdlib.h>

static u32 next_power_of_two(u32 v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v + 1;
}

bool ctr_load_png(C3D_Tex* tex, const char* name, texture_location loc)
{
	png_image img;
	u32 *data;

	memset(&img, 0, sizeof(png_image));
	img.version = PNG_IMAGE_VERSION;

	if (!png_image_begin_read_from_file(&img, name)) {
		return false;
	}

	img.format = PNG_FORMAT_ABGR;

	if (loc == TEXTURE_TARGET_VRAM) {
		C3D_TexInitVRAM(tex, next_power_of_two(img.width), next_power_of_two(img.height), GPU_RGBA8);
	} else {
		C3D_TexInit(tex, next_power_of_two(img.width), next_power_of_two(img.height), GPU_RGBA8);
	}
	data = linearAlloc(tex->width * tex->height * sizeof(u32));

	if (!png_image_finish_read(&img, NULL, data, tex->width * sizeof(u32), NULL)) {
		linearFree(data);
		C3D_TexDelete(tex);
		return false;
	}

	GSPGPU_FlushDataCache(data, tex->width * tex->height * sizeof(u32));

	C3D_SyncDisplayTransfer(data, GX_BUFFER_DIM(tex->width, tex->height),
		tex->data, GX_BUFFER_DIM(tex->width, tex->height),
		GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) |
		GX_TRANSFER_IN_FORMAT(GPU_RGBA8) | GX_TRANSFER_OUT_FORMAT(GPU_RGBA8)
		| GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));

	linearFree(data);
	png_image_free(&img);
	return true;
}

void ctr_init_shader(struct ctr_shader_data *shader, const void* data, int size)
{
	shader->dvlb = DVLB_ParseFile((u32 *) data, size);
	shaderProgramInit(&shader->program);
	shaderProgramSetVsh(&shader->program, &shader->dvlb->DVLE[0]);
	shader->proj_loc = shaderInstanceGetUniformLocation(shader->program.vertexShader, "projection");
	AttrInfo_Init(&shader->attr);
}

void ctr_bind_shader(struct ctr_shader_data *shader)
{
	C3D_BindProgram(&shader->program);
	C3D_SetAttrInfo(&shader->attr);
}
