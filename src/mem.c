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
		fprintf(stderr, "invalid gbabios data: %u\n", (unsigned)(&_binary_gbabios_bin_end - &_binary_gbabios_bin_start));
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

void mem_timers(mem_t *mem)
{
	static const uint16_t masks[4] = {0, 0x3F, 0xFF, 0x3FF};
	for (size_t i = 0; i < 4; ++i)
	{
		uint8_t cnt_h = mem_get_reg8(mem, MEM_REG_TM0CNT_H + i * 4);
		if (!(cnt_h & (1 << 7)))
			continue;
		if ((mem->gba->cycle & masks[cnt_h & 3]))
			continue;
		mem->timers[i].v++;
		if (!mem->timers[i].v)
		{
			mem->timers[i].v = mem_get_reg16(mem, MEM_REG_TM0CNT_L + i * 4);
			if (cnt_h & (1 << 6))
				mem_set_reg16(mem, MEM_REG_IF, mem_get_reg16(mem, MEM_REG_IF) | (1 << (3 + i)));
		}
	}
}

void mem_dma(mem_t *mem)
{
	for (size_t i = 0; i < 4; ++i)
	{
		if (!mem->dma[i].enabled)
			continue;
		uint16_t cnt_h = mem_get_reg16(mem, MEM_REG_DMA0CNT_H + 0xC * i);
		uint32_t step;
		if (cnt_h & (1 << 10))
		{
			mem_set32(mem, mem->dma[i].dst, mem_get32(mem, mem->dma[i].src));
			step = 4;
		}
		else
		{
			mem_set16(mem, mem->dma[i].dst, mem_get16(mem, mem->dma[i].src));
			step = 2;
		}
		switch ((cnt_h >> 5) & 3)
		{
			case 0:
				mem->dma[i].dst += step;
				break;
			case 1:
				mem->dma[i].dst -= step;
				break;
			case 2:
				break;
			case 3:
				mem->dma[i].dst += step;
				//XXX reload
				break;
		}
		switch ((cnt_h >> 7) & 3)
		{
			case 0:
				mem->dma[i].src += step;
				break;
			case 1:
				mem->dma[i].src -= step;
				break;
			case 2:
				break;
			case 3:
				break;
		}
		mem->dma[i].len--;
		if (!mem->dma[i].len)
		{
			mem->dma[i].enabled = false;
			mem_set_reg16(mem, MEM_REG_DMA0CNT_H + 0xC * i, mem_get_reg16(mem, MEM_REG_DMA0CNT_H + 0xC * i) & ~(1 << 15));
			if (cnt_h & (1 << 14))
				mem_set_reg16(mem, MEM_REG_IF, mem_get_reg16(mem, MEM_REG_IF) | (1 << (8 + i)));
		}
	}
}

static void dma_control(mem_t *mem, uint8_t dma)
{
	static uint32_t len_max[4] = {0x4000, 0x4000, 0x4000, 0x10000};
	mem->dma[dma].src = mem_get_reg32(mem, MEM_REG_DMA0SAD + 0xC * dma);
	mem->dma[dma].dst = mem_get_reg32(mem, MEM_REG_DMA0DAD + 0xC * dma);
	mem->dma[dma].len = mem_get_reg16(mem, MEM_REG_DMA0CNT_L + 0xC * dma) - 1;
	if (mem->dma[dma].len)
	{
		if (mem->dma[dma].len > len_max[dma])
			mem->dma[dma].len = len_max[dma];
	}
	else
	{
		mem->dma[dma].len = len_max[dma];
	}
	uint16_t cnt_h = mem_get_reg16(mem, MEM_REG_DMA0CNT_H + 0xC * dma);
	mem->dma[dma].enabled = cnt_h & (1 << 15);
	//printf("%s DMA of %x bytes from %x to %x\n", mem->dma[dma].enabled ? "starting" : "preparing", mem->dma[dma].len, mem->dma[dma].src, mem->dma[dma].dst);
	if (mem->dma[dma].dst == 0x40000a0 || mem->dma[dma].dst == 0x40000a4)
		return;
}

static void timer_control(mem_t *mem, uint8_t timer, uint8_t v)
{
	uint8_t prev = mem_get_reg8(mem, MEM_REG_TM0CNT_H + timer * 4);
	mem_set_reg8(mem, MEM_REG_TM0CNT_H + timer * 4, v);
	if ((v & (1 << 7)) && !(prev & (1 << 7)))
		mem->timers[timer].v = mem_get_reg16(mem, MEM_REG_TM0CNT_L);
}

static void set_reg(mem_t *mem, uint32_t reg, uint8_t v)
{
	switch (reg)
	{
		case MEM_REG_HALTCNT:
			if (v & 0x80)
				mem->gba->cpu->state = CPU_STATE_STOP;
			else
				mem->gba->cpu->state = CPU_STATE_HALT;
			return;
		case MEM_REG_IF:
		case MEM_REG_IF + 1:
			mem->io_regs[reg] &= ~v;
			return;
		case MEM_REG_DMA0CNT_H + 1:
			mem->io_regs[reg] = v;
			dma_control(mem, 0);
			return;
		case MEM_REG_DMA1CNT_H + 1:
			mem->io_regs[reg] = v;
			dma_control(mem, 1);
			return;
		case MEM_REG_DMA2CNT_H + 1:
			mem->io_regs[reg] = v;
			dma_control(mem, 2);
			return;
		case MEM_REG_DMA3CNT_H + 1:
			mem->io_regs[reg] = v;
			dma_control(mem, 3);
			return;
		case MEM_REG_TM0CNT_H:
			timer_control(mem, 0, v);
			return;
		case MEM_REG_TM1CNT_H:
			timer_control(mem, 1, v);
			return;
		case MEM_REG_TM2CNT_H:
			timer_control(mem, 2, v);
			return;
		case MEM_REG_TM3CNT_H:
			timer_control(mem, 3, v);
			return;
		case MEM_REG_KEYINPUT:
		case MEM_REG_KEYINPUT + 1:
			return;
		case MEM_REG_DMA0SAD:
		case MEM_REG_DMA0SAD + 1:
		case MEM_REG_DMA0SAD + 2:
		case MEM_REG_DMA0SAD + 3:
		case MEM_REG_DMA0DAD:
		case MEM_REG_DMA0DAD + 1:
		case MEM_REG_DMA0DAD + 2:
		case MEM_REG_DMA0DAD + 3:
		case MEM_REG_DMA0CNT_L:
		case MEM_REG_DMA0CNT_L + 1:
		case MEM_REG_DMA1SAD:
		case MEM_REG_DMA1SAD + 1:
		case MEM_REG_DMA1SAD + 2:
		case MEM_REG_DMA1SAD + 3:
		case MEM_REG_DMA1DAD:
		case MEM_REG_DMA1DAD + 1:
		case MEM_REG_DMA1DAD + 2:
		case MEM_REG_DMA1DAD + 3:
		case MEM_REG_DMA1CNT_L:
		case MEM_REG_DMA1CNT_L + 1:
		case MEM_REG_DMA2SAD:
		case MEM_REG_DMA2SAD + 1:
		case MEM_REG_DMA2SAD + 2:
		case MEM_REG_DMA2SAD + 3:
		case MEM_REG_DMA2DAD:
		case MEM_REG_DMA2DAD + 1:
		case MEM_REG_DMA2DAD + 2:
		case MEM_REG_DMA2DAD + 3:
		case MEM_REG_DMA2CNT_L:
		case MEM_REG_DMA2CNT_L + 1:
		case MEM_REG_DMA3SAD:
		case MEM_REG_DMA3SAD + 1:
		case MEM_REG_DMA3SAD + 2:
		case MEM_REG_DMA3SAD + 3:
		case MEM_REG_DMA3DAD:
		case MEM_REG_DMA3DAD + 1:
		case MEM_REG_DMA3DAD + 2:
		case MEM_REG_DMA3DAD + 3:
		case MEM_REG_DMA3CNT_L:
		case MEM_REG_DMA3CNT_L  +1:
		case MEM_REG_DISPCNT:
		case MEM_REG_DISPCNT + 1:
		case MEM_REG_DISPSTAT:
		case MEM_REG_DISPSTAT + 1:
		case MEM_REG_VCOUNT:
		case MEM_REG_VCOUNT + 1:
		case MEM_REG_BG0CNT:
		case MEM_REG_BG0CNT + 1:
		case MEM_REG_BG1CNT:
		case MEM_REG_BG1CNT + 1:
		case MEM_REG_BG2CNT:
		case MEM_REG_BG2CNT + 1:
		case MEM_REG_BG3CNT:
		case MEM_REG_BG3CNT + 1:
		case MEM_REG_BG0HOFS:
		case MEM_REG_BG0HOFS + 1:
		case MEM_REG_BG0VOFS:
		case MEM_REG_BG0VOFS + 1:
		case MEM_REG_BG1HOFS:
		case MEM_REG_BG1HOFS + 1:
		case MEM_REG_BG1VOFS:
		case MEM_REG_BG1VOFS + 1:
		case MEM_REG_BG2HOFS:
		case MEM_REG_BG2HOFS + 1:
		case MEM_REG_BG2VOFS:
		case MEM_REG_BG2VOFS + 1:
		case MEM_REG_BG3HOFS:
		case MEM_REG_BG3HOFS + 1:
		case MEM_REG_BG3VOFS:
		case MEM_REG_BG3VOFS + 1:
		case MEM_REG_BG2PA:
		case MEM_REG_BG2PA + 1:
		case MEM_REG_BG2PB:
		case MEM_REG_BG2PB + 1:
		case MEM_REG_BG2PC:
		case MEM_REG_BG2PC + 1:
		case MEM_REG_BG2PD:
		case MEM_REG_BG2PD + 1:
		case MEM_REG_BG3PA:
		case MEM_REG_BG3PA + 1:
		case MEM_REG_BG3PB:
		case MEM_REG_BG3PB + 1:
		case MEM_REG_BG3PC:
		case MEM_REG_BG3PC + 1:
		case MEM_REG_BG3PD:
		case MEM_REG_BG3PD + 1:
		case MEM_REG_BG2X:
		case MEM_REG_BG2X + 1:
		case MEM_REG_BG2X + 2:
		case MEM_REG_BG2X + 3:
		case MEM_REG_BG2Y:
		case MEM_REG_BG2Y + 1:
		case MEM_REG_BG2Y + 2:
		case MEM_REG_BG2Y + 3:
		case MEM_REG_BG3X:
		case MEM_REG_BG3X + 1:
		case MEM_REG_BG3X + 2:
		case MEM_REG_BG3X + 3:
		case MEM_REG_BG3Y:
		case MEM_REG_BG3Y + 1:
		case MEM_REG_BG3Y + 2:
		case MEM_REG_BG3Y + 3:
		case MEM_REG_WAVE_RAM:
		case MEM_REG_WAVE_RAM + 0x1:
		case MEM_REG_WAVE_RAM + 0x2:
		case MEM_REG_WAVE_RAM + 0x3:
		case MEM_REG_WAVE_RAM + 0x4:
		case MEM_REG_WAVE_RAM + 0x5:
		case MEM_REG_WAVE_RAM + 0x6:
		case MEM_REG_WAVE_RAM + 0x7:
		case MEM_REG_WAVE_RAM + 0x8:
		case MEM_REG_WAVE_RAM + 0x9:
		case MEM_REG_WAVE_RAM + 0xA:
		case MEM_REG_WAVE_RAM + 0xB:
		case MEM_REG_WAVE_RAM + 0xC:
		case MEM_REG_WAVE_RAM + 0xD:
		case MEM_REG_WAVE_RAM + 0xE:
		case MEM_REG_WAVE_RAM + 0xF:
		case MEM_REG_WAVE_RAM + 0x10:
		case MEM_REG_WAVE_RAM + 0x11:
		case MEM_REG_WAVE_RAM + 0x12:
		case MEM_REG_WAVE_RAM + 0x13:
		case MEM_REG_WAVE_RAM + 0x14:
		case MEM_REG_WAVE_RAM + 0x15:
		case MEM_REG_WAVE_RAM + 0x16:
		case MEM_REG_WAVE_RAM + 0x17:
		case MEM_REG_WAVE_RAM + 0x18:
		case MEM_REG_WAVE_RAM + 0x19:
		case MEM_REG_WAVE_RAM + 0x1A:
		case MEM_REG_WAVE_RAM + 0x1B:
		case MEM_REG_WAVE_RAM + 0x1C:
		case MEM_REG_WAVE_RAM + 0x1D:
		case MEM_REG_WAVE_RAM + 0x1E:
		case MEM_REG_WAVE_RAM + 0x1F:
		case MEM_REG_IME:
		case MEM_REG_IME + 1:
		case MEM_REG_IE:
		case MEM_REG_IE + 1:
		case MEM_REG_TM0CNT_L:
		case MEM_REG_TM0CNT_L + 1:
		case MEM_REG_TM0CNT_H + 1:
		case MEM_REG_TM1CNT_L:
		case MEM_REG_TM1CNT_L + 1:
		case MEM_REG_TM1CNT_H + 1:
		case MEM_REG_TM2CNT_L:
		case MEM_REG_TM2CNT_L + 1:
		case MEM_REG_TM2CNT_H + 1:
		case MEM_REG_TM3CNT_L:
		case MEM_REG_TM3CNT_L + 1:
		case MEM_REG_TM3CNT_H + 1:
			break;
		default:
			//printf("writing to unknown register [%04x] = %x\n", reg, v);
			break;
	}
	mem->io_regs[reg] = v;
}

static void set_reg32(mem_t *mem, uint32_t reg, uint32_t v)
{
	set_reg(mem, reg + 0, v >> 0);
	set_reg(mem, reg + 1, v >> 8);
	set_reg(mem, reg + 2, v >> 16);
	set_reg(mem, reg + 3, v >> 24);
}

static void set_reg16(mem_t *mem, uint32_t reg, uint16_t v)
{
	set_reg(mem, reg + 0, v >> 0);
	set_reg(mem, reg + 1, v >> 8);
}

static void set_reg8(mem_t *mem, uint32_t reg, uint8_t v)
{
	set_reg(mem, reg, v);
}

static uint8_t get_reg(mem_t *mem, uint32_t reg)
{
	switch (reg)
	{
		case MEM_REG_TM0CNT_L:
			return mem->timers[0].v;
		case MEM_REG_TM0CNT_L + 1:
			return mem->timers[0].v >> 8;
		case MEM_REG_TM1CNT_L:
			return mem->timers[1].v;
		case MEM_REG_TM1CNT_L + 1:
			return mem->timers[1].v >> 8;
		case MEM_REG_TM2CNT_L:
			return mem->timers[2].v;
		case MEM_REG_TM2CNT_L + 1:
			return mem->timers[2].v >> 8;
		case MEM_REG_TM3CNT_L:
			return mem->timers[3].v;
		case MEM_REG_TM3CNT_L + 1:
			return mem->timers[3].v >> 8;
		case MEM_REG_KEYINPUT:
		{
			uint8_t v = 0xFF;
			if (mem->gba->joypad & GBA_BUTTON_A)
				v &= ~(1 << 0);
			if (mem->gba->joypad & GBA_BUTTON_B)
				v &= ~(1 << 1);
			if (mem->gba->joypad & GBA_BUTTON_SELECT)
				v &= ~(1 << 2);
			if (mem->gba->joypad & GBA_BUTTON_START)
				v &= ~(1 << 3);
			if (mem->gba->joypad & GBA_BUTTON_RIGHT)
				v &= ~(1 << 4);
			if (mem->gba->joypad & GBA_BUTTON_LEFT)
				v &= ~(1 << 5);
			if (mem->gba->joypad & GBA_BUTTON_UP)
				v &= ~(1 << 6);
			if (mem->gba->joypad & GBA_BUTTON_DOWN)
				v &= ~(1 << 7);
			return v;
		}
		case MEM_REG_KEYINPUT + 1:
		{
			uint8_t v = 0x3;
			if (mem->gba->joypad & GBA_BUTTON_R)
				v &= ~(1 << 0);
			if (mem->gba->joypad & GBA_BUTTON_L)
				v &= ~(1 << 1);
			return v;
		}
	}
	return mem->io_regs[reg];
}

static uint32_t get_reg32(mem_t *mem, uint32_t reg)
{
	return (get_reg(mem, reg + 0) << 0)
	     | (get_reg(mem, reg + 1) << 8)
	     | (get_reg(mem, reg + 2) << 16)
	     | (get_reg(mem, reg + 3) << 24);
}

static uint16_t get_reg16(mem_t *mem, uint32_t reg)
{
	return (get_reg(mem, reg + 0) << 0)
	     | (get_reg(mem, reg + 1) << 8);
}

static uint8_t get_reg8(mem_t *mem, uint32_t reg)
{
	return get_reg(mem, reg);
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
			{ \
				if (cpu_get_reg(mem->gba->cpu, CPU_REG_PC) < 0x4000) \
					return *(uint##size##_t*)&mem->bios[addr]; \
				return *(uint##size##_t*)&mem->bios[mem->gba->cpu->last_bios_decode]; \
			} \
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
			return get_reg##size(mem, a); \
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
	printf("unknown get" #size " addr: %08x\n", addr); \
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
			set_reg##size(mem, a, v); \
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
	printf("unknown set" #size " addr: %08x\n", addr); \
}

MEM_SET(8);
MEM_SET(16);
MEM_SET(32);
