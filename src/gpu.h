#ifndef GPU_H
#define GPU_H

typedef struct mem_s mem_t;

typedef struct gpu_s
{
	mem_t *mem;
} gpu_t;

gpu_t *gpu_new(mem_t *mem);
void gpu_del(gpu_t *gpu);

#endif
