#include "instr_arm.h"
#include "../cpu.h"
#include "../mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ARM_LSL(v, s) ((v) << (s))
#define ARM_LSR(v, s) ((v) >> (s))
#define ARM_ASR(v, s) ((int32_t)(v) >> (s))
#define ARM_ROR(v, s) (((v) >> (s)) | ((v) << (32 - (s))))

static void exec_alu_flags_logical(cpu_t *cpu, uint32_t v)
{
	CPU_SET_FLAG_N(cpu, v & 0x80000000);
	CPU_SET_FLAG_Z(cpu, !v);
}

static void exec_alu_flags_arithmetical(cpu_t *cpu, uint32_t v, uint32_t op1, uint32_t op2)
{
	CPU_SET_FLAG_V(cpu, ((op1 ^ op2) & 0x80000000) && ((op1 ^ v) & 0x80000000));
	CPU_SET_FLAG_N(cpu, v & 0x80000000);
	CPU_SET_FLAG_Z(cpu, !v);
}

static void exec_alu_and(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 & op2);
}

static void exec_alu_ands(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 & op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_eor(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 ^ op2);
}

static void exec_alu_eors(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 ^ op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_sub(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 - op2);
}

static void exec_alu_subs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 - op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op1, op2);
		CPU_SET_FLAG_C(cpu, op2 > op1);
	}
}

static void exec_alu_rsb(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op2 - op1);
}

static void exec_alu_rsbs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op2 - op1;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op2, op1);
		CPU_SET_FLAG_C(cpu, op1 > op2);
	}
}

static void exec_alu_add(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 + op2);
}

static void exec_alu_adds(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 + op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op1, op2);
		CPU_SET_FLAG_C(cpu, v < op1);
	}
}

static void exec_alu_adc(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 + op2 + CPU_GET_FLAG_C(cpu));
}

static void exec_alu_adcs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t c = CPU_GET_FLAG_C(cpu);
	uint32_t v = op1 + op2 + c;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op1, op2);
		CPU_SET_FLAG_C(cpu, c ? v <= op1 : v < op1);
	}
}

static void exec_alu_sbc(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 - op2 + CPU_GET_FLAG_C(cpu) - 1);
}

static void exec_alu_sbcs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t c = CPU_GET_FLAG_C(cpu);
	uint32_t v = op1 - op2 + c - 1;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op1, op2);
		CPU_SET_FLAG_C(cpu, c ? op2 >= op1 : op2 > op1);
	}
}

static void exec_alu_rsc(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op2 - op1 + CPU_GET_FLAG_C(cpu) - 1);
}

static void exec_alu_rscs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t c = CPU_GET_FLAG_C(cpu);
	uint32_t v = op2 - op1 + c - 1;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op2, op1);
		CPU_SET_FLAG_C(cpu, op1 + 1 - c > op2);
	}
}

static void exec_alu_tsts(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 & op2;
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_teqs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 ^ op2;
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_cmps(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 - op2;
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op2, op1);
		CPU_SET_FLAG_C(cpu, op2 > op1);
	}
}

static void exec_alu_cmns(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 + op2;
	if (rd != 0xF)
	{
		exec_alu_flags_arithmetical(cpu, v, op2, op1);
		CPU_SET_FLAG_C(cpu, v < op1);
	}
}

static void exec_alu_orr(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 | op2;
	cpu_set_reg(cpu, rd, v);
}

static void exec_alu_orrs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 | op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_mov(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	(void)op1;
	cpu_set_reg(cpu, rd, op2);
}

static void exec_alu_movs(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	(void)op1;
	uint32_t v = op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_bic(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	cpu_set_reg(cpu, rd, op1 & ~op2);
}

static void exec_alu_bics(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	uint32_t v = op1 & ~op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

static void exec_alu_mvn(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	(void)op1;
	cpu_set_reg(cpu, rd, ~op2);
}

static void exec_alu_mvns(cpu_t *cpu, uint32_t rd, uint32_t op1, uint32_t op2)
{
	(void)op1;
	uint32_t v = ~op2;
	cpu_set_reg(cpu, rd, v);
	if (rd != 0xF)
		exec_alu_flags_logical(cpu, v);
}

#define ARM_ALU_DECODEIR(cpu, pcoff) \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t op1r = (cpu->instr_opcode >> 16) & 0xF; \
	uint32_t op1 = cpu->regs.r[op1r]; \
	op1 += op1r == 15 ? pcoff : 0; \
	uint32_t op2r = (cpu->instr_opcode >>  0) & 0xF; \
	uint32_t op2s = cpu->regs.r[op2r]; \
	op2s += op2r == 15 ? pcoff : 0

#define ARM_ALU_DECODEI(cpu) \
	uint8_t shift = (cpu->instr_opcode >> 7) & 0x1F; \
	ARM_ALU_DECODEIR(cpu, 8)

#define ARM_ALU_DECODER(cpu) \
	uint8_t shift = cpu->regs.r[(cpu->instr_opcode >> 8) & 0xF] & 0xFF; \
	ARM_ALU_DECODEIR(cpu, 12)

#define ARM_ALU_DECODEP(cpu) \
	uint8_t shift = (cpu->instr_opcode >> 7) & 0x1F; \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t op1 = (cpu->instr_opcode >> 16) & 0xF; \
	uint32_t op2 = (cpu->instr_opcode >>  0) & 0xF

#define ARM_ALU_OP_ROT(op, rot, rot_name, rot_opi, rot_opr) \
static void exec_##op##_##rot##i(cpu_t *cpu) \
{ \
	ARM_ALU_DECODEI(cpu); \
	rot_opi \
	exec_alu_##op(cpu, rd, op1, op2); \
	if (rd != 0xF) \
		cpu_inc_pc(cpu, 4); \
} \
static void print_##op##_##rot##i(cpu_t *cpu, char *data, size_t size) \
{ \
	ARM_ALU_DECODEP(cpu); \
	snprintf(data, size, "%s r%d, r%d, r%d, " rot_name " #0x%x", #op, rd, op1, op2, shift); \
} \
static const cpu_instr_t arm_##op##_##rot##i = \
{ \
	.name = #op " " #rot "i", \
	.exec = exec_##op##_##rot##i, \
	.print = print_##op##_##rot##i, \
}; \
static void exec_##op##_##rot##r(cpu_t *cpu) \
{ \
	ARM_ALU_DECODER(cpu); \
	rot_opr \
	exec_alu_##op(cpu, rd, op1, op2); \
	if (rd != 0xF) \
		cpu_inc_pc(cpu, 4); \
} \
static void print_##op##_##rot##r(cpu_t *cpu, char *data, size_t size) \
{ \
	ARM_ALU_DECODEP(cpu); \
	snprintf(data, size, "%s r%d, r%d, r%d, " rot_name " r%d", #op, rd, op1, op2, shift); \
} \
static const cpu_instr_t arm_##op##_##rot##r = \
{ \
	.name = #op " " #rot "r", \
	.exec = exec_##op##_##rot##r, \
	.print = print_##op##_##rot##r, \
};\

#define ARM_ALU_LLI(ctest) uint32_t op2 = ARM_LSL(op2s, shift); if (!ctest || shift) { CPU_SET_FLAG_C(cpu, op2s & (1 << (32 - shift))); }
#define ARM_ALU_LLR(ctest) uint32_t op2 = ARM_LSL(op2s, shift); if (!ctest || shift) { CPU_SET_FLAG_C(cpu, op2s & (1 << (32 - shift))); }
#define ARM_ALU_LRI(ctest) uint32_t op2 = ARM_LSR(op2s, shift ? shift : 32);
#define ARM_ALU_LRR(ctest) uint32_t op2 = ARM_LSR(op2s, shift);
#define ARM_ALU_ARI(ctest) uint32_t op2 = ARM_ASR(op2s, shift ? shift : 32);
#define ARM_ALU_ARR(ctest) uint32_t op2 = ARM_ASR(op2s, shift);
#define ARM_ALU_RRI(ctest) \
	uint32_t op2; \
	if (shift == 32) \
	{ \
		op2 = op2s >> 1; \
		op2 |= CPU_GET_FLAG_C(cpu) >> 31; \
		CPU_SET_FLAG_C(cpu, op2s & 0x1 ); \
	} \
	else \
	{ \
		op2 = ARM_ROR(op2s, shift); \
	}
#define ARM_ALU_RRR(ctest) uint32_t op2 = ARM_ROR(op2s, shift);

#define ARM_ALU_OP(op, ctest) \
ARM_ALU_OP_ROT(op, ll, "LSL", ARM_ALU_LLI(ctest), ARM_ALU_LLR(ctest)) \
ARM_ALU_OP_ROT(op, lr, "LSR", ARM_ALU_LRI(ctest), ARM_ALU_LRR(ctest)) \
ARM_ALU_OP_ROT(op, ar, "ASR", ARM_ALU_ARI(ctest), ARM_ALU_ARR(ctest)) \
ARM_ALU_OP_ROT(op, rr, "ROR", ARM_ALU_RRI(ctest), ARM_ALU_RRR(ctest)) \
static void exec_##op##_imm(cpu_t *cpu) \
{ \
	uint32_t shift = (cpu->instr_opcode >> 8) & 0xF; \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t op1 = cpu->regs.r[(cpu->instr_opcode >> 16) & 0xF]; \
	uint32_t op2s = (cpu->instr_opcode >> 0) & 0xFF; \
	uint32_t op2 = ARM_ROR(op2s, shift * 2); \
	exec_alu_##op(cpu, rd, op1, op2); \
	if (rd != 0xF) \
		cpu_inc_pc(cpu, 4); \
} \
static void print_##op##_imm(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t shift = (cpu->instr_opcode >> 8) & 0xF; \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t op1 = (cpu->instr_opcode >> 16) & 0xF; \
	uint32_t op2 = (cpu->instr_opcode >> 0) & 0xFF; \
	snprintf(data, size, "%s r%d, r%d, #0x%x, ROR #0x%x", #op, rd, op1, op2, shift * 2); \
} \
static const cpu_instr_t arm_##op##_imm = \
{ \
	.name = #op " imm", \
	.exec = exec_##op##_imm, \
	.print = print_##op##_imm, \
};

#define ARM_ALU_OPS(op, ctest) \
	ARM_ALU_OP(op   , 0) \
	ARM_ALU_OP(op##s, ctest)

ARM_ALU_OPS(and, 1);
ARM_ALU_OPS(eor, 1);
ARM_ALU_OPS(sub, 0);
ARM_ALU_OPS(rsb, 0);
ARM_ALU_OPS(add, 0);
ARM_ALU_OPS(adc, 0);
ARM_ALU_OPS(sbc, 0);
ARM_ALU_OPS(rsc, 0);
ARM_ALU_OP(tsts, 1);
ARM_ALU_OP(teqs, 1);
ARM_ALU_OP(cmps, 0);
ARM_ALU_OP(cmns, 0);
ARM_ALU_OPS(orr, 1);
ARM_ALU_OPS(mov, 1);
ARM_ALU_OPS(bic, 1);
ARM_ALU_OPS(mvn, 1);

static const cpu_instr_t arm_mul =
{
	.name = "mul",
};

static const cpu_instr_t arm_muls =
{
	.name = "muls",
};

static const cpu_instr_t arm_mla =
{
	.name = "mla",
};

static const cpu_instr_t arm_mlas =
{
	.name = "mlas",
};

static const cpu_instr_t arm_umull =
{
	.name = "umull",
};

static const cpu_instr_t arm_umulls =
{
	.name = "umulls",
};

static const cpu_instr_t arm_umlal =
{
	.name = "umlal",
};

static const cpu_instr_t arm_umlals =
{
	.name = "umlals",
};

static const cpu_instr_t arm_smull =
{
	.name = "smull",
};

static const cpu_instr_t arm_smulls =
{
	.name = "smulls",
};

static const cpu_instr_t arm_smulbb =
{
	.name = "smulbb",
};

static const cpu_instr_t arm_smultb =
{
	.name = "smultb",
};

static const cpu_instr_t arm_smulbt =
{
	.name = "smulbt",
};

static const cpu_instr_t arm_smultt =
{
	.name = "smultt",
};

static const cpu_instr_t arm_smlal =
{
	.name = "smlal",
};

static const cpu_instr_t arm_smlals =
{
	.name = "smlals",
};

static const cpu_instr_t arm_smlalbb =
{
	.name = "smlalbb",
};

static const cpu_instr_t arm_smlaltb =
{
	.name = "smlaltb",
};

static const cpu_instr_t arm_smlalbt =
{
	.name = "smlalbt",
};

static const cpu_instr_t arm_smlaltt =
{
	.name = "smlaltt",
};

static const cpu_instr_t arm_smlabb =
{
	.name = "smlabb",
};

static const cpu_instr_t arm_smlatb =
{
	.name = "smlatb",
};

static const cpu_instr_t arm_smlabt =
{
	.name = "smlabt",
};

static const cpu_instr_t arm_smlatt =
{
	.name = "smlatt",
};

static const cpu_instr_t arm_smulwb =
{
	.name = "smulwb",
};

static const cpu_instr_t arm_smlawt =
{
	.name = "smlawt",
};

static const cpu_instr_t arm_smlawb =
{
	.name = "smlawb",
};

static const cpu_instr_t arm_smulwt =
{
	.name = "smulwt",
};

#define STLD_ROT_LL offset = ARM_LSL(rm, shift)
#define STLD_ROT_LR offset = ARM_LSR(rm, shift ? shift : 32)
#define STLD_ROT_AR offset = ARM_ASR(rm, shift ? shift : 32)
#define STLD_ROT_RR offset = ARM_ROR(rm, shift)
#define STLD_ROT_NO offset = rm

#define STLD_BTBT(opname, oparg, rotname, imm_reg, post_pre, dec_inc, byte_word, writeback_mm, st_ld, regshift) \
static void exec_##opname##_##oparg####rotname(cpu_t *cpu) \
{ \
	uint32_t rnr = (cpu->instr_opcode >> 16) & 0xF; \
	uint32_t rn = cpu_get_reg(cpu, rnr); \
	rn += (rnr == 15) ? 8 : 0; \
	uint32_t rdr = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t offset; \
	if (imm_reg) \
	{ \
		uint8_t shift = (cpu->instr_opcode >> 7) & 0x1F; \
		(void)shift; \
		uint8_t rm = cpu->instr_opcode & 0xF; \
		regshift; \
	} \
	else \
	{ \
		offset = cpu->instr_opcode & 0xFFF; \
	} \
	if (post_pre) \
	{ \
		if (dec_inc) \
			rn += offset; \
		else \
			rn -= offset; \
		if (writeback_mm) \
			cpu_set_reg(cpu, rnr, rn); \
	} \
	if (st_ld) \
	{ \
		uint32_t v; \
		if (byte_word) \
			v = mem_get32(cpu->mem, rn); \
		else \
			v = mem_get8(cpu->mem, rn); \
		cpu_set_reg(cpu, rdr, v); \
	} \
	else \
	{ \
		uint32_t rd = cpu_get_reg(cpu, rdr); \
		rd += (rdr == 15) ? 12 : 0; \
		if (byte_word) \
			mem_set32(cpu->mem, rn, rd); \
		else \
			mem_set8(cpu->mem, rn, rd); \
	} \
	if (!post_pre && (!st_ld || rdr != rnr)) \
	{ \
		if (dec_inc) \
			rn += offset; \
		else \
			rn -= offset; \
		cpu_set_reg(cpu, rnr, rn); \
	} \
	if (rdr != 15) \
	{ \
		cpu_inc_pc(cpu, 4); \
		cpu->instr_delay = 3; \
	} \
	else \
	{ \
		cpu->instr_delay = 5; \
	} \
} \
static void print_##opname##_##oparg####rotname(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t rn = (cpu->instr_opcode >> 16) & 0xF; \
	if (imm_reg) \
	{ \
		uint8_t shift = (cpu->instr_opcode >> 7) & 0x1F; \
		uint8_t rm = cpu->instr_opcode & 0xF; \
		if (post_pre) \
			snprintf(data, size, #opname " r%d, [r%d, r%d," #rotname " #0x%x]!", rd, rn, rm, shift); \
		else \
			snprintf(data, size, #opname " r%d, [r%d], r%d, " #rotname " #0x%x", rd, rn, rm, shift); \
	} \
	else \
	{ \
		uint32_t offset = cpu->instr_opcode & 0xFFF; \
		if (post_pre) \
			snprintf(data, size, #opname " r%d, [r%d, #0x%x]!", rd, rn, offset); \
		else \
			snprintf(data, size, #opname " r%d, [r%d], #0x%x", rd, rn, offset); \
	} \
} \
static const cpu_instr_t arm_##opname##_##oparg####rotname = \
{ \
	.name = #opname " " #oparg #rotname, \
	.exec = exec_##opname##_##oparg####rotname, \
	.print = print_##opname##_##oparg####rotname, \
}

#define STLD_HDST(opname, oparg, post_pre, dec_inc, reg_imm, writeback, st_ld, op) \
static void exec_##opname##_##oparg(cpu_t *cpu) \
{ \
	uint32_t rnr = (cpu->instr_opcode >> 16) & 0xF; \
	uint32_t rn = cpu_get_reg(cpu, rnr); \
	rn += (rnr == 15) ? 8 : 0; \
	uint32_t rdr = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t offset; \
	int post_update = 1; \
	if (reg_imm) \
		offset = ((cpu->instr_opcode & 0xF00) >> 4) | (cpu->instr_opcode & 0xF); \
	else \
		offset = cpu_get_reg(cpu, cpu->instr_opcode & 0xF); \
	if (post_pre) \
	{ \
		if (dec_inc) \
			rn += offset; \
		else \
			rn -= offset; \
		if (writeback) \
			cpu_set_reg(cpu, rnr, rn); \
	} \
	if (st_ld) \
	{ \
		if (op == 1) \
			cpu_set_reg(cpu, rdr, mem_get16(cpu->mem, rn + 2 * offset)); \
		else if (op == 2) \
			cpu_set_reg(cpu, rdr, (int8_t)mem_get8(cpu->mem, rn + offset)); \
		else if (op == 3) \
			cpu_set_reg(cpu, rdr, (int16_t)mem_get16(cpu->mem, rn + 2 * offset)); \
		post_update = (rdr != rnr); \
	} \
	else \
	{ \
		uint32_t rd = cpu_get_reg(cpu, rdr); \
		rd += (rdr == 15) ? 12 : 0; \
		if (op == 1) \
		{ \
			mem_set16(cpu->mem, rn + 2 * offset, rd); \
		} \
		else if (op == 2) \
		{ \
			uint32_t rdr2 = (rdr + 1) & 0xF; \
			cpu_set_reg(cpu, rdr , mem_get32(cpu->mem, rn + 2 * offset + 0)); \
			cpu_set_reg(cpu, rdr2, mem_get32(cpu->mem, rn + 2 * offset + 4)); \
			post_update = (rdr != rnr && rdr2 != rnr); \
		} \
		else if (op == 3) \
		{ \
			uint32_t rdr2 = (rdr + 1) & 0xF; \
			uint32_t rd2 = cpu_get_reg(cpu, rdr2); \
			rd2 += (rdr2 == 15) ? 12 : 0; \
			mem_set32(cpu->mem, rn + 2 * offset + 0, rd); \
			mem_set32(cpu->mem, rn + 2 * offset + 4, rd2); \
		} \
	} \
	if (!post_pre && post_update) \
	{ \
		if (dec_inc) \
			rn += offset; \
		else \
			rn -= offset; \
		cpu_set_reg(cpu, rnr, rn); \
	} \
	if (rdr != 15) \
	{ \
		cpu_inc_pc(cpu, 4); \
		cpu->instr_delay = 3; \
	} \
	else \
	{ \
		cpu->instr_delay = 5; \
	} \
}; \
static void print_##opname##_##oparg(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	uint32_t rn = (cpu->instr_opcode >> 16) & 0xF; \
	if (reg_imm) \
	{ \
		uint32_t offset = ((cpu->instr_opcode & 0xF00) >> 4) | (cpu->instr_opcode & 0xF); \
		if (post_pre) \
			snprintf(data, size, #opname " r%d, [r%d, #0x%x]!", rd, rn, offset); \
		else \
			snprintf(data, size, #opname " r%d, [r%d], #0x%x", rd, rn, offset); \
	} \
	else \
	{ \
		uint32_t offset = cpu->instr_opcode & 0xF; \
		if (post_pre) \
			snprintf(data, size, #opname " r%d, [r%d, r%d]!", rd, rn, offset); \
		else \
			snprintf(data, size, #opname " r%d, [r%d], r%d", rd, rn, offset); \
	} \
} \
static const cpu_instr_t arm_##opname##_##oparg = \
{ \
	.name = #opname " " #oparg, \
	.exec = exec_##opname##_##oparg, \
	.print = print_##opname##_##oparg, \
}

#define STLD_PTR(v, dec_inc) \
STLD_BTBT(str  , v, ll, 1, 0, dec_inc, 0, 0, 0, STLD_ROT_LL); \
STLD_BTBT(str  , v, lr, 1, 0, dec_inc, 0, 0, 0, STLD_ROT_LR); \
STLD_BTBT(str  , v, ar, 1, 0, dec_inc, 0, 0, 0, STLD_ROT_AR); \
STLD_BTBT(str  , v, rr, 1, 0, dec_inc, 0, 0, 0, STLD_ROT_RR); \
STLD_HDST(strh, v, 0, dec_inc, 0, 0, 0, 1); \
STLD_HDST(strd, v, 0, dec_inc, 0, 0, 0, 3); \
STLD_BTBT(strb , v, ll, 1, 0, dec_inc, 1, 0, 0, STLD_ROT_LL); \
STLD_BTBT(strb , v, lr, 1, 0, dec_inc, 1, 0, 0, STLD_ROT_LR); \
STLD_BTBT(strb , v, ar, 1, 0, dec_inc, 1, 0, 0, STLD_ROT_AR); \
STLD_BTBT(strb , v, rr, 1, 0, dec_inc, 1, 0, 0, STLD_ROT_RR); \
STLD_BTBT(strt , v, ll, 1, 0, dec_inc, 0, 1, 0, STLD_ROT_LL); \
STLD_BTBT(strt , v, lr, 1, 0, dec_inc, 0, 1, 0, STLD_ROT_LR); \
STLD_BTBT(strt , v, ar, 1, 0, dec_inc, 0, 1, 0, STLD_ROT_AR); \
STLD_BTBT(strt , v, rr, 1, 0, dec_inc, 0, 1, 0, STLD_ROT_RR); \
STLD_BTBT(strbt, v, ll, 1, 0, dec_inc, 1, 1, 0, STLD_ROT_LL); \
STLD_BTBT(strbt, v, lr, 1, 0, dec_inc, 1, 1, 0, STLD_ROT_LR); \
STLD_BTBT(strbt, v, ar, 1, 0, dec_inc, 1, 1, 0, STLD_ROT_AR); \
STLD_BTBT(strbt, v, rr, 1, 0, dec_inc, 1, 1, 0, STLD_ROT_RR); \
STLD_BTBT(ldr  , v, ll, 1, 0, dec_inc, 0, 0, 1, STLD_ROT_LL); \
STLD_BTBT(ldr  , v, lr, 1, 0, dec_inc, 0, 0, 1, STLD_ROT_LR); \
STLD_BTBT(ldr  , v, ar, 1, 0, dec_inc, 0, 0, 1, STLD_ROT_AR); \
STLD_BTBT(ldr  , v, rr, 1, 0, dec_inc, 0, 0, 1, STLD_ROT_RR); \
STLD_HDST(ldrh, v, 0, dec_inc, 0, 0, 1, 1); \
STLD_HDST(ldrd, v, 0, dec_inc, 0, 0, 0, 2); \
STLD_BTBT(ldrb , v, ll, 1, 0, dec_inc, 1, 0, 1, STLD_ROT_LL); \
STLD_BTBT(ldrb , v, lr, 1, 0, dec_inc, 1, 0, 1, STLD_ROT_LR); \
STLD_BTBT(ldrb , v, ar, 1, 0, dec_inc, 1, 0, 1, STLD_ROT_AR); \
STLD_BTBT(ldrb , v, rr, 1, 0, dec_inc, 1, 0, 1, STLD_ROT_RR); \
STLD_BTBT(ldrt , v, ll, 1, 0, dec_inc, 0, 1, 1, STLD_ROT_LL); \
STLD_BTBT(ldrt , v, lr, 1, 0, dec_inc, 0, 1, 1, STLD_ROT_LR); \
STLD_BTBT(ldrt , v, ar, 1, 0, dec_inc, 0, 1, 1, STLD_ROT_AR); \
STLD_BTBT(ldrt , v, rr, 1, 0, dec_inc, 0, 1, 1, STLD_ROT_RR); \
STLD_BTBT(ldrbt, v, ll, 1, 0, dec_inc, 1, 1, 1, STLD_ROT_LL); \
STLD_BTBT(ldrbt, v, lr, 1, 0, dec_inc, 1, 1, 1, STLD_ROT_LR); \
STLD_BTBT(ldrbt, v, ar, 1, 0, dec_inc, 1, 1, 1, STLD_ROT_AR); \
STLD_BTBT(ldrbt, v, rr, 1, 0, dec_inc, 1, 1, 1, STLD_ROT_RR); \
STLD_HDST(ldrsb, v, 0, dec_inc, 0, 0, 1, 2); \
STLD_HDST(ldrsh, v, 0, dec_inc, 0, 0, 1, 3)

STLD_PTR(ptrm, 0);
STLD_PTR(ptrp, 1);

#define STLD_PTI(v, dec_inc) \
STLD_BTBT(str  , v,  , 0, 0, dec_inc, 0, 0, 0, STLD_ROT_NO); \
STLD_HDST(strh , v, 0, dec_inc, 1, 0, 0, 1); \
STLD_HDST(strd , v, 0, dec_inc, 1, 0, 0, 3); \
STLD_BTBT(strb , v,  , 0, 0, dec_inc, 1, 0, 0, STLD_ROT_NO); \
STLD_BTBT(strt , v,  , 0, 0, dec_inc, 0, 1, 0, STLD_ROT_NO); \
STLD_BTBT(strbt, v,  , 0, 0, dec_inc, 1, 1, 0, STLD_ROT_NO); \
STLD_BTBT(ldr  , v,  , 0, 0, dec_inc, 0, 0, 1, STLD_ROT_NO); \
STLD_HDST(ldrh , v, 0, dec_inc, 1, 0, 1, 1); \
STLD_HDST(ldrd , v, 0, dec_inc, 1, 0, 0, 2); \
STLD_BTBT(ldrb , v,  , 0, 0, dec_inc, 1, 0, 1, STLD_ROT_NO); \
STLD_BTBT(ldrt , v,  , 0, 0, dec_inc, 0, 1, 1, STLD_ROT_NO); \
STLD_BTBT(ldrbt, v,  , 0, 0, dec_inc, 1, 1, 1, STLD_ROT_NO); \
STLD_HDST(ldrsb, v, 0, dec_inc, 1, 0, 1, 2); \
STLD_HDST(ldrsh, v, 0, dec_inc, 1, 0, 1, 3)

STLD_PTI(ptim, 0);
STLD_PTI(ptip, 1);

#define STLD_R(v, dec_inc, writeback) \
STLD_BTBT(str  , v, ll, 1, 1, dec_inc, 0, writeback, 0, STLD_ROT_LL); \
STLD_BTBT(str  , v, lr, 1, 1, dec_inc, 0, writeback, 0, STLD_ROT_LR); \
STLD_BTBT(str  , v, ar, 1, 1, dec_inc, 0, writeback, 0, STLD_ROT_AR); \
STLD_BTBT(str  , v, rr, 1, 1, dec_inc, 0, writeback, 0, STLD_ROT_RR); \
STLD_HDST(strh, v, 1, dec_inc, 0, writeback, 0, 1); \
STLD_HDST(strd, v, 1, dec_inc, 0, writeback, 0, 3); \
STLD_BTBT(strb , v, ll, 1, 1, dec_inc, 1, writeback, 0, STLD_ROT_LL); \
STLD_BTBT(strb , v, lr, 1, 1, dec_inc, 1, writeback, 0, STLD_ROT_LR); \
STLD_BTBT(strb , v, ar, 1, 1, dec_inc, 1, writeback, 0, STLD_ROT_AR); \
STLD_BTBT(strb , v, rr, 1, 1, dec_inc, 1, writeback, 0, STLD_ROT_RR); \
STLD_BTBT(ldr  , v, ll, 1, 1, dec_inc, 0, writeback, 1, STLD_ROT_LL); \
STLD_BTBT(ldr  , v, lr, 1, 1, dec_inc, 0, writeback, 1, STLD_ROT_LR); \
STLD_BTBT(ldr  , v, ar, 1, 1, dec_inc, 0, writeback, 1, STLD_ROT_AR); \
STLD_BTBT(ldr  , v, rr, 1, 1, dec_inc, 0, writeback, 1, STLD_ROT_RR); \
STLD_HDST(ldrh, v, 1, dec_inc, 0, writeback, 1, 1); \
STLD_HDST(ldrd, v, 1, dec_inc, 0, writeback, 0, 2); \
STLD_BTBT(ldrb , v, ll, 1, 1, dec_inc, 1, writeback, 1, STLD_ROT_LL); \
STLD_BTBT(ldrb , v, lr, 1, 1, dec_inc, 1, writeback, 1, STLD_ROT_LR); \
STLD_BTBT(ldrb , v, ar, 1, 1, dec_inc, 1, writeback, 1, STLD_ROT_AR); \
STLD_BTBT(ldrb , v, rr, 1, 1, dec_inc, 1, writeback, 1, STLD_ROT_RR); \
STLD_HDST(ldrsb, v, 1, dec_inc, 0, writeback, 1, 2); \
STLD_HDST(ldrsh, v, 1, dec_inc, 0, writeback, 1, 3)

STLD_R(ofrm, 0, 0);
STLD_R(ofrp, 1, 0);
STLD_R(prrm, 0, 1);
STLD_R(prrp, 1, 1);

#define STLD_I(v, dec_inc, writeback) \
STLD_BTBT(str  , v,  , 0, 1, dec_inc, 0, writeback, 0, STLD_ROT_NO); \
STLD_HDST(strh , v, 1, dec_inc, 1, writeback, 0, 1); \
STLD_HDST(strd , v, 1, dec_inc, 1, writeback, 0, 3); \
STLD_BTBT(strb , v,  , 0, 1, dec_inc, 1, writeback, 0, STLD_ROT_NO); \
STLD_BTBT(ldr  , v,  , 0, 1, dec_inc, 0, writeback, 1, STLD_ROT_NO); \
STLD_HDST(ldrh , v, 1, dec_inc, 1, writeback, 1, 1); \
STLD_HDST(ldrd , v, 1, dec_inc, 1, writeback, 0, 2); \
STLD_BTBT(ldrb , v,  , 0, 1, dec_inc, 1, writeback, 1, STLD_ROT_NO); \
STLD_HDST(ldrsb, v, 1, dec_inc, 1, writeback, 1, 2); \
STLD_HDST(ldrsh, v, 1, dec_inc, 1, writeback, 1, 3)

STLD_I(ofim, 0, 0);
STLD_I(ofip, 1, 0);
STLD_I(prim, 0, 1);
STLD_I(prip, 1, 1);

#define STLD_MI(id, ab) \
static const cpu_instr_t arm_stm##id##ab = \
{ \
	.name = "stm" #id #ab, \
}; \
static const cpu_instr_t arm_ldm##id##ab = \
{ \
	.name = "ldm" #id #ab, \
}; \
static const cpu_instr_t arm_stm##id##ab##_w = \
{ \
	.name = "stm" #id #ab "_w", \
}; \
static const cpu_instr_t arm_ldm##id##ab##_w = \
{ \
	.name = "ldm" #id #ab "_w", \
}; \
static const cpu_instr_t arm_stm##id##ab##_u = \
{ \
	.name = "stm" #id #ab "_u", \
}; \
static const cpu_instr_t arm_ldm##id##ab##_u = \
{ \
	.name = "ldm" #id #ab "_u", \
}; \
static const cpu_instr_t arm_stm##id##ab##_uw = \
{ \
	.name = "stm" #id #ab "_uw", \
}; \
static const cpu_instr_t arm_ldm##id##ab##_uw = \
{ \
	.name = "ldm" #id #ab "_uw", \
};

STLD_MI(d, a);
STLD_MI(i, a);
STLD_MI(d, b);
STLD_MI(i, b);

#define STLDC(v) \
static const cpu_instr_t arm_stc_##v = \
{ \
	.name = "stc " #v, \
}; \
static const cpu_instr_t arm_ldc_##v = \
{ \
	.name = "ldc " #v, \
};

STLDC(ofm);
STLDC(prm);
STLDC(ofp);
STLDC(prp);
STLDC(unm);
STLDC(ptm);
STLDC(unp);
STLDC(ptp);

static const cpu_instr_t arm_clz =
{
	.name = "clz",
};

static const cpu_instr_t arm_bkpt =
{
	.name = "bkpt",
};

#define ARM_MRS(n, psr) \
static void exec_mrs_##n(cpu_t *cpu) \
{ \
	uint32_t v; \
	if (psr) \
		v = cpu->regs.cpsr; \
	else \
		v = *cpu->regs.spsr; \
	uint32_t rd = (cpu->instr_opcode >> 12) & 0xF; \
	cpu_set_reg(cpu, rd, v); \
	cpu_inc_pc(cpu,  4); \
	cpu->instr_delay = 1; \
} \
static const cpu_instr_t arm_mrs_##n = \
{ \
	.name = "mrs " #n, \
	.exec = exec_mrs_##n, \
}

ARM_MRS(rc, 0);
ARM_MRS(rs, 1);

#define ARM_MSR(n, imm, psr) \
static void exec_msr_##n(cpu_t *cpu) \
{ \
	uint32_t v; \
	if (imm) \
		v = ARM_ROR(cpu->instr_opcode & 0xF, ((cpu->instr_opcode >> 8) & 0xF) * 2); \
	else \
		v = cpu_get_reg(cpu, cpu->instr_opcode & 0xF); \
	uint32_t mask = 0; \
	if (cpu->instr_opcode & (1 << 19)) \
		mask |= 0xFF000000; \
	if (cpu->instr_opcode & (1 << 18)) \
		mask |= 0x00FF0000; \
	if (cpu->instr_opcode & (1 << 17)) \
		mask |= 0x0000FF00; \
	if (cpu->instr_opcode & (1 << 16)) \
		mask |= 0x000000FF; \
	mask &= ~CPU_FLAG_T; \
	if (psr) \
		*cpu->regs.spsr = (*cpu->regs.spsr & ~mask) | (v & mask); \
	else \
		cpu->regs.cpsr = (cpu->regs.cpsr & ~mask) | (v & mask); \
	cpu_inc_pc(cpu, 4); \
	cpu_update_mode(cpu); \
	cpu->instr_delay = 1; \
} \
static void print_msr_##n(cpu_t *cpu, char *data, size_t size) \
{ \
	char flags[5]; \
	memset(flags, 0, sizeof(flags)); \
	size_t fn = 0; \
	if (cpu->instr_opcode & (1 << 19)) \
		flags[fn++] = 'f'; \
	if (cpu->instr_opcode & (1 << 18)) \
		flags[fn++] = 's'; \
	if (cpu->instr_opcode & (1 << 17)) \
		flags[fn++] = 'x'; \
	if (cpu->instr_opcode & (1 << 16)) \
		flags[fn++] = 'c'; \
	if (imm) \
		snprintf(data, size, "msr %s_%s #0x%x, ROR#0x%x", psr ? "spsr" : "cpsr", flags, cpu->instr_opcode & 0xF, ((cpu->instr_opcode >> 8) & 0xF) * 2); \
	else \
		snprintf(data, size, "msr %s_%s r%d", psr ? "spsr" : "cpsr", flags, cpu->instr_opcode & 0xF); \
} \
static const cpu_instr_t arm_msr_##n = \
{ \
	.name = "msr " #n, \
	.exec = exec_msr_##n, \
	.print = print_msr_##n, \
}

ARM_MSR(rc, 0, 0);
ARM_MSR(rs, 0, 1);
ARM_MSR(ic, 1, 0);
ARM_MSR(is, 1, 1);

static const cpu_instr_t arm_qadd =
{
	.name = "qadd",
};

static const cpu_instr_t arm_qsub =
{
	.name = "qsub",
};

static const cpu_instr_t arm_qdadd =
{
	.name = "qdadd",
};

static const cpu_instr_t arm_qdsub =
{
	.name = "qdsub",
};

static const cpu_instr_t arm_swp =
{
	.name = "swp",
};

static const cpu_instr_t arm_swpb =
{
	.name = "swpb",
};

static void exec_b(cpu_t *cpu)
{
	int32_t v = cpu->instr_opcode & 0x7FFFFF;
	if (cpu->instr_opcode & 0x800000)
		v = -(~v & 0x7FFFFF);
	cpu_inc_pc(cpu, 8 + 4 * v);
	cpu->instr_delay = 3;
}

static void print_b(cpu_t *cpu, char *data, size_t size)
{
	int32_t v = cpu->instr_opcode & 0x7FFFFF;
	if (cpu->instr_opcode & 0x800000)
		v = -(~v & 0x7FFFFF);
	snprintf(data, size, "b %c0x%x", v < 0 ? '-' : '+', v < 0 ? -v : v);
}

static const cpu_instr_t arm_b =
{
	.name = "b",
	.exec = exec_b,
	.print = print_b,
};

static void exec_bl(cpu_t *cpu)
{
	int32_t v = cpu->instr_opcode & 0x7FFFFF;
	if (cpu->instr_opcode & 0x800000)
		v = -(~v & 0x7FFFFF);
	cpu_set_reg(cpu, 14, cpu_get_reg(cpu, 15) + 4);
	cpu_inc_pc(cpu, 8 + 4 * v);
	cpu->instr_delay = 3;
}

static void print_bl(cpu_t *cpu, char *data, size_t size)
{
	int32_t v = cpu->instr_opcode & 0x7FFFFF;
	if (cpu->instr_opcode & 0x800000)
		v = -(~v & 0x7FFFFF);
	snprintf(data, size, "bl %c0x%x", v < 0 ? '-' : '+', v < 0 ? -v : v);
}

static const cpu_instr_t arm_bl =
{
	.name = "bl",
	.exec = exec_bl,
	.print = print_bl,
};

static void exec_bx(cpu_t *cpu)
{
	uint8_t rn = cpu->instr_opcode & 0xF;
	cpu_set_reg(cpu, rn, cpu_get_reg(cpu, rn) | 1);
	CPU_SET_FLAG_T(cpu, 1);
	cpu_inc_pc(cpu, 4);
}

static const cpu_instr_t arm_bx =
{
	.name = "bx",
	.exec = exec_bx,
};

static const cpu_instr_t arm_blx_reg =
{
	.name = "blx reg",
};

static const cpu_instr_t arm_cdp =
{
	.name = "cdp",
};

static const cpu_instr_t arm_mcr =
{
	.name = "mcr",
};

static const cpu_instr_t arm_mrc =
{
	.name = "mrc",
};

static const cpu_instr_t arm_swi =
{
	.name = "swi",
};

static const cpu_instr_t arm_undef =
{
	.name = "undef",
};

#define REPEAT1(v) &arm_##v
#define REPEAT2(v) REPEAT1(v), REPEAT1(v)
#define REPEAT4(v) REPEAT2(v), REPEAT2(v)
#define REPEAT8(v) REPEAT4(v), REPEAT4(v)
#define REPEAT16(v) REPEAT8(v), REPEAT8(v)
#define REPEAT32(v) REPEAT16(v), REPEAT16(v)
#define REPEAT64(v) REPEAT32(v), REPEAT32(v)
#define REPEAT128(v) REPEAT64(v), REPEAT64(v)
#define REPEAT256(v) REPEAT128(v), REPEAT128(v)

#define ALU_OP(op, v9, vB, vD, vF) \
	REPEAT1(op##_lli), REPEAT1(op##_llr), \
	REPEAT1(op##_lri), REPEAT1(op##_lrr), \
	REPEAT1(op##_ari), REPEAT1(op##_arr), \
	REPEAT1(op##_rri), REPEAT1(op##_rrr), \
	REPEAT1(op##_lli), REPEAT1(v9), \
	REPEAT1(op##_lri), REPEAT1(vB), \
	REPEAT1(op##_ari), REPEAT1(vD), \
	REPEAT1(op##_rri), REPEAT1(vF)

#define ALU_OPS(op, v09, v0B, v0D, v0F, v19, v1B, v1D, v1F) \
	ALU_OP(op, v09, v0B, v0D, v0F), \
	ALU_OP(op##s, v19, v1B, v1D, v1F)

#define CDP_LINE(v) \
	REPEAT1(cdp), REPEAT1(v), REPEAT1(cdp), REPEAT1(v), \
	REPEAT1(cdp), REPEAT1(v), REPEAT1(cdp), REPEAT1(v), \
	REPEAT1(cdp), REPEAT1(v), REPEAT1(cdp), REPEAT1(v), \
	REPEAT1(cdp), REPEAT1(v), REPEAT1(cdp), REPEAT1(v)

#define STLD_BLANK(v) \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##lr), REPEAT1(undef), \
	REPEAT1(v##ar), REPEAT1(undef), \
	REPEAT1(v##rr), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##lr), REPEAT1(undef), \
	REPEAT1(v##ar), REPEAT1(undef), \
	REPEAT1(v##rr), REPEAT1(undef)

const cpu_instr_t *cpu_instr_arm[0x1000] =
{
	/* 0x000 */ ALU_OPS(and, mul  , strh_ptrm, ldrd_ptrm, strd_ptrm, muls  , ldrh_ptrm, ldrsb_ptrm, ldrsh_ptrm),
	/* 0x020 */ ALU_OPS(eor, mla  , strh_ptrm, ldrd_ptrm, strd_ptrm, mlas  , ldrh_ptrm, ldrsb_ptrm, ldrsh_ptrm),
	/* 0x040 */ ALU_OPS(sub, undef, strh_ptim, ldrd_ptim, strd_ptim, undef , ldrh_ptim, ldrsb_ptim, ldrsh_ptim),
	/* 0x060 */ ALU_OPS(rsb, undef, strh_ptim, ldrd_ptim, strd_ptim ,undef , ldrh_ptim, ldrsb_ptim, ldrsh_ptim),
	/* 0x080 */ ALU_OPS(add, umull, strh_ptrp, ldrd_ptrp, strd_ptrp, umulls, ldrh_ptrp, ldrsb_ptrp, ldrsh_ptrp),
	/* 0x0A0 */ ALU_OPS(adc, umlal, strh_ptrp, ldrd_ptrp, strd_ptrp, umlals, ldrh_ptrp, ldrsb_ptrp, ldrsh_ptrp),
	/* 0x0C0 */ ALU_OPS(sbc, smull, strh_ptip, ldrd_ptip, strd_ptip, smulls, ldrh_ptip, ldrsb_ptip, ldrsh_ptip),
	/* 0x0E0 */ ALU_OPS(rsc, smlal, strh_ptip, ldrd_ptip, strd_ptip, smlals, ldrh_ptip, ldrsb_ptip, ldrsh_ptip),
	/* 0x100 */ REPEAT1(mrs_rc) , REPEAT1(undef)    , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x104 */ REPEAT1(undef)  , REPEAT1(qadd)     , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x108 */ REPEAT1(smlabb) , REPEAT1(swp)      , REPEAT1(smlatb) , REPEAT1(strh_ofrm),
	/* 0x10C */ REPEAT1(smlabt) , REPEAT1(ldrd_ofrm), REPEAT1(smlatt) , REPEAT1(strd_ofrm),
	/* 0x110 */ ALU_OP(tsts, undef, ldrh_ofrm, ldrsb_ofrm, ldrsh_ofrm),
	/* 0x120 */ REPEAT1(msr_rc) , REPEAT1(bx)       , REPEAT1(undef)  , REPEAT1(blx_reg),
	/* 0x124 */ REPEAT1(undef)  , REPEAT1(qsub)     , REPEAT1(undef)  , REPEAT1(bkpt),
	/* 0x128 */ REPEAT1(smlawb) , REPEAT1(undef)    , REPEAT1(smulwb) , REPEAT1(strh_prrm),
	/* 0x12C */ REPEAT1(smlawt) , REPEAT1(ldrd_prrm), REPEAT1(smulwt) , REPEAT1(strd_prrm),
	/* 0x130 */ ALU_OP(teqs, undef, ldrh_prrm, ldrsb_prrm, ldrsh_prrm),
	/* 0x140 */ REPEAT1(mrs_rs) , REPEAT1(undef)    , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x144 */ REPEAT1(undef)  , REPEAT1(qdadd)    , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x148 */ REPEAT1(smlalbb), REPEAT1(swpb)     , REPEAT1(smlaltb), REPEAT1(strh_ofim),
	/* 0x14C */ REPEAT1(smlalbt), REPEAT1(ldrd_ofim), REPEAT1(smlaltt), REPEAT1(strd_ofim),
	/* 0x150 */ ALU_OP(cmps, undef, ldrh_ofim, ldrsb_ofim, ldrsh_ofim),
	/* 0x160 */ REPEAT1(msr_rs) , REPEAT1(clz)      , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x164 */ REPEAT1(undef)  , REPEAT1(qdsub)    , REPEAT1(undef)  , REPEAT1(undef),
	/* 0x168 */ REPEAT1(smulbb) , REPEAT1(undef)    , REPEAT1(smultb) , REPEAT1(strh_prim),
	/* 0x16C */ REPEAT1(smulbt) , REPEAT1(ldrd_prim), REPEAT1(smultt) , REPEAT1(strd_prim),
	/* 0x170 */ ALU_OP(cmns, undef, ldrh_prim, ldrsb_prim, ldrsh_prim),
	/* 0x180 */ ALU_OPS(orr, undef, strh_ofrp, ldrd_ofrp, strd_ofrp, undef, ldrh_ofrp, ldrsb_ofrp, ldrsh_ofrp),
	/* 0x1A0 */ ALU_OPS(mov, undef, strh_prrp, ldrd_prrp, strd_prrp, undef, ldrh_prrp, ldrsb_prrp, ldrsh_prrp),
	/* 0x1C0 */ ALU_OPS(bic, undef, strh_ofip, ldrd_ofip, strd_ofip, undef, ldrh_ofip, ldrsb_ofip, ldrsh_ofip),
	/* 0x1E0 */ ALU_OPS(mvn, undef, strh_prip, ldrd_prip, strd_prip, undef, ldrh_prip, ldrsb_prip, ldrsh_prip),
	/* 0x200 */ REPEAT16(and_imm),
	/* 0x210 */ REPEAT16(ands_imm),
	/* 0x220 */ REPEAT16(eor_imm),
	/* 0x230 */ REPEAT16(eors_imm),
	/* 0x240 */ REPEAT16(sub_imm),
	/* 0x250 */ REPEAT16(subs_imm),
	/* 0x260 */ REPEAT16(rsb_imm),
	/* 0x270 */ REPEAT16(rsbs_imm),
	/* 0x280 */ REPEAT16(add_imm),
	/* 0x290 */ REPEAT16(adds_imm),
	/* 0x2A0 */ REPEAT16(adc_imm),
	/* 0x2B0 */ REPEAT16(adcs_imm),
	/* 0x2C0 */ REPEAT16(sbc_imm),
	/* 0x2D0 */ REPEAT16(sbcs_imm),
	/* 0x2E0 */ REPEAT16(rsc_imm),
	/* 0x2F0 */ REPEAT16(rscs_imm),
	/* 0x300 */ REPEAT16(undef),
	/* 0x310 */ REPEAT16(tsts_imm),
	/* 0x320 */ REPEAT16(msr_ic),
	/* 0x330 */ REPEAT16(teqs_imm),
	/* 0x340 */ REPEAT16(undef),
	/* 0x350 */ REPEAT16(cmps_imm),
	/* 0x360 */ REPEAT16(msr_is),
	/* 0x370 */ REPEAT16(cmns_imm),
	/* 0x380 */ REPEAT16(orr_imm),
	/* 0x390 */ REPEAT16(orrs_imm),
	/* 0x3A0 */ REPEAT16(mov_imm),
	/* 0x3B0 */ REPEAT16(movs_imm),
	/* 0x3C0 */ REPEAT16(bic_imm),
	/* 0x3D0 */ REPEAT16(bics_imm),
	/* 0x3E0 */ REPEAT16(mvn_imm),
	/* 0x3F0 */ REPEAT16(mvns_imm),
	/* 0x400 */ REPEAT16(str_ptim),
	/* 0x410 */ REPEAT16(ldr_ptim),
	/* 0x420 */ REPEAT16(strt_ptim),
	/* 0x430 */ REPEAT16(ldrt_ptim),
	/* 0x440 */ REPEAT16(strb_ptim),
	/* 0x450 */ REPEAT16(ldrb_ptim),
	/* 0x460 */ REPEAT16(strbt_ptim),
	/* 0x470 */ REPEAT16(ldrbt_ptim),
	/* 0x480 */ REPEAT16(str_ptip),
	/* 0x490 */ REPEAT16(ldr_ptip),
	/* 0x4A0 */ REPEAT16(strt_ptip),
	/* 0x4B0 */ REPEAT16(ldrt_ptip),
	/* 0x4C0 */ REPEAT16(strb_ptip),
	/* 0x4D0 */ REPEAT16(ldrb_ptip),
	/* 0x4E0 */ REPEAT16(strbt_ptip),
	/* 0x4F0 */ REPEAT16(ldrbt_ptip),
	/* 0x500 */ REPEAT16(str_ofim),
	/* 0x510 */ REPEAT16(ldr_ofim),
	/* 0x520 */ REPEAT16(str_prim),
	/* 0x530 */ REPEAT16(ldr_prim),
	/* 0x540 */ REPEAT16(strb_ofim),
	/* 0x550 */ REPEAT16(ldrb_ofim),
	/* 0x560 */ REPEAT16(strb_prim),
	/* 0x570 */ REPEAT16(ldrb_prim),
	/* 0x580 */ REPEAT16(str_ofip),
	/* 0x590 */ REPEAT16(ldr_ofip),
	/* 0x5A0 */ REPEAT16(str_prip),
	/* 0x5B0 */ REPEAT16(ldr_prip),
	/* 0x5C0 */ REPEAT16(strb_ofip),
	/* 0x5D0 */ REPEAT16(ldrb_ofip),
	/* 0x5E0 */ REPEAT16(strb_prip),
	/* 0x5F0 */ REPEAT16(ldrb_prip),
	/* 0x600 */ STLD_BLANK(str_ptrm),
	/* 0x610 */ STLD_BLANK(ldr_ptrm),
	/* 0x620 */ STLD_BLANK(strt_ptrm),
	/* 0x630 */ STLD_BLANK(ldrt_ptrm),
	/* 0x640 */ STLD_BLANK(strb_ptrm),
	/* 0x650 */ STLD_BLANK(ldrb_ptrm),
	/* 0x660 */ STLD_BLANK(strbt_ptrm),
	/* 0x670 */ STLD_BLANK(ldrbt_ptrm),
	/* 0x680 */ STLD_BLANK(str_ptrp),
	/* 0x690 */ STLD_BLANK(ldr_ptrp),
	/* 0x6A0 */ STLD_BLANK(strt_ptrp),
	/* 0x6B0 */ STLD_BLANK(ldrt_ptrp),
	/* 0x6C0 */ STLD_BLANK(strb_ptrp),
	/* 0x6D0 */ STLD_BLANK(ldrb_ptrp),
	/* 0x6E0 */ STLD_BLANK(strbt_ptrp),
	/* 0x6F0 */ STLD_BLANK(ldrbt_ptrp),
	/* 0x700 */ STLD_BLANK(str_ofrm),
	/* 0x710 */ STLD_BLANK(ldr_ofrm),
	/* 0x720 */ STLD_BLANK(str_prrm),
	/* 0x730 */ STLD_BLANK(ldr_prrm),
	/* 0x740 */ STLD_BLANK(strb_ofrm),
	/* 0x750 */ STLD_BLANK(ldrb_ofrm),
	/* 0x760 */ STLD_BLANK(strb_prrm),
	/* 0x770 */ STLD_BLANK(ldrb_prrm),
	/* 0x780 */ STLD_BLANK(str_ofrp),
	/* 0x790 */ STLD_BLANK(ldr_ofrp),
	/* 0x7A0 */ STLD_BLANK(str_prrp),
	/* 0x7B0 */ STLD_BLANK(ldr_prrp),
	/* 0x7C0 */ STLD_BLANK(strb_ofrp),
	/* 0x7D0 */ STLD_BLANK(ldrb_ofrp),
	/* 0x7E0 */ STLD_BLANK(strb_prrp),
	/* 0x7F0 */ STLD_BLANK(ldrb_prrp),
	/* 0x800 */ REPEAT16(stmda),
	/* 0x810 */ REPEAT16(ldmda),
	/* 0x820 */ REPEAT16(stmda_w),
	/* 0x830 */ REPEAT16(ldmda_w),
	/* 0x840 */ REPEAT16(stmda_u),
	/* 0x850 */ REPEAT16(ldmda_u),
	/* 0x860 */ REPEAT16(stmda_uw),
	/* 0x870 */ REPEAT16(ldmda_uw),
	/* 0x880 */ REPEAT16(stmia),
	/* 0x890 */ REPEAT16(ldmia),
	/* 0x8A0 */ REPEAT16(stmia_w),
	/* 0x8B0 */ REPEAT16(ldmia_w),
	/* 0x8C0 */ REPEAT16(stmia_u),
	/* 0x8D0 */ REPEAT16(ldmia_u),
	/* 0x8E0 */ REPEAT16(stmia_uw),
	/* 0x8F0 */ REPEAT16(ldmia_uw),
	/* 0x900 */ REPEAT16(stmdb),
	/* 0x910 */ REPEAT16(ldmdb),
	/* 0x920 */ REPEAT16(stmdb_w),
	/* 0x930 */ REPEAT16(ldmdb_w),
	/* 0x940 */ REPEAT16(stmdb_u),
	/* 0x950 */ REPEAT16(ldmdb_u),
	/* 0x960 */ REPEAT16(stmdb_uw),
	/* 0x970 */ REPEAT16(ldmdb_uw),
	/* 0x980 */ REPEAT16(stmib),
	/* 0x990 */ REPEAT16(ldmib),
	/* 0x9A0 */ REPEAT16(stmib_w),
	/* 0x9B0 */ REPEAT16(ldmib_w),
	/* 0x9C0 */ REPEAT16(stmib_u),
	/* 0x9D0 */ REPEAT16(ldmib_u),
	/* 0x9E0 */ REPEAT16(stmib_uw),
	/* 0x9F0 */ REPEAT16(ldmib_uw),
	/* 0xA00 */ REPEAT256(b),
	/* 0xB00 */ REPEAT256(bl),
	/* 0xC00 */ REPEAT16(stc_ofm),
	/* 0xC10 */ REPEAT16(ldc_ofm),
	/* 0xC20 */ REPEAT16(stc_prm),
	/* 0xC30 */ REPEAT16(ldc_prm),
	/* 0xC40 */ REPEAT16(stc_ofm),
	/* 0xC50 */ REPEAT16(ldc_ofm),
	/* 0xC60 */ REPEAT16(stc_prm),
	/* 0xC70 */ REPEAT16(ldc_prm),
	/* 0xC80 */ REPEAT16(stc_ofp),
	/* 0xC90 */ REPEAT16(ldc_ofp),
	/* 0xCA0 */ REPEAT16(stc_prp),
	/* 0xCB0 */ REPEAT16(ldc_prp),
	/* 0xCC0 */ REPEAT16(stc_ofp),
	/* 0xCD0 */ REPEAT16(ldc_ofp),
	/* 0xCE0 */ REPEAT16(stc_prp),
	/* 0xCF0 */ REPEAT16(ldc_prp),
	/* 0xD00 */ REPEAT16(stc_unm),
	/* 0xD10 */ REPEAT16(ldc_unm),
	/* 0xD20 */ REPEAT16(stc_ptm),
	/* 0xD30 */ REPEAT16(ldc_ptm),
	/* 0xD40 */ REPEAT16(stc_unm),
	/* 0xD50 */ REPEAT16(ldc_unm),
	/* 0xD60 */ REPEAT16(stc_ptm),
	/* 0xD70 */ REPEAT16(ldc_ptm),
	/* 0xD80 */ REPEAT16(stc_unp),
	/* 0xD90 */ REPEAT16(ldc_unp),
	/* 0xDA0 */ REPEAT16(stc_ptp),
	/* 0xDB0 */ REPEAT16(ldc_ptp),
	/* 0xDC0 */ REPEAT16(stc_unp),
	/* 0xDD0 */ REPEAT16(ldc_unp),
	/* 0xDE0 */ REPEAT16(stc_ptp),
	/* 0xDF0 */ REPEAT16(ldc_ptp),
	/* 0xE00 */ CDP_LINE(mcr),
	/* 0xE10 */ CDP_LINE(mrc),
	/* 0xE20 */ CDP_LINE(mcr),
	/* 0xE30 */ CDP_LINE(mrc),
	/* 0xE40 */ CDP_LINE(mcr),
	/* 0xE50 */ CDP_LINE(mrc),
	/* 0xE60 */ CDP_LINE(mcr),
	/* 0xE70 */ CDP_LINE(mrc),
	/* 0xE80 */ CDP_LINE(mcr),
	/* 0xE90 */ CDP_LINE(mrc),
	/* 0xEA0 */ CDP_LINE(mcr),
	/* 0xEB0 */ CDP_LINE(mrc),
	/* 0xEC0 */ CDP_LINE(mcr),
	/* 0xED0 */ CDP_LINE(mrc),
	/* 0xEE0 */ CDP_LINE(mcr),
	/* 0xEF0 */ CDP_LINE(mrc),
	/* 0xF00 */ REPEAT256(swi),
};
