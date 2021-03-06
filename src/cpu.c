#include "cpu.h"
#include "mem.h"
#include "cpu/instr.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

cpu_t *cpu_new(mem_t *mem)
{
	cpu_t *cpu = calloc(sizeof(*cpu), 1);
	if (!cpu)
		return NULL;

	cpu->mem = mem;
	cpu->regs.cpsr = 0xD3;
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
			return !CPU_GET_FLAG_C(cpu) || CPU_GET_FLAG_Z(cpu);
		case 0xA:
			return CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu);
		case 0xB:
			return CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu);
		case 0xC:
			return !CPU_GET_FLAG_Z(cpu) && CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu);
		case 0xD:
			return CPU_GET_FLAG_Z(cpu) || CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu);
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
	char tmp[1024] = "";

	if ((cpu->debug & CPU_DEBUG_INSTR) && instr->print)
	{
		tmp[0] = ' ';
		instr->print(cpu, tmp + 1, sizeof(tmp) - 1);
	}

	printf("[%-4s] [%08x] [%08x] [%08x]%s\n",
	        msg,
	        cpu->regs.cpsr,
	        *cpu->regs.spsr,
	        cpu->instr_opcode,
	        tmp
	        );

	if (cpu->debug & CPU_DEBUG_REGS)
	{
		if (cpu->debug & CPU_DEBUG_REGS_ML)
		{
			printf("r00=%08x r01=%08x r02=%08x r03=%08x\n"
			       "r04=%08x r05=%08x r06=%08x r07=%08x\n"
			       "r08=%08x r09=%08x r10=%08x r11=%08x\n"
			       "r12=%08x r13=%08x r14=%08x r15=%08x\n",
			       cpu_get_reg(cpu, 0x0),
			       cpu_get_reg(cpu, 0x1),
			       cpu_get_reg(cpu, 0x2),
			       cpu_get_reg(cpu, 0x3),
			       cpu_get_reg(cpu, 0x4),
			       cpu_get_reg(cpu, 0x5),
			       cpu_get_reg(cpu, 0x6),
			       cpu_get_reg(cpu, 0x7),
			       cpu_get_reg(cpu, 0x8),
			       cpu_get_reg(cpu, 0x9),
			       cpu_get_reg(cpu, 0xA),
			       cpu_get_reg(cpu, 0xB),
			       cpu_get_reg(cpu, 0xC),
			       cpu_get_reg(cpu, 0xD),
			       cpu_get_reg(cpu, 0xE),
			       cpu_get_reg(cpu, 0xF));
		}
		else
		{
			printf("r00=%08x r01=%08x r02=%08x r03=%08x "
			       "r04=%08x r05=%08x r06=%08x r07=%08x "
			       "r08=%08x r09=%08x r10=%08x r11=%08x "
			       "r12=%08x r13=%08x r14=%08x r15=%08x\n",
			       cpu_get_reg(cpu, 0x0),
			       cpu_get_reg(cpu, 0x1),
			       cpu_get_reg(cpu, 0x2),
			       cpu_get_reg(cpu, 0x3),
			       cpu_get_reg(cpu, 0x4),
			       cpu_get_reg(cpu, 0x5),
			       cpu_get_reg(cpu, 0x6),
			       cpu_get_reg(cpu, 0x7),
			       cpu_get_reg(cpu, 0x8),
			       cpu_get_reg(cpu, 0x9),
			       cpu_get_reg(cpu, 0xA),
			       cpu_get_reg(cpu, 0xB),
			       cpu_get_reg(cpu, 0xC),
			       cpu_get_reg(cpu, 0xD),
			       cpu_get_reg(cpu, 0xE),
			       cpu_get_reg(cpu, 0xF));
		}
		fflush(stdout);
	}
}

static bool handle_interrupt(cpu_t *cpu)
{
	if (CPU_GET_FLAG_I(cpu))
		return false;
	uint16_t reg_if = mem_get_reg16(cpu->mem, MEM_REG_IF);
	if (!reg_if)
		return false;
	uint16_t reg_ie = mem_get_reg16(cpu->mem, MEM_REG_IE);
	uint16_t ints = reg_ie & reg_if;
	if (!ints)
		return false;
	cpu->state = CPU_STATE_RUN;
	uint16_t ime = mem_get_reg16(cpu->mem, MEM_REG_IME);
	if (!ime)
		return false;
	for (uint8_t i = 0; i < 16; ++i)
	{
		if (!(ints & (1 << i)))
			continue;
		cpu->regs.spsr_modes[3] = cpu->regs.cpsr;
		CPU_SET_MODE(cpu, CPU_MODE_IRQ);
		cpu_update_mode(cpu);
		CPU_SET_FLAG_I(cpu, 1);
		if (CPU_GET_FLAG_T(cpu))
		{
			CPU_SET_FLAG_T(cpu, 0);
			cpu_set_reg(cpu, CPU_REG_LR, cpu_get_reg(cpu, CPU_REG_PC) + 4);
		}
		else
		{
			cpu_set_reg(cpu, CPU_REG_LR, cpu_get_reg(cpu, CPU_REG_PC) + 4);
		}
		cpu_set_reg(cpu, CPU_REG_PC, 0x18);
		return true;
	}
	return false;
}

static bool decode_instruction(cpu_t *cpu)
{
	if (CPU_GET_FLAG_T(cpu))
	{
		uint32_t pc = cpu_get_reg(cpu, CPU_REG_PC);
		if (pc < 0x4000)
			cpu->last_bios_decode = pc + 4;
		cpu->instr_opcode = mem_get16(cpu->mem, pc);
		cpu->instr = cpu_instr_thumb[cpu->instr_opcode >> 6];
	}
	else
	{
		uint32_t pc = cpu_get_reg(cpu, CPU_REG_PC);
		if (pc < 0x4000)
			cpu->last_bios_decode = pc + 8;
		cpu->instr_opcode = mem_get32(cpu->mem, pc);
		if (!check_arm_cond(cpu, cpu->instr_opcode >> 28))
		{
			if (cpu->debug)
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
	//if (cpu_get_reg(cpu, CPU_REG_PC) >= 0x4000)
	//	cpu->debug = CPU_DEBUG_REGS | CPU_DEBUG_INSTR;
	//if (cpu_get_reg(cpu, CPU_REG_PC) == 0x872)
	//	cpu->debug = 0;

	if (cpu->instr_delay)
	{
		cpu->instr_delay--;
		return;
	}

	if (!cpu->instr)
	{
		if (!decode_instruction(cpu))
			return;
	}

	if (cpu->state != CPU_STATE_RUN)
	{
		if (!handle_interrupt(cpu))
			return;
		if (!decode_instruction(cpu))
			return;
	}

	if (cpu->debug)
		print_instr(cpu, "EXEC", cpu->instr);
	cpu->instr->exec(cpu);

	(void)handle_interrupt(cpu);
	(void)decode_instruction(cpu);
}

void cpu_update_mode(cpu_t *cpu)
{
	for (size_t i = 0; i < 16; ++i)
		cpu->regs.rptr[i] = &cpu->regs.r[i];
	switch (CPU_GET_MODE(cpu))
	{
		case CPU_MODE_USR:
		case CPU_MODE_SYS:
			cpu->regs.spsr = &cpu->regs.cpsr;
			break;
		case CPU_MODE_FIQ:
			for (size_t i = 8; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_fiq[i - 8];
			cpu->regs.spsr = &cpu->regs.spsr_modes[0];
			break;
		case CPU_MODE_SVC:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_svc[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[1];
			break;
		case CPU_MODE_ABT:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_abt[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[2];
			break;
		case CPU_MODE_IRQ:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_irq[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[3];
			break;
		case CPU_MODE_UND:
			for (size_t i = 13; i < 15; ++i)
				cpu->regs.rptr[i] = &cpu->regs.r_und[i - 13];
			cpu->regs.spsr = &cpu->regs.spsr_modes[4];
			break;
		default:
			printf("unknown mode: %x\n", CPU_GET_MODE(cpu));
			assert(!"invalid mode");
	}
}
