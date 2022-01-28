#ifndef MEM_H
#define MEM_H

#include <stdint.h>

typedef struct mbc_s mbc_t;

typedef struct mem_s
{
	mbc_t *mbc;
	uint8_t bios[0x4000];
	uint8_t board_wram[0x40000];
	uint8_t chip_wram[0x800];
	uint8_t io_regs[0x3FF];
	uint8_t palette[0x400];
	uint8_t vram[0x1800];
	uint8_t oam[0x400];
} mem_t;

mem_t *mem_new(mbc_t *mbc);
void mem_del(mem_t *mem);

uint8_t  mem_get8 (mem_t *mem, uint32_t addr);
uint16_t mem_get16(mem_t *mem, uint32_t addr);
uint32_t mem_get32(mem_t *mem, uint32_t addr);
void mem_set8 (mem_t *mem, uint32_t addr, uint8_t val);
void mem_set16(mem_t *mem, uint32_t addr, uint16_t val);
void mem_set32(mem_t *mem, uint32_t addr, uint32_t val);

#endif
