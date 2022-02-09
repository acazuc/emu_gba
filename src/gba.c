#include "gba.h"
#include "mbc.h"
#include "mem.h"
#include "apu.h"
#include "cpu.h"
#include "gpu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

gba_t *gba_new(const void *rom_data, size_t rom_size)
{
	gba_t *gba = calloc(sizeof(*gba), 1);
	if (!gba)
		return NULL;

	gba->mbc = mbc_new(rom_data, rom_size);
	if (!gba->mbc)
		return NULL;

	gba->mem = mem_new(gba, gba->mbc);
	if (!gba->mem)
		return NULL;

	gba->apu = apu_new(gba->mem);
	if (!gba->apu)
		return NULL;

	gba->cpu = cpu_new(gba->mem);
	if (!gba->cpu)
		return NULL;

	gba->gpu = gpu_new(gba->mem);
	if (!gba->gpu)
		return NULL;

	return gba;
}

void gba_del(gba_t *gba)
{
	if (!gba)
		return;
	mbc_del(gba->mbc);
	mem_del(gba->mem);
	apu_del(gba->apu);
	cpu_del(gba->cpu);
	gpu_del(gba->gpu);
	free(gba);
}

static void gba_cycle(gba_t *gba)
{
	cpu_cycle(gba->cpu);
}

void gba_frame(gba_t *gba, uint8_t *video_buf, int16_t *audio_buf, uint32_t joypad)
{
	printf("frame\n");
	for (uint8_t y = 0; y < 160; ++y)
	{
		mem_set_reg16(gba->mem, MEM_REG_DISPSTAT, (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & 0xFFFC) | 0x0);
		mem_set_reg16(gba->mem, MEM_REG_VCOUNT, y);

		if (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & (1 << 5) && y == ((mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) >> 8) & 0xFF))
			mem_set_reg16(gba->mem, MEM_REG_IF, mem_get_reg16(gba->mem, MEM_REG_IF) | (1 << 2));

		/* draw */
		gpu_draw(gba->gpu, y);
		for (size_t i = 0; i < 960; ++i)
			gba_cycle(gba);

		/* hblank */
		mem_set_reg16(gba->mem, MEM_REG_DISPSTAT, (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & 0xFFFC) | 0x2);
		if (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & (1 << 4))
			mem_set_reg16(gba->mem, MEM_REG_IF, mem_get_reg16(gba->mem, MEM_REG_IF) | (1 << 1));

		for (size_t i = 0; i < 272; ++i)
			gba_cycle(gba);
	}

	if (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & (1 << 3))
		mem_set_reg16(gba->mem, MEM_REG_IF, mem_get_reg16(gba->mem, MEM_REG_IF) | (1 << 0));

	for (uint8_t y = 160; y < 228; ++y)
	{
		mem_set_reg16(gba->mem, MEM_REG_DISPSTAT, (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & 0xFFFE) | 0x1);
		mem_set_reg16(gba->mem, MEM_REG_VCOUNT, y);

		if (y == ((mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) >> 8) & 0xFF))
			mem_set_reg16(gba->mem, MEM_REG_IF, mem_get_reg16(gba->mem, MEM_REG_IF) | (1 << 2));

		/* vblank */
		for (size_t i = 0; i < 960; ++i)
			gba_cycle(gba);

		/* hblank */
		mem_set_reg16(gba->mem, MEM_REG_DISPSTAT, (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & 0xFFFE) | 0x3);
		if (mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & (1 << 5) && mem_get_reg16(gba->mem, MEM_REG_DISPSTAT) & (1 << 4))
			mem_set_reg16(gba->mem, MEM_REG_IF, mem_get_reg16(gba->mem, MEM_REG_IF) | (1 << 1));

		for (size_t i = 0; i < 272; ++i)
			gba_cycle(gba);
	}
	memcpy(video_buf, gba->gpu->data, sizeof(gba->gpu->data));
	memset(audio_buf, 0, 804 * 2);
}

void gba_get_mbc_ram(gba_t *gba, uint8_t **data, size_t *size)
{
	*data = NULL;
	*size = 0;
}

void gba_get_mbc_rtc(gba_t *gba, uint8_t **data, size_t *size)
{
	*data = NULL;
	*size = 0;
}
