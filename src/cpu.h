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

#define CPU_HAS_FLAG(cpu, f) ((cpu->regs.cpsr & f) ? 1 : 0)

#define CPU_HAS_FLAG_N(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_N)
#define CPU_HAS_FLAG_Z(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_Z)
#define CPU_HAS_FLAG_C(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_C)
#define CPU_HAS_FLAG_V(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_V)
#define CPU_HAS_FLAG_Q(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_Q)
#define CPU_HAS_FLAG_I(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_I)
#define CPU_HAS_FLAG_F(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_F)
#define CPU_HAS_FLAG_T(cpu) CPU_HAS_FLAG(cpu, CPU_FLAG_T)

typedef struct cpu_regs_s
{
	uint32_t r[16];
	uint32_t r_fiq[7];
	uint32_t r_svc[2];
	uint32_t r_abt[2];
	uint32_t r_irq[2];
	uint32_t r_und[2];
	uint32_t cpsr;
	uint32_t spsr_fiq;
	uint32_t spsr_svc;
	uint32_t spsr_abt;
	uint32_t spsr_irq;
	uint32_t spsr_und;
} cpu_regs_t;

enum cpu_mode
{
	CPU_MODE_SYS,
	CPU_MODE_FIQ,
	CPU_MODE_SVC,
	CPU_MODE_ABT,
	CPU_MODE_IEQ,
	CPU_MODE_UND,
};

typedef struct cpu_s
{
	cpu_regs_t regs;
	enum cpu_mode mode;
	bool thumb;
	mem_t *mem;
	const cpu_instr_t *instr;
	uint32_t instr_opcode;
} cpu_t;

cpu_t *cpu_new(mem_t *mem);
void cpu_del(cpu_t *cpu);

void cpu_cycle(cpu_t *cpu);

#endif
