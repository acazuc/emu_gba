#include "mbc.h"
#include "mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

mbc_t *mbc_new(const void *data, size_t size)
{
	mbc_t *mbc = calloc(sizeof(*mbc), 1);
	if (!mbc)
		return NULL;

	mbc->data = malloc(size);
	if (!mbc->data)
		return NULL;

	memcpy(mbc->data, data, size);
	memset(mbc->backup, 0xff, sizeof(mbc->backup));
	mbc->data_size = size;

	if (memmem(data, size, "EEPROM_V", 8))
		mbc->backup_type = MBC_EEPROM;
	else if (memmem(data, size, "SRAM_V", 6))
		mbc->backup_type = MBC_SRAM;
	else if (memmem(data, size, "FLASH_V", 7))
		mbc->backup_type = MBC_FLASH64;
	else if (memmem(data, size, "FLASH512_V", 10))
		mbc->backup_type = MBC_FLASH64;
	else if (memmem(data, size, "FLASH1M_V", 9))
		mbc->backup_type = MBC_FLASH128;
	return mbc;
}

void mbc_del(mbc_t *mbc)
{
	if (!mbc)
		return;
	free(mbc->data);
	free(mbc);
}

#define MBC_GET(size) \
static uint##size##_t eeprom_get##size(mbc_t *mbc, uint32_t addr) \
{ \
	printf("eeprom get" #size " %x\n", addr); \
	addr &= 0x1FFF; \
	return *(uint##size##_t*)&mbc->backup[addr]; \
} \
static uint##size##_t sram_get##size(mbc_t *mbc, uint32_t addr) \
{ \
	printf("sram get" #size " %x\n", addr); \
	addr &= 0x7FFF; \
	return *(uint##size##_t*)&mbc->backup[addr]; \
} \
static uint##size##_t flash64_get##size(mbc_t *mbc, uint32_t addr) \
{ \
	printf("flash64 get" #size "%x\n", addr); \
	addr &= 0xFFFF; \
	if (mbc->chipid) \
	{ \
		if (addr == 0) \
			return 0xC2; \
		if (addr == 1) \
			return 0x1C; \
	} \
	return *(uint##size##_t*)&mbc->backup[addr]; \
} \
static uint##size##_t flash128_get##size(mbc_t *mbc, uint32_t addr) \
{ \
	printf("flash128 get" #size " %x\n", addr); \
	addr &= 0x1FFFF; \
	if (mbc->chipid) \
	{ \
		if (addr == 0) \
			return 0xC2; \
		if (addr == 1) \
			return 0x09; \
	} \
	return *(uint##size##_t*)&mbc->backup[addr]; \
} \
uint##size##_t mbc_get##size(mbc_t *mbc, uint32_t addr) \
{ \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x8: \
		case 0x9: /* rom0 */ \
		case 0xA: \
		case 0xB: /* rom1 */ \
		case 0xC: \
		case 0xD: /* rom2 */ \
		{ \
			uint32_t a = addr & 0x1FFFFFF; \
			if (a < mbc->data_size) \
				return *(uint##size##_t*)&mbc->data[a]; \
			if (size == 16) \
				return addr >> 1; \
			if (size == 32) \
			{ \
				uint16_t lo = addr >> 1; \
				uint16_t hi = lo + 1; \
				return (hi << 16) | lo; \
			} \
			break; \
		} \
		case 0xE: /* backup */ \
		{ \
			switch (mbc->backup_type) \
			{ \
				case MBC_EEPROM: \
					return eeprom_get##size(mbc, addr); \
				case MBC_SRAM: \
					return sram_get##size(mbc, addr); \
				case MBC_FLASH64: \
					return flash64_get##size(mbc, addr); \
				case MBC_FLASH128: \
					return flash128_get##size(mbc, addr); \
			} \
		} \
		case 0xF: /* unused */ \
			break; \
	} \
	printf("unknown get" #size " mbc addr: %08x\n", addr); \
	return 0; \
}

MBC_GET(8);
MBC_GET(16);
MBC_GET(32);

#define MBC_SET(size) \
static void eeprom_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	printf("eeprom set" #size " %x = %x\n", addr, v); \
	addr &= 0x1FFF; \
	*(uint##size##_t*)&mbc->backup[addr] = v; \
} \
static void sram_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	printf("sram set" #size " %x = %x\n", addr, v); \
	addr &= 0x7FFF; \
	*(uint##size##_t*)&mbc->backup[addr] = v; \
} \
static void flash64_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	printf("flash64 set" #size " %x = %x\n", addr, v); \
	addr &= 0xFFFF; \
	*(uint##size##_t*)&mbc->backup[addr] = v; \
} \
static void flash128_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	printf("flash128 set" #size " %x = %x\n", addr, v); \
	addr &= 0x1FFFF; \
	if (mbc->cmdphase == 0 && addr == 0x5555 && v == 0xAA) \
	{ \
		mbc->cmdphase++; \
		return;  \
	} \
	if (mbc->cmdphase == 1 && addr == 0x2AAA && v == 0x55) \
	{ \
		mbc->cmdphase++; \
		return; \
	} \
	if (mbc->cmdphase == 2 && addr == 0x5555) \
	{ \
		switch (v) \
		{ \
			case 0x90: \
				mbc->chipid = true; \
				break; \
			case 0xF0: \
				mbc->chipid = false; \
				break; \
		} \
		return; \
	} \
	*(uint##size##_t*)&mbc->backup[addr] = v; \
} \
void mbc_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x8: \
		case 0x9: /* rom0 */ \
		case 0xA: \
		case 0xB: /* rom1 */ \
		case 0xC: \
		case 0xD: /* rom2 */ \
			return; \
		case 0xE: /* backup */ \
		{ \
			uint32_t a = addr & 0x7FFF; \
			*(uint##size##_t*)&mbc->backup[a] = v; \
			switch (mbc->backup_type) \
			{ \
				case MBC_EEPROM: \
					eeprom_set##size(mbc, addr, v); \
					return; \
				case MBC_SRAM: \
					sram_set##size(mbc, addr, v); \
					return; \
				case MBC_FLASH64: \
					flash64_set##size(mbc, addr, v); \
					return; \
				case MBC_FLASH128: \
					flash128_set##size(mbc, addr, v); \
					return; \
			} \
		} \
		case 0xF: /* unused */ \
			break; \
	} \
	printf("unknown set" #size " mbc addr: %08x\n", addr); \
}

MBC_SET(8);
MBC_SET(16);
MBC_SET(32);
