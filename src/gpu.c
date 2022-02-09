#include "gpu.h"
#include "mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TO8(v) (((uint32_t)(v) * 527 + 23) >> 6)

gpu_t *gpu_new(mem_t *mem)
{
	gpu_t *gpu = calloc(sizeof(*gpu), 1);
	if (!gpu)
		return NULL;

	gpu->mem = mem;
	return gpu;
}

void gpu_del(gpu_t *gpu)
{
	if (!gpu)
		return;
	free(gpu);
}

static void draw_background(gpu_t *gpu, uint8_t y, uint8_t bg)
{
	(void)gpu;
	(void)y;
	(void)bg;
}

static void draw_window(gpu_t *gpu, uint8_t y, uint8_t win)
{
	(void)gpu;
	(void)y;
	(void)win;
}

static void draw_objects(gpu_t *gpu, uint32_t tileaddr, uint8_t y)
{
	static const uint8_t widths[16] =
	{
		8 , 16, 32, 64,
		16, 32, 32, 64,
		8 , 8 , 16, 32,
		0 , 0 , 0 , 0 ,
	};
	static const uint8_t heights[16] =
	{
		8 , 16, 32, 64,
		8 , 8 , 16, 32,
		16, 32, 32, 64,
		0 , 0 , 0 , 0 ,
	};
#if 0
	for (uint8_t x = 0; x < 240; ++x)
	{
		uint8_t xpos = x;
		uint8_t ypos = y + 80;
		uint8_t tilex = xpos / 8;
		uint8_t tilebx = xpos % 8;
		uint8_t tiley = ypos / 8;
		uint8_t tileby = ypos % 8;
		uint16_t tilepos = tilex + 0x10 * tiley;
		tilepos &= 0x1FF;
		uint32_t tilea = tileaddr + tilepos * 0x40 + tileby * 0x8 + tilebx;
		uint8_t tilev = mem_get_vram8(gpu->mem, tilea);
		uint16_t col = mem_get_obj_palette(gpu->mem, tilev * 2);
		uint8_t color[4] =
		{
			TO8((col >> 0x0) & 0x1F),
			TO8((col >> 0x5) & 0x1F),
			TO8((col >> 0xA) & 0x1F),
			0xFF,
		};
		memcpy(&gpu->data[(240 * y + x) * 4], color, 4);
	}
	return;
#endif

	for (int16_t i = 128; i >= 0; --i)
	{
		uint16_t attr0 = mem_get_oam16(gpu->mem, i * 8);
		if ((attr0 & 0x300) == 0x200) //disable flag
			continue;
		int16_t objy = attr0 & 0xFF;
		if (objy >= 160)
			objy -= 256;
		if (objy > y)
			continue;
		uint16_t attr1 = mem_get_oam16(gpu->mem, i * 8 + 2);
		int16_t objx = attr1 & 0x1FF;
		if (objx >= 240)
			objx -= 512;
		uint8_t shape = (attr0 >> 14) & 0x3;
		uint8_t size = (attr1 >> 14) & 0x3;
		uint8_t width = widths[size + shape * 4];
		uint8_t height = heights[size + shape * 4];
		uint8_t doublesize = (attr0 >> 9) & 0x1;
		if (doublesize)
		{
			width *= 2;
			height *= 2;
		}
		if (objy + height <= y)
			continue;
		uint8_t affine = (attr0 >> 8) & 0x1;
		int16_t pa;
		int16_t pb;
		int16_t pc;
		int16_t pd;
		if (affine)
		{
			uint16_t affineidx = ((attr1 >> 9) & 0x1F) * 0x20;
			pa = mem_get_oam16(gpu->mem, affineidx + 0x06);
			pb = mem_get_oam16(gpu->mem, affineidx + 0x0E);
			pc = mem_get_oam16(gpu->mem, affineidx + 0x16);
			pd = mem_get_oam16(gpu->mem, affineidx + 0x1E);
		}
		else
		{
			pa = 0x100;
			pb = 0;
			pc = 0;
			pd = 0x100;
		}
		uint16_t attr2 = mem_get_oam16(gpu->mem, i * 8 + 4);
		uint16_t tileid = attr2 & 0x3FF;
		uint8_t mode = (attr0 >> 10) & 0x3;
		uint8_t color_mode = (attr0 >> 13) & 0x1;
		if (mem_get_reg16(gpu->mem, MEM_REG_DISPCNT) & (1 << 6))
		{
		}
		else
		{
			if (color_mode)
				tileid >>= 1;
			uint8_t xsize = color_mode ? 16 : 32;
			uint8_t ysize = 32;
			int16_t centerx = width / 2;
			int16_t centery = height / 2;
			for (int16_t x = 0; x < width; ++x)
			{
				int16_t screenx = objx + x;
				if (screenx < 0 || screenx >= 240)
					continue;
				uint8_t xpos = x;
				uint8_t ypos = y - objy;
				int32_t texx;
				int32_t texy;
				if (affine)
				{
					int32_t dx = xpos - centerx;
					int32_t dy = ypos - centery;
					int32_t midx = centerx;
					int32_t midy = centery;
					int32_t maxx = width;
					int32_t maxy = height;
					if (doublesize)
					{
						midx /= 2;
						midy /= 2;
						maxx /= 2;
						maxy /= 2;
					}
					texx = ((pa * dx + pb * dy) >> 8) + midx;
					texy = ((pc * dx + pd * dy) >> 8) + midy;
					if (texx < 0 || texx >= maxx
					 || texy < 0 || texy >= maxy)
						continue;
				}
				else
				{
					texx = xpos;
					texy = ypos;
				}
				int16_t tilex = texx / 8;
				int16_t tilebx = texx % 8;
				int16_t tiley = texy / 8;
				int16_t tileby = texy % 8;
				uint16_t tilepos = tileid + tilex + xsize * tiley;
				tilepos &= 0x3FF;
				uint32_t tilea = tileaddr + tilepos * 0x40 + tileby * 0x8 + tilebx;
				uint8_t tilev = mem_get_vram8(gpu->mem, tilea);
				if (!tilev) //XXX
					continue;
				uint16_t col = mem_get_obj_palette(gpu->mem, tilev * 2);
				uint8_t color[4] =
				{
					TO8((col >> 0xA) & 0x1F),
					TO8((col >> 0x5) & 0x1F),
					TO8((col >> 0x0) & 0x1F),
					0xFF,
				};
				memcpy(&gpu->data[(240 * y + screenx) * 4], color, 4);
			}
		}
	}
}

static void draw_mode0(gpu_t *gpu, uint8_t y)
{
}

static void draw_mode2(gpu_t *gpu, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background(gpu, y, 2);
	if (display & (1 << 0xB))
		draw_background(gpu, y, 3);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
}

static void draw_mode3(gpu_t *gpu, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background(gpu, y, 2);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
}

void gpu_draw(gpu_t *gpu, uint8_t y)
{
	memset(&gpu->data[240 * y * 4], mem_get_bg_palette(gpu->mem, 0), 240 * 4);
	uint16_t display = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	switch (display & 0x7)
	{
		case 0:
			draw_mode0(gpu, y);
			return;
		case 1:
			printf("unsupported mode 1\n");
			break;
		case 2:
			draw_mode2(gpu, y);
			return;
		case 3:
			draw_mode3(gpu, y);
			break;
		case 4:
			printf("unsupported mode 4\n");
			break;
		case 5:
			printf("unsupported mode 5\n");
			break;
		default:
			printf("invalid mode: %x\n", display & 0x7);
			break;
	}
}
