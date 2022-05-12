#include "gpu.h"
#include "mem.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define TO8(v) (((uint32_t)(v) * 527 + 23) >> 6)

#define RGB5TO8(v, a) \
{ \
	TO8((v >> 0xA) & 0x1F), \
	TO8((v >> 0x5) & 0x1F), \
	TO8((v >> 0x0) & 0x1F), \
a, \
}

#define TRANSFORM_INT16(n) \
do \
{ \
	if (n & (1 << 15)) \
		n = -(n & ~(1 << 15)); \
} while (0)

#define TRANSFORM_INT28(n) \
do \
{ \
	if (n & (1 << 27)) \
		n = -(n & ~(1 << 27)); \
} while (0)

enum layer_type
{
	LAYER_NONE,
	LAYER_BD,
	LAYER_BG0,
	LAYER_BG1,
	LAYER_BG2,
	LAYER_BG3,
	LAYER_OBJ,
};

typedef struct line_buff_s
{
	uint8_t bg0[240 * 4];
	uint8_t bg1[240 * 4];
	uint8_t bg2[240 * 4];
	uint8_t bg3[240 * 4];
	uint8_t obj[240 * 4];
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

static void draw_background_text(gpu_t *gpu, uint8_t y, uint8_t bg, uint8_t *data)
{
	static const uint32_t mapwidths[]  = {32 * 8, 64 * 8, 32 * 8, 64 * 8};
	static const uint32_t mapheights[] = {32 * 8, 32 * 8, 64 * 8, 64 * 8};
	uint16_t bgcnt = mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + bg * 2);
	uint8_t bghofs = mem_get_reg16(gpu->mem, MEM_REG_BG0HOFS + bg * 4) & 0x1FF;
	uint8_t bgvofs = mem_get_reg16(gpu->mem, MEM_REG_BG0VOFS + bg * 4) & 0x1FF;
	uint8_t size = (bgcnt >> 14) & 0x3;
	uint32_t tilebase = ((bgcnt >> 2) & 0x3) * 0x4000;
	uint32_t mapbase = ((bgcnt >> 8) & 0x1F) * 0x800;
	uint32_t mapw = mapwidths[size];
	uint32_t maph = mapheights[size];
	for (int32_t x = 0; x < 240; ++x)
	{
		int32_t vx = (x + bghofs) % mapw;
		int32_t vy = (y + bgvofs) % maph;
		if (vx < 0)
			vx += mapw;
		if (vy < 0)
			vy += maph;
		uint32_t mapx = vx / 8;
		uint32_t mapy = vy / 8;
		uint32_t tilex = vx % 8;
		uint32_t tiley = vy % 8;
		uint32_t mapaddr = mapx + mapy * 32;
		uint16_t map = mem_get_vram16(gpu->mem, mapbase + mapaddr * 2);
		uint16_t tileid = map & 0x3FF;
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
				paladdr >>= 4;
			else
				paladdr &= 0xF;
			if (!paladdr)
				continue;
			paladdr += ((map >> 12) & 0xF) * 0x10;
		}
		uint16_t val = mem_get_bg_palette(gpu->mem, paladdr * 2);
		uint8_t color[4] = RGB5TO8(val, 0xFF);
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_affine(gpu_t *gpu, uint8_t y, uint8_t bg, uint8_t *data)
{
	static const uint32_t mapsizes[]  = {16 * 8, 32 * 8, 64 * 8, 128 * 8};
	uint16_t bgcnt = mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + bg * 2);
	uint8_t size = (bgcnt >> 14) & 0x3;
	uint32_t tilebase = ((bgcnt >> 2) & 0x3) * 0x4000;
	uint32_t mapbase = ((bgcnt >> 8) & 0x1F) * 0x800;
	uint32_t mapsize = mapsizes[size];
	int32_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA + 0x10 * (bg - 2));
	int32_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB + 0x10 * (bg - 2));
	int32_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC + 0x10 * (bg - 2));
	int32_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD + 0x10 * (bg - 2));
	int32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X + 0x10 * (bg - 2)) & 0xFFFFFFF;
	int32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y + 0x10 * (bg - 2)) & 0xFFFFFFF;
	TRANSFORM_INT16(pa);
	TRANSFORM_INT16(pb);
	TRANSFORM_INT16(pc);
	TRANSFORM_INT16(pd);
	TRANSFORM_INT28(bgx);
	TRANSFORM_INT28(bgy);
	for (int32_t x = 0; x < 240; ++x)
	{
		int32_t vx = (((pa * x + pb * y) - bgx) >> 8) % mapsize;
		int32_t vy = (((pc * x + pd * y) - bgy) >> 8) % mapsize;
		if (vx < 0)
			vx += mapsize;
		if (vy < 0)
			vy += mapsize;
		uint32_t mapx = vx / 8;
		uint32_t mapy = vy / 8;
		uint32_t tilex = vx % 8;
		uint32_t tiley = vy % 8;
		uint32_t mapaddr = mapx + mapy * 32;
		uint16_t tileid = mem_get_vram8(gpu->mem, mapbase + mapaddr);
		uint8_t paladdr;
		uint16_t tileaddr = tilebase + tileid * 0x40;
		paladdr = mem_get_vram8(gpu->mem, tileaddr + tilex + tiley * 8);
		if (!paladdr)
			continue;
		uint16_t val = mem_get_bg_palette(gpu->mem, paladdr * 2);
		uint8_t color[4] = RGB5TO8(val, 0xFF);
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_3(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	int32_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	int32_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	int32_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	int32_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	int32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X) & 0xFFFFFFF;
	int32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y) & 0xFFFFFFF;
	TRANSFORM_INT16(pa);
	TRANSFORM_INT16(pb);
	TRANSFORM_INT16(pc);
	TRANSFORM_INT16(pd);
	TRANSFORM_INT28(bgx);
	TRANSFORM_INT28(bgy);
	for (int32_t x = 0; x < 240; ++x)
	{
		int32_t vx = (((pa * x + pb * y) + bgx) >> 8) % 240;
		int32_t vy = (((pc * x + pd * y) + bgy) >> 8) % 160;
		if (vx < 0)
			vx += 240;
		if (vy < 0)
			vy += 160;
		uint32_t addr = 2 * (vx + 240 * vy);
		uint16_t val = mem_get_vram16(gpu->mem, addr);
		uint8_t color[4] = RGB5TO8(val, 0xFF);
		memcpy(&data[x * 4], color, 4);
	}
}

static void draw_background_bitmap_4(gpu_t *gpu, uint8_t y, uint8_t *data)
{
	int32_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	int32_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	int32_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	int32_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	int32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X) & 0xFFFFFFF;
	int32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y) & 0xFFFFFFF;
	TRANSFORM_INT16(pa);
	TRANSFORM_INT16(pb);
	TRANSFORM_INT16(pc);
	TRANSFORM_INT16(pd);
	TRANSFORM_INT28(bgx);
	TRANSFORM_INT28(bgy);
	uint16_t dispcnt = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	uint32_t addr_offset = (dispcnt & (1 << 4)) ? 0xA000 : 0;
	for (int32_t x = 0; x < 240; ++x)
	{
		int32_t vx = (((pa * x + pb * y) - bgx) >> 8) % 240;
		int32_t vy = (((pc * x + pd * y) - bgy) >> 8) % 160;
		if (vx < 0)
			vx += 240;
		if (vy < 0)
			vy += 240;
		uint32_t addr = addr_offset + vx + 240 * vy;
		uint8_t val = mem_get_vram8(gpu->mem, addr);
		if (!val)
			continue;
		uint16_t col = mem_get_bg_palette(gpu->mem, val * 2);
		uint8_t color[4] = RGB5TO8(col, 0xFF);
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
	int32_t pa = mem_get_reg16(gpu->mem, MEM_REG_BG2PA);
	int32_t pb = mem_get_reg16(gpu->mem, MEM_REG_BG2PB);
	int32_t pc = mem_get_reg16(gpu->mem, MEM_REG_BG2PC);
	int32_t pd = mem_get_reg16(gpu->mem, MEM_REG_BG2PD);
	int32_t bgx = mem_get_reg32(gpu->mem, MEM_REG_BG2X) & 0xFFFFFFF;
	int32_t bgy = mem_get_reg32(gpu->mem, MEM_REG_BG2Y) & 0xFFFFFFF;
	TRANSFORM_INT16(pa);
	TRANSFORM_INT16(pb);
	TRANSFORM_INT16(pc);
	TRANSFORM_INT16(pd);
	TRANSFORM_INT28(bgx);
	TRANSFORM_INT28(bgy);
	uint8_t baseaddr = (mem_get_reg32(gpu->mem, MEM_REG_DISPCNT) & (1 << 4)) ? 0xA000 : 0;
	for (int32_t x = 0; x < 160; ++x)
	{
		int32_t vx = (((pa * x + pb * y) - bgx) >> 8) % 160;
		int32_t vy = (((pc * x + pd * y) - bgy) >> 8) % 128;
		if (vx < 0)
			vx += 160;
		if (vy < 0)
			vy += 128;
		uint32_t addr = baseaddr + 2 * (vx + 160 * vy);
		uint16_t val = mem_get_vram16(gpu->mem, addr);
		uint8_t color[4] = RGB5TO8(val, 0xFF);
		memcpy(&data[(40 + x) * 4], color, 4);
	}
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
	for (size_t i = 0; i < 240; ++i)
		data[i * 4 + 3] = 0xE;
	for (uint8_t i = 0; i < 128; ++i)
	{
		uint16_t attr0 = mem_get_oam16(gpu->mem, i * 8);
		if ((attr0 & 0x300) == 0x200) //disable flag
			continue;
		uint8_t mode = (attr0 >> 10) & 0x3;
		if (mode == 3)
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
		uint8_t basewidth = width;
		uint8_t baseheight = height;
		uint8_t doublesize = (attr0 >> 9) & 0x1;
		if (doublesize)
		{
			width *= 2;
			height *= 2;
		}
		if (objy + height <= y)
			continue;
		uint8_t affine = (attr0 >> 8) & 0x1;
		int32_t pa;
		int32_t pb;
		int32_t pc;
		int32_t pd;
		if (affine)
		{
			uint16_t affineidx = ((attr1 >> 9) & 0x1F) * 0x20;
			pa = mem_get_oam16(gpu->mem, affineidx + 0x06);
			pb = mem_get_oam16(gpu->mem, affineidx + 0x0E);
			pc = mem_get_oam16(gpu->mem, affineidx + 0x16);
			pd = mem_get_oam16(gpu->mem, affineidx + 0x1E);
			TRANSFORM_INT16(pa);
			TRANSFORM_INT16(pb);
			TRANSFORM_INT16(pc);
			TRANSFORM_INT16(pd);
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
		uint8_t color_mode = (attr0 >> 13) & 0x1;
		uint8_t priority = (attr2 >> 10) & 0x3;
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
					texx = basewidth - 1 - texx;
				if (attr1 & (1 << 13))
					texy = baseheight - 1 - texy;
			}
			int16_t tilex = texx / 8;
			int16_t tilebx = texx % 8;
			int16_t tiley = texy / 8;
			int16_t tileby = texy % 8;
			uint16_t tilepos = tileid + tilex;
			if (mem_get_reg16(gpu->mem, MEM_REG_DISPCNT) & (1 << 6))
			{
				uint16_t tmp = tiley * basewidth / 4;
				if (!color_mode)
					tmp /= 2;
				tilepos += tmp;
			}
			else
			{
				tilepos += tiley * 32;
			}
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
					tilev >>= 4;
				else
					tilev &= 0xF;
			}
			if (!tilev)
				continue;
			if (!color_mode)
				tilev += palette * 0x10;
			uint16_t col = mem_get_obj_palette(gpu->mem, tilev * 2);
			if (mode == 2)
			{
				if (col)
					data[screenx * 4 + 3] |= 0x40;
				continue;
			}
			if (priority >= ((data[screenx * 4 + 3] >> 1) & 0x7))
				continue;
			uint8_t color[4] = RGB5TO8(col, 0x80 | (mode & 1) | (priority << 1) | (data[screenx * 4 + 3] & 0x40));
			memcpy(&data[screenx * 4], color, 4);
		}
	}
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
		case LAYER_NONE:
			break;
	}
	assert(!"unknown layer");
	return NULL;
}

static void compose(gpu_t *gpu, line_buff_t *line, uint8_t y)
{
	uint16_t bd_col = mem_get_bg_palette(gpu->mem, 0);
	uint8_t bd_color[4] = RGB5TO8(bd_col, 0xFF);
	for (size_t x = 0; x < 240; ++x)
		memcpy(&gpu->data[(240 * y + x) * 4], bd_color, 4);
	uint8_t *bg_data[4] = {&line->bg0[0], &line->bg1[0], &line->bg2[0], &line->bg3[0]};
	uint8_t bg_order[4];
	uint8_t bg_order_cnt = 0;
	uint8_t bg_prio[4];
	for (size_t i = 0; i < 4; ++i)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			uint8_t bgp = mem_get_reg16(gpu->mem, MEM_REG_BG0CNT + 2 * j) & 3;
			if (bgp == i)
			{
				bg_order[bg_order_cnt] = j;
				bg_prio[bg_order_cnt] = bgp;
				bg_order_cnt++;
			}
		}
	}
	uint16_t dispcnt = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	bool has_window = (dispcnt & (7 << 13)) != 0;
	uint16_t bldcnt = mem_get_reg16(gpu->mem, MEM_REG_BLDCNT);
	uint8_t blending = (bldcnt >> 6) & 3;
	uint16_t win0h = mem_get_reg16(gpu->mem, MEM_REG_WIN0H);
	uint16_t win0v = mem_get_reg16(gpu->mem, MEM_REG_WIN0V);
	uint16_t win1h = mem_get_reg16(gpu->mem, MEM_REG_WIN1H);
	uint16_t win1v = mem_get_reg16(gpu->mem, MEM_REG_WIN1V);
	uint16_t winin = mem_get_reg16(gpu->mem, MEM_REG_WININ);
	uint16_t winout = mem_get_reg16(gpu->mem, MEM_REG_WINOUT);
	uint8_t win0l = win0h >> 8;
	uint8_t win0r = win0h & 0xFF;
	uint8_t win0t = win0v >> 8;
	uint8_t win0b = win0v & 0xFF;
	uint8_t win1l = win1h >> 8;
	uint8_t win1r = win1h & 0xFF;
	uint8_t win1t = win1v >> 8;
	uint8_t win1b = win1v & 0xFF;
	uint16_t bldalpha = mem_get_reg16(gpu->mem, MEM_REG_BLDALPHA);
	uint8_t bldy = mem_get_reg16(gpu->mem, MEM_REG_BLDY) & 0x1F;
	uint8_t eva = (bldalpha >> 0) & 0x1F;
	uint8_t evb = (bldalpha >> 8) & 0x1F;
	for (size_t x = 0; x < 240; ++x)
	{
		uint8_t *dst = &gpu->data[(y * 240 + x) * 4];
		uint8_t winflags;
		if (has_window)
		{
			if ((dispcnt & (1 << 13))
			 && x >= win0l && x < win0r
			 && y >= win0t && y < win0b)
				winflags = winin & 0xFF;
			else if ((dispcnt & (1 << 14))
			      && x >= win1l && x < win1r
			      && y >= win1t && y < win1b)
				winflags = winin >> 8;
			else if ((dispcnt & (1 << 15))
			      && (line->obj[x * 4 + 3] & 0x40))
				winflags = winout >> 8;
			else
				winflags = winout & 0xFF;
		}
		else
		{
			winflags = 0xFF;
		}
		uint8_t pixel_blend = blending;
		if (!(winflags & (1 << 5)))
			pixel_blend = 0;
		if (line->obj[x * 4 + 3] & 1)
			pixel_blend = 1;
		switch (pixel_blend)
		{
			case 0:
			{
				enum layer_type layer = LAYER_BD;
				for (size_t i = 0; i < bg_order_cnt; ++i)
				{
					uint8_t bgp = bg_order[i];
					if ((winflags & (1 << bgp)) && bg_data[bgp][x * 4 + 3])
					{
						layer = LAYER_BG0 + bgp;
						break;
					}
				}
				if ((winflags & (1 << 4)) && (line->obj[x * 4 + 3] & 0x80))
					layer = LAYER_OBJ;
				memcpy(dst, layer_data(line, layer, bd_color, x * 4), 3);
				break;
			}
			case 1:
			{
				enum layer_type top_layer = LAYER_NONE;
				enum layer_type bot_layer = LAYER_NONE;
				uint8_t top_priority = 4;
				uint8_t bot_priority = 4;
				if (bldcnt & (1 << 5))
					top_layer = LAYER_BD;
				if (bldcnt & (1 << 13))
					bot_layer = LAYER_BD;
				for (size_t i = 0; i < bg_order_cnt; ++i)
				{
					uint8_t bgp = bg_order[i];
					if (!(winflags & (1 << bgp)))
						continue;
					if ((bldcnt & (1 << bgp)) && bg_data[bgp][x * 4 + 3] && top_layer < LAYER_BG0)
					{
						top_layer = LAYER_BG0 + bgp;
						top_priority = bg_prio[bgp];
						continue;
					}
					if ((bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3] && bot_layer < LAYER_BG0)
					{
						bot_layer = LAYER_BG0 + bgp;
						bot_priority = bg_prio[bgp];
						continue;
					}
				}
				if (winflags & (1 << 4))
				{
					uint8_t obj = line->obj[x * 4 + 3];
					if ((obj & 0x81) == 0x81 || ((bldcnt & (1 << 4)) && (obj & 0x81) == 0x80 && ((obj >> 2) & 3) <= top_priority))
					{
						bot_layer = top_layer;
						top_layer = LAYER_OBJ;
					}
					else if ((bldcnt & (1 << 12)) && (obj & 0x81) == 0x80 && ((obj >> 2) & 3) <= bot_priority)
					{
						bot_layer = LAYER_OBJ;
					}
				}
				if (top_layer != LAYER_NONE)
				{
					if (bot_layer != LAYER_NONE)
					{
						const uint8_t *top_layer_data = layer_data(line, top_layer, bd_color, x * 4);
						const uint8_t *bot_layer_data = layer_data(line, bot_layer, bd_color, x * 4);
						for (size_t i = 0; i < 3; ++i)
						{
							uint16_t res = (top_layer_data[i] * eva + bot_layer_data[i] * evb) >> 4;
							if (res > 0xFF)
								res = 0xFF;
							dst[i] = res;
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
				break;
			}
			case 2:
			{
				enum layer_type layer = LAYER_NONE;
				for (size_t i = 0; i < bg_order_cnt; ++i)
				{
					uint8_t bgp = bg_order[i];
					if ((winflags & (1 << bgp)) && (bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3])
					{
						layer = LAYER_BG0 + bgp;
						break;
					}
				}
				if ((winflags & (1 << 4)) && (bldcnt & (1 << 4)) && (line->obj[x * 4 + 3] & 0x80))
					layer = LAYER_OBJ;
				if (layer != LAYER_NONE)
				{
					const uint8_t *data = layer_data(line, layer, bd_color, x * 4);
					for (size_t i = 0; i < 3; ++i)
						dst[i] = data[i] + (((255 - data[i]) * bldy) >> 4);
				}
				else
				{
					memset(dst, 0xFF, 3);
				}
				break;
			}
			case 3:
			{
				enum layer_type layer = LAYER_NONE;
				for (size_t i = 0; i < bg_order_cnt; ++i)
				{
					uint8_t bgp = bg_order[i];
					if ((winflags & (1 << bgp)) && (bldcnt & (1 << (8 + bgp))) && bg_data[bgp][x * 4 + 3])
					{
						layer = LAYER_BG0 + bgp;
						break;
					}
				}
				if ((winflags & (1 << 4)) && (bldcnt & (1 << 4)) && (line->obj[x * 4 + 3] & 0x80))
					layer = LAYER_OBJ;
				if (layer != LAYER_NONE)
				{
					const uint8_t *data = layer_data(line, layer, bd_color, x * 4);
					for (size_t i = 0; i < 3; ++i)
						dst[i] = data[i] - ((data[i] * bldy) >> 4);
				}
				else
				{
					memset(dst, 0, 3);
				}
				break;
			}
		}
	}
}

void gpu_draw(gpu_t *gpu, uint8_t y)
{
	line_buff_t line;
	memset(&line, 0, sizeof(line));
	uint16_t display = mem_get_reg16(gpu->mem, MEM_REG_DISPCNT);
	uint32_t objbase;
	switch (display & 0x7)
	{
		case 0:
			if (display & (1 << 0x8))
				draw_background_text(gpu, y, 0, line.bg0);
			if (display & (1 << 0x9))
				draw_background_text(gpu, y, 1, line.bg1);
			if (display & (1 << 0xA))
				draw_background_text(gpu, y, 2, line.bg2);
			if (display & (1 << 0xB))
				draw_background_text(gpu, y, 3, line.bg3);
			objbase = 0x10000;
			break;
		case 1:
			if (display & (1 << 0x8))
				draw_background_text(gpu, y, 0, line.bg0);
			if (display & (1 << 0x9))
				draw_background_text(gpu, y, 1, line.bg1);
			if (display & (1 << 0xA))
				draw_background_affine(gpu, y, 2, line.bg2);
			objbase = 0x10000;
			break;
		case 2:
			if (display & (1 << 0xA))
				draw_background_affine(gpu, y, 2, line.bg2);
			if (display & (1 << 0xB))
				draw_background_affine(gpu, y, 3, line.bg3);
			objbase = 0x10000;
			break;
		case 3:
			if (display & (1 << 0xA))
				draw_background_bitmap_3(gpu, y, line.bg2);
			objbase = 0x14000;
			break;
		case 4:
			if (display & (1 << 0xA))
				draw_background_bitmap_4(gpu, y, line.bg2);
			objbase = 0x14000;
			break;
		case 5:
			if (display & (1 << 0xA))
				draw_background_bitmap_5(gpu, y, line.bg2);
			objbase = 0x14000;
			break;
		default:
			printf("invalid mode: %x\n", display & 0x7);
			return;
	}
	if (display & (1 << 0xC))
		draw_objects(gpu, objbase, y, line.obj);
	compose(gpu, &line, y);
}
