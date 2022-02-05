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

uint8_t  mbc_get8 (mbc_t *mbc, uint32_t addr);
uint16_t mbc_get16(mbc_t *mbc, uint32_t addr);
uint32_t mbc_get32(mbc_t *mbc, uint32_t addr);

void mbc_set8 (mbc_t *mbc, uint32_t addr, uint8_t v);
void mbc_set16(mbc_t *mbc, uint32_t addr, uint16_t v);
void mbc_set32(mbc_t *mbc, uint32_t addr, uint32_t v);

#endif
