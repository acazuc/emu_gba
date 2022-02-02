#include "mem.h"
#include "mbc.h"
#include <stdlib.h>
#include <stdio.h>

extern uint8_t _binary_gbabios_bin_start;
extern uint8_t _binary_gbabios_bin_end;

mem_t *mem_new(mbc_t *mbc)
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

	mem->mbc = mbc;
	return mem;
}

void mem_del(mem_t *mem)
{
	if (!mem)
		return;
	free(mem);
}

void *mem_ptr(mem_t *mem, uint32_t addr)
{
	if (addr >= 0x10000000)
		return NULL;
	switch ((addr >> 24) & 0xF)
	{
		case 0x0: //bios
			if (addr < 0x4000)
				return &mem->bios[addr];
			break;
		case 0x1: //empty
			break;
		case 0x2: //board wram
		{
			uint32_t a = addr & 0x3FFFF;
			return &mem->board_wram[a];
		}
		case 0x3: //chip wram
		{
			uint32_t a = addr & 0x7FFF;
			return &mem->chip_wram[a];
		}
		case 0x4: //registers
			if (addr < 0x40003FF)
				return &mem->io_regs[addr - 0x4000000];
			break;
		case 0x5: //palette
			if (addr < 0x5000400)
				return &mem->palette[addr - 0x5000000];
			break;
		case 0x6: //vram
			if (addr < 0x6018000)
				return &mem->vram[addr - 0x6000000];
			break;
		case 0x7: //oam
			if (addr < 0x7000400)
				return &mem->oam[addr - 0x7000000];
			break;
		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB:
		case 0xC:
		case 0xD:
		case 0xE:
		case 0xF:
			return mbc_ptr(mem->mbc, addr);
	}
	printf("unknown addr: %08x\n", addr);
	return NULL;
}

uint8_t mem_get8(mem_t *mem, uint32_t addr)
{
	void *ptr = mem_ptr(mem, addr);
	if (!ptr)
		return 0;
	return *(uint8_t*)ptr;
}

uint16_t mem_get16(mem_t *mem, uint32_t addr)
{
	void *ptr = mem_ptr(mem, addr & ~0x1);
	if (!ptr)
		return 0;
	return *(uint16_t*)ptr;
}

uint32_t mem_get32(mem_t *mem, uint32_t addr)
{
	void *ptr = mem_ptr(mem, addr & ~0x3);
	if (!ptr)
		return 0;
	return *(uint32_t*)ptr;
}

void mem_set8(mem_t *mem, uint32_t addr, uint8_t val)
{
	void *ptr = mem_ptr(mem, addr);
	if (!ptr)
		return;
	*(uint8_t*)ptr = val;
}

void mem_set16(mem_t *mem, uint32_t addr, uint16_t val)
{
	void *ptr = mem_ptr(mem, addr & ~0x1);
	if (!ptr)
		return;
	*(uint16_t*)ptr = val;
}

void mem_set32(mem_t *mem, uint32_t addr, uint32_t val)
{
	void *ptr = mem_ptr(mem, addr & ~0x3);
	if (!ptr)
		return;
	*(uint32_t*)ptr = val;
}
