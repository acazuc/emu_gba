#ifndef CPU_INSTR_H
#define CPU_INSTR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct cpu_s cpu_t;

typedef struct cpu_instr_s
{
	const char *name;
	void (*print)(cpu_t *cpu, char *data, size_t size);
	bool (*exec)(cpu_t *cpu);
} cpu_instr_t;

#endif
