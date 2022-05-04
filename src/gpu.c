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

static void draw_background_tiled(gpu_t *gpu, uint8_t y, uint8_t bg, uint8_t *data)
{
	uint16_t bgcnt = mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + bg * 2);
	uint8_t bghofs = mem_get_reg16(gpu->mem, MEM_REG_BG0HOFS + bg * 4) & 0x1FF;
	uint8_t bgvofs = mem_get_reg16(gpu->mem, MEM_REG_BG0VOFS + bg * 4) & 0x1FF;
	uint8_t size = (bgcnt >> 14) & 0x3;
	uint32_t tilebase = ((bgcnt >> 2) & 0x3) * 0x4000;
	uint32_t mapbase = ((bgcnt >> 8) & 0x1F) * 0x800;
	printf("layer %d cnt: %x\n", bg, bgcnt);
	uint32_t mapw;
	uint32_t maph;
	if (bg == 0)
	{
		mapw = 32;
		maph = 32;
	}
	else
	{
		static const uint32_t mapsizes[] = {16, 32, 64, 128};
		mapw = mapsizes[size];
		maph = mapsizes[size];
	}
	for (size_t x = 0; x < 240; ++x)
	{
		uint32_t diffx = x + bghofs;
		uint32_t diffy = y + bgvofs;
		uint32_t mapx = diffx / 8;
		uint32_t mapy = diffy / 8;
		uint32_t mapaddr = mapbase + (mapx + mapy * 32) * 2;
		uint16_t map = mem_get_vram16(gpu->mem, mapaddr);
		uint32_t tileid = map & 0x3FF;
		uint32_t tilex = diffx % 8;
		uint32_t tiley = diffy % 8;
		if (map & (1 << 10))
			tilex = 7 - tilex;
		if (map & (1 << 11))
			tiley = 7 - tiley;
		uint8_t paladdr;
		uint16_t tileaddr = tilebase;
		if (bgcnt & (1 << 7))
		{
			tileaddr += tileid * 0x40;
			paladdr = mem_get_vram8(gpu->mem, tileaddr + tilex + tiley * 8);
		}
		else
		{
			tileaddr += tileid * 0x20;
			if (tilex & 1)
				paladdr = mem_get_vram8(gpu->mem, tileaddr + tilex / 2 + tiley * 4) >> 4;
			else
				paladdr = mem_get_vram8(gpu->mem, tileaddr + tilex / 2 + tiley * 4) & 0xF;
			paladdr += ((map >> 12) & 0xF) * 0x10;
		}
		uint16_t val = mem_get_bg_palette(gpu->mem, paladdr * 2);
		uint8_t color[4] =
		{
			TO8((val >> 0xA) & 0x1F),
			TO8((val >> 0x5) & 0x1F),
			TO8((val >> 0x0) & 0x1F),
			0xFF,
		};
		memcpy(&data[x * 4], color, 4);
	}
	(void)y;
	(void)bg;
}

static void draw_background_bitmap_3(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	uint16_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	uint16_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	uint16_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	uint16_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	uint32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X);
	uint32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y);
	uint16_t hofs = mem_get_reg32(gpu->mem, MEM_REG_BG2HOFS) & 0x1FF;
	uint16_t vofs = mem_get_reg32(gpu->mem, MEM_REG_BG2VOFS) & 0x1FF;
	for (size_t x = 0; x < 240; ++x)
	{
		int32_t dx = x - bgx;
		int32_t dy = y - bgy;
		uint32_t vx = ((pa * dx + pb * dy) >> 8) + bgx;
		uint32_t vy = ((pc * dx + pd * dy) >> 8) + bgy;
		uint32_t addr = 2 * (x + 240 * y);
		uint16_t val = mem_get_vram16(gpu->mem, addr);
		uint8_t color[4] =
		{
			TO8((val >> 0xA) & 0x1F),
			TO8((val >> 0x5) & 0x1F),
			TO8((val >> 0x0) & 0x1F),
			0xFF,
		};
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_4(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	uint16_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	uint16_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	uint16_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	uint16_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	uint32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X);
	uint32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y);
	uint16_t hofs = mem_get_reg32(gpu->mem, MEM_REG_BG2HOFS) & 0x1FF;
	uint16_t vofs = mem_get_reg32(gpu->mem, MEM_REG_BG2VOFS) & 0x1FF;
	uint16_t dispcnt = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	uint32_t addr_offset = (dispcnt & (1 << 4)) ? 0xA000 : 0;
	for (size_t x = 0; x < 240; ++x)
	{
		int32_t dx = x - bgx;
		int32_t dy = y - bgy;
		uint32_t vx = ((pa * dx + pb * dy) >> 8) + bgx;
		uint32_t vy = ((pc * dx + pd * dy) >> 8) + bgy;
		uint32_t addr = addr_offset + x + 240 * y;
		uint8_t val = mem_get_vram8(gpu->mem, addr);
		if (!val)
			continue;
		uint16_t col = mem_get_bg_palette(gpu->mem, val * 2);
		uint8_t color[4] =
		{
			TO8((col >> 0xA) & 0x1F),
			TO8((col >> 0x5) & 0x1F),
			TO8((col >> 0x0) & 0x1F),
			0xFF,
		};
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_5(gpu_t *gpu, uint8_t y, uint8_t *data)
{
}

static void draw_window(gpu_t *gpu, uint8_t y, uint8_t win)
{
	(void)gpu;
	(void)y;
	(void)win;
}

static void draw_objects(gpu_t *gpu, uint32_t tileaddr, uint8_t y, uint8_t *data)
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
		memcpy(&data[x * 4], color, 4);
	}
	return;
#endif

	uint8_t written[240] = {0};
	uint16_t bldalpha = mem_get_reg16(gpu->mem, MEM_REG_BLDALPHA);
	uint32_t eva = (bldalpha >> 0) & 0x1F;
	uint32_t evb = (bldalpha >> 8) & 0x1F;

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
		//printf("obj[%d] = %d, x: %d, y: %d\n", i, tileid, objx, objy);
		if (mem_get_reg16(gpu->mem, MEM_REG_DISPCNT) & (1 << 6))
		{
			//XXX
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
				/*if (written[screenx])
				{
					uint8_t *cur = &gpu->data[(240 * y + screenx) * 4];
					color[0] = ((color[0] * eva) >> 4) + ((cur[0] * evb) >> 4);
					color[1] = ((color[1] * eva) >> 4) + ((cur[1] * evb) >> 4);
					color[2] = ((color[2] * eva) >> 4) + ((cur[2] * evb) >> 4);
					printf("eva: %x, evb: %x\n", eva, evb);
				}
				else
				{
					written[screenx] = 1;
				}*/
				memcpy(&data[screenx * 4], color, 4);
			}
		}
	}
}

static void draw_window_obj(gpu_t *gpu, uint8_t y)
{
}

static void draw_mode0(gpu_t *gpu, uint8_t y)
{
	uint8_t bg0_data[240 * 4];
	uint8_t bg1_data[240 * 4];
	uint8_t bg2_data[240 * 4];
	uint8_t bg3_data[240 * 4];
	uint8_t obj_data[240 * 4];
	uint8_t *bg_data[4] = {&bg0_data[0], &bg1_data[0], &bg2_data[0], &bg3_data[0]};
	memset(bg0_data, 0, sizeof(bg0_data));
	memset(bg1_data, 0, sizeof(bg1_data));
	memset(bg2_data, 0, sizeof(bg2_data));
	memset(bg3_data, 0, sizeof(bg3_data));
	memset(obj_data, 0, sizeof(obj_data));
	uint8_t bg_prio[4];
	uint8_t bg_prio_cnt = 0;
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			if ((mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + 2 * j) & 3) == i)
				bg_prio[bg_prio_cnt++] = j;
		}
	}
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0x8))
		draw_background_tiled(gpu, y, 0, bg0_data);
	if (display & (1 << 0x9))
		draw_background_tiled(gpu, y, 1, bg1_data);
	if (display & (1 << 0xA))
		draw_background_tiled(gpu, y, 2, bg2_data);
	if (display & (1 << 0xB))
		draw_background_tiled(gpu, y, 3, bg3_data);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y, obj_data);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
	for (size_t x = 0; x < 240; ++x)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			if (bg_data[i][x * 4 + 3])
			{
				memcpy(&gpu->data[(y * 240 + x) * 4], &bg3_data[x * 4], 3);
				break;
			}
		}
		if (obj_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &obj_data[x * 4], 3);
	}
}

static void draw_mode1(gpu_t *gpu, uint8_t y)
{
}

static void draw_mode2(gpu_t *gpu, uint8_t y)
{
	uint8_t bg2_data[240 * 4];
	uint8_t bg3_data[240 * 4];
	uint8_t obj_data[240 * 4];
	memset(bg2_data, 0, sizeof(bg2_data));
	memset(bg3_data, 0, sizeof(bg3_data));
	memset(obj_data, 0, sizeof(obj_data));
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_tiled(gpu, y, 2, bg2_data);
	if (display & (1 << 0xB))
		draw_background_tiled(gpu, y, 3, bg3_data);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y, obj_data);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y);
	for (size_t x = 0; x < 240; ++x)
	{
		//XXX: bg priority
		if (bg3_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &bg3_data[x * 4], 3);
		if (bg2_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &bg2_data[x * 4], 3);
		if (obj_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &obj_data[x * 4], 3);
	}
}

static void draw_mode3(gpu_t *gpu, uint8_t y)
{
	uint8_t bg2_data[240 * 4];
	uint8_t obj_data[240 * 4];
	memset(bg2_data, 0, sizeof(bg2_data));
	memset(obj_data, 0, sizeof(obj_data));
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_3(gpu, y, bg2_data);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, obj_data);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y);
	for (size_t x = 0; x < 240; ++x)
	{
		if (bg2_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &bg2_data[x * 4], 3);
		if (obj_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &obj_data[x * 4], 3);
	}
}

static void draw_mode4(gpu_t *gpu, uint8_t y)
{
	uint8_t bg2_data[240 * 4];
	uint8_t obj_data[240 * 4];
	memset(bg2_data, 0, sizeof(bg2_data));
	memset(obj_data, 0, sizeof(obj_data));
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_4(gpu, y, bg2_data);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, obj_data);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y);
	for (size_t x = 0; x < 240; ++x)
	{
		if (bg2_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &bg2_data[x * 4], 4);
		if (obj_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &obj_data[x * 4], 3);
	}
}

static void draw_mode5(gpu_t *gpu, uint8_t y)
{
	uint8_t bg2_data[240 * 4];
	uint8_t obj_data[240 * 4];
	memset(bg2_data, 0, sizeof(bg2_data));
	memset(obj_data, 0, sizeof(obj_data));
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_5(gpu, y, bg2_data);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, obj_data);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y);
	for (size_t x = 0; x < 240; ++x)
	{
		if (bg2_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &bg2_data[x * 4], 3);
		if (obj_data[x * 4 + 3])
			memcpy(&gpu->data[(y * 240 + x) * 4], &obj_data[x * 4], 3);
	}
}

void gpu_draw(gpu_t *gpu, uint8_t y)
{
	uint16_t bg_col = mem_get_bg_palette(gpu->mem, 0);
	uint8_t bg_color[4] =
	{
		TO8((bg_col >> 0xA) & 0x1F),
		TO8((bg_col >> 0x5) & 0x1F),
		TO8((bg_col >> 0x0) & 0x1F),
		0xFF,
	};
	for (size_t x = 0; x < 240; ++x)
		memcpy(&gpu->data[(240 * y + x) * 4], bg_color, 4);
	uint16_t display = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	if (0)
		printf("display: %d\n", display & 0x7);
	switch (display & 0x7)
	{
		case 0:
			draw_mode0(gpu, y);
			return;
		case 1:
			draw_mode1(gpu, y);
			break;
		case 2:
			draw_mode2(gpu, y);
			return;
		case 3:
			draw_mode3(gpu, y);
			break;
		case 4:
			draw_mode4(gpu, y);
			break;
		case 5:
			draw_mode5(gpu, y);
			break;
		default:
			printf("invalid mode: %x\n", display & 0x7);
			break;
	}
}
