#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct cpu_s cpu_t;

typedef struct cpu_instr_s
{
	void (*exec)(cpu_t *cpu);
	void (*print)(cpu_t *cpu, char *data, size_t size);
} cpu_instr_t;

extern const cpu_instr_t *cpu_instr_thumb[0x400];
extern const cpu_instr_t *cpu_instr_arm[0x1000];

#endif
