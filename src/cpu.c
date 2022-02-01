#include "cpu.h"
#include "mem.h"
#include "cpu/instr.h"
#include "cpu/instr_thumb.h"
#include "cpu/instr_arm.h"
#include <stdlib.h>
#include <stdio.h>

cpu_t *cpu_new(mem_t *mem)
{
	cpu_t *cpu = calloc(sizeof(*cpu), 1);
	if (!cpu)
		return NULL;

	cpu->mem = mem;
	cpu_update_mode(cpu);
	return cpu;
}

void cpu_del(cpu_t *cpu)
{
	if (!cpu)
		return;
	free(cpu);
}

static bool check_arm_cond(cpu_t *cpu, uint32_t cond)
{
	switch (cond & 0xF)
	{
		case 0x0:
			return CPU_GET_FLAG_Z(cpu);
		case 0x1:
			return !CPU_GET_FLAG_Z(cpu);
		case 0x2:
			return CPU_GET_FLAG_C(cpu);
		case 0x3:
			return !CPU_GET_FLAG_C(cpu);
		case 0x4:
			return CPU_GET_FLAG_N(cpu);
		case 0x5:
			return !CPU_GET_FLAG_N(cpu);
		case 0x6:
			return CPU_GET_FLAG_V(cpu);
		case 0x7:
			return !CPU_GET_FLAG_V(cpu);
		case 0x8:
			return CPU_GET_FLAG_C(cpu) && !CPU_GET_FLAG_Z(cpu);
		case 0x9:
			return !CPU_GET_FLAG_C(cpu) && CPU_GET_FLAG_Z(cpu);
		case 0xA:
			return CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu);
		case 0xB:
			return CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu);
		case 0xC:
			return !CPU_GET_FLAG_Z(cpu) && CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu);
		case 0xD:
			return CPU_GET_FLAG_Z(cpu) && CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu);
		case 0xE:
			return true;
		case 0xF:
			return false;
	}
	/* unreachable */
	return false;
}

static void print_instr(cpu_t *cpu, const char *msg, const cpu_instr_t *instr)
{
	char tmp[1024];
	char regs[1024];
	char *rtmp;

	if (instr->print)
		instr->print(cpu, tmp, sizeof(tmp));
	else
		snprintf(tmp, sizeof(tmp), "%s", instr->name);

	rtmp = regs;
	for (unsigned i = 0; i < 16; ++i)
	{
		snprintf(rtmp, 13, "r%02u=%08x", i, cpu_get_reg(cpu, i));
		rtmp += 12;
		if (i == 15)
			break;
		if (i % 4 == 3)
		{
			snprintf(rtmp, 62, "\n%60s", "");
			rtmp += 61;
		}
		else
		{
			snprintf(rtmp, 2, " ");
			rtmp++;
		}
	}

	printf("[%-4s] [%08x] %-30s (%08x) %s\n\n",
	        msg,
	        cpu_get_reg(cpu, CPU_REG_PC),
	        tmp,
	        cpu->instr_opcode,
	        regs);
}

static bool next_instruction(cpu_t *cpu)
{
	if (CPU_GET_FLAG_T(cpu))
	{
		cpu->instr_opcode = mem_get16(cpu->mem, cpu_get_reg(cpu, CPU_REG_PC));
		cpu->instr = cpu_instr_thumb[cpu->instr_opcode >> 6];
	}
	else
	{
		cpu->instr_opcode = mem_get32(cpu->mem, cpu_get_reg(cpu, CPU_REG_PC));
		if (!check_arm_cond(cpu, cpu->instr_opcode >> 28))
		{
			print_instr(cpu, "SKIP", cpu_instr_arm[((cpu->instr_opcode >> 16) & 0xFF0) | ((cpu->instr_opcode >> 4) & 0xF)]);
			cpu_inc_pc(cpu, 4);
			cpu->instr = NULL;
			return false;
		}
		cpu->instr = cpu_instr_arm[((cpu->instr_opcode >> 16) & 0xFF0) | ((cpu->instr_opcode >> 4) & 0xF)];
	}

	return true;
}

void cpu_cycle(cpu_t *cpu)
{
	switch (cpu->instr_delay)
	{
		case 1:
			cpu->instr_delay--;
			break;
		case 0:
			break;
		default:
			cpu->instr_delay--;
			return;
	}

	if (!cpu->instr)
	{
		if (!next_instruction(cpu))
			return;
	}

	if (cpu->instr->exec)
	{
		print_instr(cpu, "EXEC", cpu->instr);
		cpu->instr->exec(cpu);
	}
	else
	{
		print_instr(cpu, "UNIM", cpu->instr);
		cpu->regs.r[15] += CPU_GET_FLAG_T(cpu) ? 2 : 4;
	}

	if (!next_instruction(cpu))
		return;
}

void cpu_update_mode(cpu_t *cpu)
{
	for (size_t i = 0; i < 16; ++i)
		cpu->regs.rptr[i] = &cpu->regs.r[i];
	switch (CPU_GET_MODE(cpu))
	{
		case CPU_MODE_USR:
		case CPU_MODE_SYS:
			cpu->regs.spsr = &cpu->regs.spsr_modes[0];
			break;
		case CPU_MODE_FIQ:
			for (size_t i = 8; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_fiq[i - 8];
			cpu->regs.spsr = &cpu->regs.spsr_modes[1];
			break;
		case CPU_MODE_SVC:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_svc[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[2];
			break;
		case CPU_MODE_ABT:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_abt[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[3];
			break;
		case CPU_MODE_IRQ:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_irq[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[4];
			break;
		case CPU_MODE_UND:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_und[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[5];
			break;
	}
}
