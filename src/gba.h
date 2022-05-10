#ifndef GBA_H
#define GBA_H

#include <stddef.h>
#include <stdint.h>

typedef struct mbc_s mbc_t;
typedef struct mem_s mem_t;
typedef struct apu_s apu_t;
typedef struct cpu_s cpu_t;
typedef struct gpu_s gpu_t;

enum gba_button
{
	GBA_BUTTON_RIGHT  = (1 << 0),
	GBA_BUTTON_LEFT   = (1 << 1),
	GBA_BUTTON_UP     = (1 << 2),
	GBA_BUTTON_DOWN   = (1 << 3),
	GBA_BUTTON_A      = (1 << 4),
	GBA_BUTTON_B      = (1 << 5),
	GBA_BUTTON_L      = (1 << 6),
	GBA_BUTTON_R      = (1 << 7),
	GBA_BUTTON_SELECT = (1 << 8),
	GBA_BUTTON_START  = (1 << 9),
};

typedef struct gba_s
{
	mbc_t *mbc;
	mem_t *mem;
	apu_t *apu;
	cpu_t *cpu;
	gpu_t *gpu;
	uint32_t joypad;
	uint32_t cycle;
} gba_t;

gba_t *gba_new(const void *rom_data, size_t rom_size);
void gba_del(gba_t *gba);

void gba_frame(gba_t *gba, uint8_t *video_buf, int16_t *audio_buf, uint32_t joypad);

void gba_get_mbc_ram(gba_t *gba, uint8_t **data, size_t *size);
void gba_get_mbc_rtc(gba_t *gba, uint8_t **data, size_t *size);

void gba_test_keypad_int(gba_t *gba);

#endif
