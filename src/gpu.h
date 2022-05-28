#ifndef GPU_H
#define GPU_H

#include <stdint.h>

typedef struct mem_s mem_t;

typedef struct gpu_s
{
	uint8_t data[240 * 160 * 4];
	int32_t bg2x;
	int32_t bg2y;
	int32_t bg3x;
	int32_t bg3y;
	mem_t *mem;
} gpu_t;

gpu_t *gpu_new(mem_t *mem);
void gpu_del(gpu_t *gpu);

void gpu_draw(gpu_t *gpu, uint8_t y);

#endif
