#include "mem.h"
#include "mbc.h"
#include "gba.h"
#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>

extern uint8_t _binary_gbabios_bin_start;
extern uint8_t _binary_gbabios_bin_end;

mem_t *mem_new(gba_t *gba, mbc_t *mbc)
{
	mem_t *mem = calloc(sizeof(*mem), 1);
	if (!mem)
		return NULL;

	if (&_binary_gbabios_bin_end - &_binary_gbabios_bin_start != 0x4000)
	{
		fprintf(stderr, "invalid dmgbios data: %u\n", (unsigned)(&_binary_gbabios_bin_end - &_binary_gbabios_bin_start));
		return NULL;
	}

	size_t i = 0;
	for (uint8_t *s = &_binary_gbabios_bin_start; s < &_binary_gbabios_bin_end; ++s)
		mem->bios[i++] = *s;

	mem->gba = gba;
	mem->mbc = mbc;
	mem_set_reg32(mem, MEM_REG_SOUNDBIAS, 0x200);
	return mem;
}

void mem_del(mem_t *mem)
{
	if (!mem)
		return;
	free(mem);
}

#define MEM_GET(size) \
uint##size##_t mem_get##size(mem_t *mem, uint32_t addr) \
{ \
	if (size == 16) \
		addr &= ~1; \
	if (size == 32) \
		addr &= ~3; \
	if (addr >= 0x10000000) \
		goto end; \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x0: /* bios */ \
			if (addr < 0x4000) \
				return *(uint##size##_t*)&mem->bios[addr]; \
			break; \
		case 0x1: /* empty */ \
			break; \
		case 0x2: /* board wram */ \
		{ \
			uint32_t a = addr & 0x3FFFF; \
			return *(uint##size##_t*)&mem->board_wram[a]; \
		} \
		case 0x3: /* chip wram */ \
		{ \
			uint32_t a = addr & 0x7FFF; \
			return *(uint##size##_t*)&mem->chip_wram[a]; \
		} \
		case 0x4: /* registers */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			return *(uint##size##_t*)&mem->io_regs[a]; \
		} \
		case 0x5: /* palette */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			return *(uint##size##_t*)&mem->palette[a]; \
		} \
		case 0x6: /* vram */ \
		{ \
			uint32_t a = addr & 0x1FFFF; \
			if (a >= 0x18000) \
				a &= ~0x8000; \
			return *(uint##size##_t*)&mem->vram[a]; \
		} \
		case 0x7: /* oam */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			return *(uint##size##_t*)&mem->oam[a]; \
		} \
		case 0x8: \
		case 0x9: \
		case 0xA: \
		case 0xB: \
		case 0xC: \
		case 0xD: \
		case 0xE: \
		case 0xF: \
			return mbc_get##size(mem->mbc, addr); \
	} \
end: \
	printf("unknown addr: %08x\n", addr); \
	return 0; \
}

MEM_GET(8);
MEM_GET(16);
MEM_GET(32);

#define MEM_SET(size) \
void mem_set##size(mem_t *mem, uint32_t addr, uint##size##_t v) \
{ \
	if (size == 16) \
		addr &= ~1; \
	if (size == 32) \
		addr &= ~3; \
	if (addr >= 0x10000000) \
		goto end; \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x0: /* bios */ \
			break; \
		case 0x1: /* empty */ \
			break; \
		case 0x2: /* board wram */ \
		{ \
			uint32_t a = addr & 0x3FFFF; \
			*(uint##size##_t*)&mem->board_wram[a] = v; \
			return; \
		} \
		case 0x3: /* chip wram */ \
		{ \
			uint32_t a = addr & 0x7FFF; \
			*(uint##size##_t*)&mem->chip_wram[a] = v; \
			return; \
		} \
		case 0x4: /* registers */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			switch (a) \
			{ \
				case MEM_REG_HALTCNT: \
					if (v & 0x80) \
						mem->gba->cpu->state = CPU_STATE_STOP; \
					else \
						mem->gba->cpu->state = CPU_STATE_HALT; \
					return; \
				case MEM_REG_IF: \
					*(uint##size##_t*)&mem->io_regs[a] &= ~v; \
					return; \
			} \
			*(uint##size##_t*)&mem->io_regs[a] = v; \
			return; \
		} \
		case 0x5: /* palette */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			*(uint##size##_t*)&mem->palette[a] = v; \
			return; \
		} \
		case 0x6: /* vram */ \
		{ \
			uint32_t a = addr & 0x1FFFF; \
			if (a >= 0x18000) \
				a &= ~0x8000; \
			*(uint##size##_t*)&mem->vram[a] = v; \
			return; \
		} \
		case 0x7: /* oam */ \
		{ \
			uint32_t a = addr & 0x3FF; \
			*(uint##size##_t*)&mem->oam[a] = v; \
			return; \
		} \
		case 0x8: \
		case 0x9: \
		case 0xA: \
		case 0xB: \
		case 0xC: \
		case 0xD: \
		case 0xE: \
		case 0xF: \
			mbc_set##size(mem->mbc, addr, v); \
			return; \
	} \
end: \
	printf("unknown addr: %08x\n", addr); \
}

MEM_SET(8);
MEM_SET(16);
MEM_SET(32);
