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

void *mbc_ptr(mbc_t *mbc, uint32_t addr)
{
	switch ((addr >> 28) & 0xF)
	{
		case 0x8:
		case 0x9: //rom0
			break;
		case 0xA:
		case 0xB: //rom1
			break;
		case 0xC:
		case 0xD: //rom2
			break;
		case 0xE:
			if (addr < 0xE0010000)
				return NULL; //XXX: SRAM
			break;
		case 0xF: //unused
			break;
	}
	fprintf(stderr, "unknown addr: %08x\n", addr);
	return NULL;
}
