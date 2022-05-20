#ifndef APU_H
#define APU_H

#include <stdint.h>

#define APU_FRAME_SAMPLES 804

typedef struct mem_s mem_t;

typedef struct apu_swp_s
{
	uint8_t step;
	uint8_t time;
	uint8_t dir;
	uint8_t cnt;
	uint8_t nb;
} apu_swp_t;

typedef struct apu_env_s
{
	uint8_t step;
	uint8_t time;
	uint8_t dir;
	uint8_t val;
} apu_env_t;

typedef struct apu_s
{
	uint16_t data[APU_FRAME_SAMPLES];
	uint32_t sample;
	uint32_t clock;
	uint8_t wave1_haslen;
	apu_swp_t wave1_swp;
	apu_env_t wave1_env;
	uint8_t wave1_duty;
	uint32_t wave1_len;
	uint32_t wave1_val;
	uint32_t wave1_cnt;
	uint32_t wave1_nb;
	uint8_t wave2_haslen;
	apu_env_t wave2_env;
	uint8_t wave2_duty;
	uint32_t wave2_len;
	uint32_t wave2_val;
	uint32_t wave2_cnt;
	uint32_t wave2_nb;
	uint8_t wave3_haslen;
	uint32_t wave3_gain;
	uint32_t wave3_len;
	uint32_t wave3_val;
	uint32_t wave3_cnt;
	uint32_t wave3_nb;
	uint8_t wave4_haslen;
	apu_env_t wave4_env;
	uint32_t wave4_cycle;
	uint32_t wave4_len;
	uint32_t wave4_val;
	uint32_t wave4_cnt;
	uint32_t wave4_nb;
	uint8_t fifo1_val;
	uint8_t fifo2_val;
	uint32_t timer;
	mem_t *mem;
} apu_t;

apu_t *apu_new(mem_t *mem);
void apu_del(apu_t *apu);

void apu_cycle(apu_t *apu);

void apu_start_channel1(apu_t *apu);
void apu_start_channel2(apu_t *apu);
void apu_start_channel3(apu_t *apu);
void apu_start_channel4(apu_t *apu);

#endif
