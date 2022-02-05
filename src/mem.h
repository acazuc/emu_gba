#ifndef MEM_H
#define MEM_H

#include <stdint.h>

#define MEM_REG_DISPCNT     0x000
#define MEM_REG_DISPSTAT    0x004
#define MEM_REG_VCOUNT      0x006
#define MEM_REG_BG0CNT      0x008
#define MEM_REG_BG1CNT      0x00A
#define MEM_REG_BG2CNT      0x00C
#define MEM_REG_BG3CNT      0x00E
#define MEM_REG_BG0HOFS     0x010
#define MEM_REG_BG0VOFS     0x012
#define MEM_REG_BG1HOFS     0x014
#define MEM_REG_BG1VOFS     0x016
#define MEM_REG_BG2HOFS     0x018
#define MEM_REG_BG2VOFS     0x01A
#define MEM_REG_BG3HOFS     0x01C
#define MEM_REG_BG3VOFS     0x01E
#define MEM_REG_BG2PA       0x020
#define MEM_REG_BG2PB       0x022
#define MEM_REG_BG2PC       0x024
#define MEM_REG_BG2PD       0x026
#define MEM_REG_BG2X        0x028
#define MEM_REG_BG2Y        0x02C
#define MEM_REG_BG3PA       0x030
#define MEM_REG_BG3PB       0x032
#define MEM_REG_BG3PC       0x034
#define MEM_REG_BG3PD       0x036
#define MEM_REG_BG3X        0x038
#define MEM_REG_BG3Y        0x03C
#define MEM_REG_WIN0H       0x040
#define MEM_REG_WIN1H       0x042
#define MEM_REG_WIN0V       0x044
#define MEM_REG_WIN1V       0x046
#define MEM_REG_WININ       0x048
#define MEM_REG_WINOUT      0x04A
#define MEM_REG_MOSAIC      0x04C
#define MEM_REG_BLDCNT      0x050
#define MEM_REG_BLDALPHA    0x052
#define MEM_REG_BLDY        0x054

#define MEM_REG_SOUND1CNT_L 0x060
#define MEM_REG_SOUND1CNT_H 0x062
#define MEM_REG_SOUND1CNT_X 0x064
#define MEM_REG_SOUND2CNT_L 0x068
#define MEM_REG_SOUND2CNT_H 0x06C
#define MEM_REG_SOUND3CNT_L 0x070
#define MEM_REG_SOUND3CNT_H 0x072
#define MEM_REG_SOUND3CNT_X 0x074
#define MEM_REG_SOUND4CNT_L 0x078
#define MEM_REG_SOUND4CNT_H 0x07C
#define MEM_REG_SOUNDCNT_L  0x080
#define MEM_REG_SOUNDCNT_H  0x082
#define MEM_REG_SOUNDCNT_X  0x084
#define MEM_REG_SOUNDBIAS   0x088
#define MEM_REG_WAVE_RAM    0x090
#define MEM_REG_FIFO_A      0x0A0
#define MEM_REG_FIFO_B      0x0A4

#define MEM_REG_IE          0x200
#define MEM_REG_IF          0x202
#define MEM_REG_WAITCNT     0x204
#define MEM_REG_IME         0x208
#define MEM_REG_POSTFLG     0x300
#define MEM_REG_HALTCNT     0x301

typedef struct mbc_s mbc_t;
typedef struct gba_s gba_t;

typedef struct mem_s
{
	gba_t *gba;
	mbc_t *mbc;
	uint8_t bios[0x4000];
	uint8_t board_wram[0x40000];
	uint8_t chip_wram[0x8000];
	uint8_t io_regs[0x400];
	uint8_t palette[0x400];
	uint8_t vram[0x18000];
	uint8_t oam[0x400];
} mem_t;

mem_t *mem_new(gba_t *gba, mbc_t *mbc);
void mem_del(mem_t *mem);

uint8_t  mem_get8 (mem_t *mem, uint32_t addr);
uint16_t mem_get16(mem_t *mem, uint32_t addr);
uint32_t mem_get32(mem_t *mem, uint32_t addr);
void mem_set8 (mem_t *mem, uint32_t addr, uint8_t val);
void mem_set16(mem_t *mem, uint32_t addr, uint16_t val);
void mem_set32(mem_t *mem, uint32_t addr, uint32_t val);

static inline uint32_t mem_get_reg32(mem_t *mem, uint32_t reg)
{
	return *(uint32_t*)&mem->io_regs[reg];
}

static inline void mem_set_reg32(mem_t *mem, uint32_t reg, uint32_t v)
{
	*(uint32_t*)&mem->io_regs[reg] = v;
}

static inline uint16_t mem_get_reg16(mem_t *mem, uint32_t reg)
{
	return *(uint16_t*)&mem->io_regs[reg];
}

static inline void mem_set_reg16(mem_t *mem, uint32_t reg, uint16_t v)
{
	*(uint16_t*)&mem->io_regs[reg] = v;
}

static inline uint16_t mem_get_oam16(mem_t *mem, uint32_t addr)
{
	return mem->oam[addr];
}

#endif
