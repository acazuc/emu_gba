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

	gba->mem = mem_new(gba->mbc);
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

void gba_frame(gba_t *gba, uint8_t *video_buf, int16_t *audio_buf, uint32_t joypad)
{
	for (size_t i = 0; i < 1/*280896*/; ++i)
		cpu_cycle(gba->cpu);
	memset(video_buf, 0, 240 * 160 * 4);
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
