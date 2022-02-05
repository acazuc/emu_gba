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
	mbc->size = size;
	return mbc;
}

void mbc_del(mbc_t *mbc)
{
	if (!mbc)
		return;
	free(mbc->data);
	free(mbc);
}

#define MBC_GET(s) \
uint##s##_t mbc_get##s(mbc_t *mbc, uint32_t addr) \
{ \
	switch ((addr >> 24) & 0xF) \
	{ \
		case 0x8: \
		case 0x9: /* rom0 */ \
		case 0xA: \
		case 0xB: /* rom */ \
		case 0xC: \
		case 0xD: /* rom2 */ \
		{ \
			uint32_t a = addr & 0x1FFFFF; \
			if (a < mbc->size) \
				return *(uint##s##_t*)&mbc->data[a]; \
			if (s == 16) \
				return addr >> 1; \
			if (s == 32) \
			{ \
				uint16_t lo = addr >> 1; \
				uint16_t hi = lo + 1; \
				return (hi << 16) | lo; \
			} \
			break; \
		} \
		case 0xE: \
			if (addr < 0xE010000) \
				return 0; /* XXX: SRAM */ \
			break; \
		case 0xF: /* unused */ \
			break; \
	} \
	printf("unknown mbc addr: %08x\n", addr); \
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
		case 0xB: /* rom */ \
		case 0xC: \
		case 0xD: /* rom2 */ \
			return; \
		case 0xE: \
			return; \
		case 0xF: /* unused */ \
			break; \
	} \
	printf("unknown mbc addr: %08x\n", addr); \
	return; \
}

MBC_SET(8);
MBC_SET(16);
MBC_SET(32);
