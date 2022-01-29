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
		snprintf(rtmp, 13, "R%02u=%08x", i, cpu->regs.r[i]);
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
	        cpu->regs.r[15],
	        tmp,
	        cpu->instr_opcode,
	        regs);
}

static bool next_instruction(cpu_t *cpu)
{
	if (cpu->thumb)
	{
		cpu->instr_opcode = mem_get16(cpu->mem, cpu->regs.r[15]);
		cpu->instr = cpu_instr_thumb[cpu->instr_opcode >> 8];
		cpu->regs.r[15] += 2;
	}
	else
	{
		cpu->instr_opcode = mem_get32(cpu->mem, cpu->regs.r[15]);
		if (!check_arm_cond(cpu, cpu->instr_opcode >> 28))
		{
			print_instr(cpu, "SKIP", cpu_instr_arm[((cpu->instr_opcode >> 16) & 0xFF0) | ((cpu->instr_opcode >> 4) & 0xF)]);
			cpu->regs.r[15] += 4;
			cpu->instr = NULL;
			return false;
		}
		cpu->instr = cpu_instr_arm[((cpu->instr_opcode >> 16) & 0xFF0) | ((cpu->instr_opcode >> 4) & 0xF)];
	}

	return true;
}

void cpu_cycle(cpu_t *cpu)
{
	if (cpu->instr_delay)
	{
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
	}

	cpu->regs.r[15] += 4;
	if (!next_instruction(cpu))
		return;
}
