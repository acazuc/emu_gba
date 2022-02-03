#ifndef MBC_H
#define MBC_H

#include <stddef.h>
#include <stdint.h>

typedef struct mbc_s
{
	uint8_t *data;
	size_t size;
} mbc_t;

mbc_t *mbc_new(const void *data, size_t size);
void mbc_del(mbc_t *mbc);

void *mbc_ptr(mbc_t *mbc, uint32_t addr);

#endif
