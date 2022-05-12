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
	memset(mbc->sram, 0xff, sizeof(mbc->sram));
	mbc->data_size = size;
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
		case 0xE: /* sram */ \
		{ \
			uint32_t a = addr & 0x7FFF; \
			return *(uint##size##_t*)&mbc->sram[a]; \
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
void mbc_set##size(mbc_t *mbc, uint32_t addr, uint##size##_t v) \
{ \
	(void)mbc; \
	(void)v; \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x8: \
		case 0x9: /* rom0 */ \
		case 0xA: \
		case 0xB: /* rom1 */ \
		case 0xC: \
		case 0xD: /* rom2 */ \
			return; \
		case 0xE: /* sram */ \
		{ \
			uint32_t a = addr & 0x7FFF; \
			*(uint##size##_t*)&mbc->sram[a] = v; \
			return; \
		} \
		case 0xF: /* unused */ \
			break; \
	} \
	printf("unknown set" #size " mbc addr: %08x\n", addr); \
}

MBC_SET(8);
MBC_SET(16);
MBC_SET(32);
