#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cpu_instr_s cpu_instr_t;
typedef struct mem_s mem_t;

#define CPU_FLAG_N (1 << 31)
#define CPU_FLAG_Z (1 << 30)
#define CPU_FLAG_C (1 << 29)
#define CPU_FLAG_V (1 << 28)
#define CPU_FLAG_Q (1 << 27)
#define CPU_FLAG_I (1 << 7)
#define CPU_FLAG_F (1 << 6)
#define CPU_FLAG_T (1 << 5)

#define CPU_GET_FLAG(cpu, f) (((cpu)->regs.cpsr & (f)) ? 1 : 0)
#define CPU_GET_FLAG_N(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_N)
#define CPU_GET_FLAG_Z(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_Z)
#define CPU_GET_FLAG_C(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_C)
#define CPU_GET_FLAG_V(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_V)
#define CPU_GET_FLAG_Q(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_Q)
#define CPU_GET_FLAG_I(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_I)
#define CPU_GET_FLAG_F(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_F)
#define CPU_GET_FLAG_T(cpu) CPU_GET_FLAG(cpu, CPU_FLAG_T)

#define CPU_SET_FLAG(cpu, f, v) \
do \
{ \
	if (v) \
		(cpu)->regs.cpsr |= f; \
	else \
		(cpu)->regs.cpsr &= ~f; \
} while (0)
#define CPU_SET_FLAG_N(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_N, v)
#define CPU_SET_FLAG_Z(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_Z, v)
#define CPU_SET_FLAG_C(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_C, v)
#define CPU_SET_FLAG_V(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_V, v)
#define CPU_SET_FLAG_Q(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_Q, v)
#define CPU_SET_FLAG_I(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_I, v)
#define CPU_SET_FLAG_F(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_F, v)
#define CPU_SET_FLAG_T(cpu, v) CPU_SET_FLAG(cpu, CPU_FLAG_T, v)

#define CPU_GET_MODE(cpu) ((cpu)->regs.cpsr & 0xF)

#define CPU_MODE_USR 0x0
#define CPU_MODE_FIQ 0x1
#define CPU_MODE_IRQ 0x2
#define CPU_MODE_SVC 0x3
#define CPU_MODE_ABT 0x7
#define CPU_MODE_UND 0xB
#define CPU_MODE_SYS 0xF

typedef struct cpu_regs_s
{
	uint32_t r[16];
	uint32_t r_fiq[7];
	uint32_t r_svc[2];
	uint32_t r_abt[2];
	uint32_t r_irq[2];
	uint32_t r_und[2];
	uint32_t cpsr;
	uint32_t spsr_modes[6];
	uint32_t *rptr[16];
	uint32_t *spsr;
} cpu_regs_t;

typedef struct cpu_s
{
	cpu_regs_t regs;
	mem_t *mem;
	const cpu_instr_t *instr;
	uint32_t instr_opcode;
	uint32_t instr_delay;
} cpu_t;

cpu_t *cpu_new(mem_t *mem);
void cpu_del(cpu_t *cpu);

void cpu_cycle(cpu_t *cpu);
void cpu_update_mode(cpu_t *cpu);

static inline uint32_t cpu_get_reg(cpu_t *cpu, uint32_t reg)
{
	return *cpu->regs.rptr[reg];
}

static inline void cpu_set_reg(cpu_t *cpu, uint32_t reg, uint32_t v)
{
	*cpu->regs.rptr[reg] = v;
}

static inline void cpu_inc_pc(cpu_t *cpu, uint32_t v)
{
	*cpu->regs.rptr[15] += v;
}

#endif
