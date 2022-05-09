#include "gpu.h"
#include "mem.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define TO8(v) (((uint32_t)(v) * 527 + 23) >> 6)

#define RGB5TO8(v) \
{ \
	TO8((v >> 0xA) & 0x1F), \
	TO8((v >> 0x5) & 0x1F), \
	TO8((v >> 0x0) & 0x1F), \
	0xFF, \
}

enum layer_type
{
	LAYER_NONE,
	LAYER_BD,
	LAYER_BG0,
	LAYER_BG1,
	LAYER_BG2,
	LAYER_BG3,
	LAYER_OBJ,
	LAYER_WN0,
	LAYER_WN1,
	LAYER_WNO,
};

typedef struct line_buff_s
{
	uint8_t bg0[240 * 4];
	uint8_t bg1[240 * 4];
	uint8_t bg2[240 * 4];
	uint8_t bg3[240 * 4];
	uint8_t obj[240 * 4];
	uint8_t wn0[240 * 4];
	uint8_t wn1[240 * 4];
	uint8_t wno[240 * 4];
} line_buff_t;

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
	static const uint32_t mapwidths[]  = {32, 64, 32, 64};
	static const uint32_t mapheights[] = {32, 32, 64, 64};
	uint16_t bgcnt = mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + bg * 2);
	uint8_t bghofs = mem_get_reg16(gpu->mem, MEM_REG_BG0HOFS + bg * 4) & 0x1FF;
	uint8_t bgvofs = mem_get_reg16(gpu->mem, MEM_REG_BG0VOFS + bg * 4) & 0x1FF;
	uint8_t size = (bgcnt >> 14) & 0x3;
	uint32_t tilebase = ((bgcnt >> 2) & 0x3) * 0x4000;
	uint32_t mapbase = ((bgcnt >> 8) & 0x1F) * 0x800;
	uint32_t mapw = mapwidths[size];
	uint32_t maph = mapheights[size];
	for (size_t x = 0; x < 240; ++x)
	{
		uint32_t diffx = (x + bghofs) % (mapw * 8);
		uint32_t diffy = (y + bgvofs) % (maph * 8);
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
			if (!paladdr)
				continue;
		}
		else
		{
			tileaddr += tileid * 0x20;
			paladdr = mem_get_vram8(gpu->mem, tileaddr + tilex / 2 + tiley * 4);
			if (tilex & 1)
				paladdr = paladdr >> 4;
			else
				paladdr = paladdr & 0xF;
			if (!paladdr)
				continue;
			paladdr += ((map >> 12) & 0xF) * 0x10;
		}
		uint16_t val = mem_get_bg_palette(gpu->mem, paladdr * 2);
		uint8_t color[4] = RGB5TO8(val);
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_3(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	uint16_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	uint16_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	uint16_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	uint16_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	uint32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X);
	uint32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y);
	for (size_t x = 0; x < 240; ++x)
	{
		int32_t dx = x - bgx;
		int32_t dy = y - bgy;
		uint32_t vx = ((pa * dx + pb * dy) >> 8) + bgx;
		uint32_t vy = ((pc * dx + pd * dy) >> 8) + bgy;
		uint32_t addr = 2 * (x + 240 * y);
		uint16_t val = mem_get_vram16(gpu->mem, addr);
		uint8_t color[4] = RGB5TO8(val);
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
		uint8_t color[4] = RGB5TO8(col);
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_5(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	if (y < 16 || y > 143)
	{
		memset(&data[0], 0, 4 * 240);
		return;
	}
	memset(&data[0], 0, 4 * 40);
	memset(&data[200 * 4], 0, 4 * 40);
	uint16_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	uint16_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	uint16_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	uint16_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	uint32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X);
	uint32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y);
	uint8_t baseaddr = (mem_get_reg32(gpu->mem, MEM_REG_DISPCNT) & (1 << 4)) ? 0xA000 : 0;
	for (size_t x = 0; x < 160; ++x)
	{
		int32_t dx = x - bgx;
		int32_t dy = y - bgy;
		uint32_t vx = ((pa * dx + pb * dy) >> 8) + bgx;
		uint32_t vy = ((pc * dx + pd * dy) >> 8) + bgy;
		uint32_t addr = baseaddr + 2 * (x + 160 * y);
		uint16_t val = mem_get_vram16(gpu->mem, addr);
		uint8_t color[4] = RGB5TO8(val);
		memcpy(&data[(40 + x) * 4], color, 4);
	}
}

static void draw_window(gpu_t *gpu, uint8_t y, uint8_t win, uint8_t *data)
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

	for (int16_t i = 127; i >= 0; --i)
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
		uint8_t palette = (attr2 >> 12) & 0xF;
		uint8_t mode = (attr0 >> 10) & 0x3;
		uint8_t color_mode = (attr0 >> 13) & 0x1;
		if (mem_get_reg16(gpu->mem, MEM_REG_DISPCNT) & (1 << 6))
		{
			//XXX
		}
		else
		{
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
					if (attr1 & (1 << 12))
						xpos = width - 1 - xpos;
					if (attr1 & (1 << 13))
						ypos = height - 1 - ypos;
				}
				int16_t tilex = texx / 8;
				int16_t tilebx = texx % 8;
				int16_t tiley = texy / 8;
				int16_t tileby = texy % 8;
				uint16_t tilepos = tileid + tilex + 32 * tiley;
				if (color_mode)
					tilepos += tilex;
				tilepos &= 0x3FF;
				uint32_t tileoff = tileby * 0x8 + tilebx;
				if (!color_mode)
					tileoff /= 2;
				uint8_t tilev = mem_get_vram8(gpu->mem, tileaddr + tilepos * 0x20 + tileoff);
				if (!color_mode)
				{
					if (tilebx & 1)
						tilev = tilev >> 4;
					else
						tilev = tilev & 0xF;
				}
				if (!tilev)
					continue;
				if (!color_mode)
					tilev += palette * 0x10;
				uint16_t col = mem_get_obj_palette(gpu->mem, tilev * 2);
				uint8_t color[4] =
				{
					TO8((col >> 0xA) & 0x1F),
					TO8((col >> 0x5) & 0x1F),
					TO8((col >> 0x0) & 0x1F),
					0x80 | mode,
				};
				memcpy(&data[screenx * 4], color, 4);
			}
		}
	}
}

static void draw_window_obj(gpu_t *gpu, uint8_t y, uint8_t *data)
{
}

static void draw_mode0(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0x8))
		draw_background_tiled(gpu, y, 0, line->bg0);
	if (display & (1 << 0x9))
		draw_background_tiled(gpu, y, 1, line->bg1);
	if (display & (1 << 0xA))
		draw_background_tiled(gpu, y, 2, line->bg2);
	if (display & (1 << 0xB))
		draw_background_tiled(gpu, y, 3, line->bg3);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static void draw_mode1(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0x8))
		draw_background_tiled(gpu, y, 0, line->bg0);
	if (display & (1 << 0x9))
		draw_background_tiled(gpu, y, 1, line->bg1);
	if (display & (1 << 0xA))
		draw_background_tiled(gpu, y, 2, line->bg2);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static void draw_mode2(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_tiled(gpu, y, 2, line->bg2);
	if (display & (1 << 0xB))
		draw_background_tiled(gpu, y, 3, line->bg3);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x10000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static void draw_mode3(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_3(gpu, y, line->bg2);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static void draw_mode4(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_4(gpu, y, line->bg2);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static void draw_mode5(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint32_t display = mem_get_reg32(gpu->mem, MEM_REG_DISPCNT);
	if (display & (1 << 0xA))
		draw_background_bitmap_5(gpu, y, line->bg2);
	if (display & (1 << 0xC))
		draw_objects(gpu, 0x14000, y, line->obj);
	if (display & (1 << 0xD))
		draw_window(gpu, y, 0, line->wn0);
	if (display & (1 << 0xE))
		draw_window(gpu, y, 1, line->wn1);
	if (display & (1 << 0xF))
		draw_window_obj(gpu, y, line->wno);
}

static const uint8_t *layer_data(line_buff_t *line, enum layer_type layer, const uint8_t *bd_color, uint32_t n)
{
	switch (layer)
	{
		case LAYER_BD:
			return bd_color;
		case LAYER_BG0:
			return &line->bg0[n];
		case LAYER_BG1:
			return &line->bg1[n];
		case LAYER_BG2:
			return &line->bg2[n];
		case LAYER_BG3:
			return &line->bg3[n];
		case LAYER_OBJ:
			return &line->obj[n];
		case LAYER_WN0:
			return &line->wn0[n];
		case LAYER_WN1:
			return &line->wn1[n];
		case LAYER_WNO:
			return &line->wno[n];
		case LAYER_NONE:
			break;
	}
	assert(!"unknown layer");
	return NULL;
}

static void compose(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint16_t bd_col = mem_get_bg_palette(gpu->mem, 0);
	uint8_t bd_color[4] = RGB5TO8(bd_col);
	for (size_t x = 0; x < 240; ++x)
		memcpy(&gpu->data[(240 * y + x) * 4], bd_color, 4);
	uint8_t *bg_data[4] = {&line->bg0[0], &line->bg1[0], &line->bg2[0], &line->bg3[0]};
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
	uint16_t bldcnt = mem_get_reg16(gpu->mem, MEM_REG_BLDCNT);
	switch ((bldcnt >> 6) & 3)
	{
		case 0:
		{
			for (size_t x = 0; x < 240; ++x)
			{
				enum layer_type layer = LAYER_BD;
				for (size_t i = 0; i < bg_prio_cnt; ++i)
				{
					if (bg_data[bg_prio[i]][x * 4 + 3])
					{
						layer = LAYER_BG0 + bg_prio[i];
						break;
					}
				}
				if (line->obj[x * 4 + 3])
					layer = LAYER_OBJ;
				memcpy(&gpu->data[(y * 240 + x) * 4], layer_data(line, layer, bd_color, x * 4), 3);
			}
			break;
		}
		case 1:
		{
			uint16_t bldalpha = mem_get_reg16(gpu->mem, MEM_REG_BLDALPHA);
			uint8_t eva = (bldalpha >> 0) & 0x1F;
			uint8_t evb = (bldalpha >> 8) & 0x1F;
			if (eva >= 0x10)
				eva = 0x10;
			if (evb >= 0x10)
				evb = 0x10;
			for (size_t x = 0; x < 240; ++x)
			{
				uint8_t *dst = &gpu->data[(y * 240 + x) * 4];
				enum layer_type top_layer = LAYER_NONE;
				enum layer_type bot_layer = LAYER_NONE;
				if (bldcnt & (1 << 5))
					top_layer = LAYER_BD;
				if (bldcnt & (1 << 13))
					bot_layer = LAYER_BD;
				for (size_t i = 0; i < bg_prio_cnt; ++i)
				{
					uint8_t bgp = bg_prio[i];
					if ((bldcnt & (1 << bgp)) && bg_data[bgp][x * 4 + 3] && top_layer < LAYER_BG0)
					{
						top_layer = LAYER_BG0 + bgp;
						continue;
					}
					if ((bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3] && bot_layer < LAYER_BG0)
						bot_layer = LAYER_BG0 + bgp;
				}
				if (line->obj[x * 4 + 3] == 0x81 || ((bldcnt & (1 << 4)) && line->obj[x * 4 + 3] == 0x80))
					top_layer = LAYER_OBJ;
				if ((bldcnt & (1 << 12)) && line->obj[x * 4 + 3] == 0x80)
					bot_layer = LAYER_OBJ;
				if (top_layer != LAYER_NONE)
				{
					if (bot_layer != LAYER_NONE)
					{
						const uint8_t *top_layer_data = layer_data(line, top_layer, bd_color, x * 4);
						const uint8_t *bot_layer_data = layer_data(line, bot_layer, bd_color, x * 4);
						for (size_t i = 0; i < 3; ++i)
						{
							dst[i] = (top_layer_data[i] * eva
							       +  bot_layer_data[i] * evb) >> 4;
						}
					}
					else
					{
						memcpy(dst, layer_data(line, top_layer, bd_color, x * 4), 3);
					}
				}
				else if (bot_layer != LAYER_NONE)
				{
					memcpy(dst, layer_data(line, bot_layer, bd_color, x * 4), 3);
				}
				else
				{
					memcpy(dst, bd_color, 4);
				}
			}
			break;
		}
		case 2:
		{
			uint8_t fac = mem_get_reg16(gpu->mem, MEM_REG_BLDY) & 0x1F;
			for (size_t x = 0; x < 240; ++x)
			{
				uint8_t color[4] = {0};
				for (size_t i = 0; i < bg_prio_cnt; ++i)
				{
					uint8_t bgp = bg_prio[i];
					if ((bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3])
					{
						memcpy(color, &bg_data[bgp][x * 4], 4);
						break;
					}
				}
				if ((bldcnt & (1 << 4)) && line->obj[x * 4 + 3])
					memcpy(color, &line->obj[x * 4], 4);
				if (color[3])
				{
					for (size_t i = 0; i < 3; ++i)
						gpu->data[(y * 240 + x) * 4 + i] = color[i] + (((31 - fac) * color[i]) >> 4);
				}
			}
			break;
		}
		case 3:
		{
			uint8_t fac = mem_get_reg16(gpu->mem, MEM_REG_BLDY) & 0x1F;
			for (size_t x = 0; x < 240; ++x)
			{
				uint8_t color[4] = {0};
				for (size_t i = 0; i < bg_prio_cnt; ++i)
				{
					uint8_t bgp = bg_prio[i];
					if ((bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3])
					{
						memcpy(color, &bg_data[bgp][x * 4], 4);
						break;
					}
				}
				if ((bldcnt & (1 << 4)) && line->obj[x * 4 + 3])
					memcpy(color, &line->obj[x * 4], 4);
				if (color[3])
				{
					for (size_t i = 0; i < 3; ++i)
						gpu->data[(y * 240 + x) * 4 + i] = color[i] - ((fac * color[i]) >> 4);
				}
			}
			break;
		}
	}
}

void gpu_draw(gpu_t *gpu, uint8_t y)
{
	line_buff_t line;
	memset(&line, 0, sizeof(line));
	uint16_t display = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	switch (display & 0x7)
	{
		case 0:
			draw_mode0(gpu, &line, y);
			break;
		case 1:
			draw_mode1(gpu, &line, y);
			break;
		case 2:
			draw_mode2(gpu, &line, y);
			break;
		case 3:
			draw_mode3(gpu, &line, y);
			break;
		case 4:
			draw_mode4(gpu, &line, y);
			break;
		case 5:
			draw_mode5(gpu, &line, y);
			break;
		default:
			printf("invalid mode: %x\n", display & 0x7);
			return;
	}
	compose(gpu, &line, y);
}
