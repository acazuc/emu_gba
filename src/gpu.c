#include "gpu.h"
#include "mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

static void draw_objects(gpu_t *gpu, uint8_t y)
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
	for (uint8_t x = 0; x < 160; ++x)
	{
		for (uint8_t i = 0; i < 128; ++i)
		{
			uint16_t attr0 = mem_get_oam16(gpu->mem, i * 8);
			if ((attr0 & 0x300) == 0x200) //disable flag
				continue;
			uint8_t objy = attr0 & 0xFF;
			if (objy > y)
				continue;
			uint16_t attr1 = mem_get_oam16(gpu->mem, i * 8 + 2);
			uint8_t objx = attr1 & 0x1FF;
			if (objx > x)
				continue;
			uint8_t shape = (attr0 >> 14) & 0x3;
			uint8_t size = (attr1 >> 14) & 0x3;
			uint8_t width = widths[size + shape * 4];
			uint8_t height = heights[size + shape * 4];
			uint8_t doublesize = (attr0 >> 9) & 0x1;
			uint8_t affine = (attr0 >> 8) & 0x1;
			if (affine)
			{
				uint8_t affineidx = (attr1 >> 9) & 0x1F;
				uint16_t pa = mem_get_oam16(gpu->mem, 6 + affineidx * 8);
				uint16_t pb = mem_get_oam16(gpu->mem, 7 + affineidx * 8);
				uint16_t pc = mem_get_oam16(gpu->mem, 8 + affineidx * 8);
				uint16_t pd = mem_get_oam16(gpu->mem, 9 + affineidx * 8);
			}
			if (doublesize)
			{
				width *= 2;
				height *= 2;
			}
			if (objx + width <= x
			 || objy + height <= y)
				continue;
			uint16_t attr2 = mem_get_oam16(gpu->mem, i * 8 + 3);
			memset(&gpu->data[(240 * y + x) * 4], 0xC0 | (attr2 & 0x3F), 4);
		}
	}
}

static void draw_mode0(gpu_t *gpu, uint8_t y)
{
	memset(&gpu->data[160 * y * 4], 0, 240 * 4);
}

static void draw_mode2(gpu_t *gpu, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0x8))
		draw_background(gpu, y, 0);
	if (display & (1 << 0x9))
		draw_background(gpu, y, 1);
	if (display & (1 << 0xA))
		draw_background(gpu, y, 2);
	if (display & (1 << 0xB))
		draw_background(gpu, y, 3);
	if (display & (1 << 0xC))
		draw_objects(gpu, y);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
}

void gpu_draw(gpu_t *gpu, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	printf("display: %08x\n", display);
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
			printf("unsupported mode 3\n");
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
	memset(&gpu->data[160 * y * 4], 0, 240 * 4);
}
