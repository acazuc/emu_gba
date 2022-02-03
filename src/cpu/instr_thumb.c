#include "instr_thumb.h"
#include "../cpu.h"
#include "../mem.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define THUMB_LSL(v, s) ((v) << (s))
#define THUMB_LSR(v, s) ((v) >> (s))
#define THUMB_ASR(v, s) ((int32_t)(v) >> (s))
#define THUMB_ROR(v, s) (((v) >> (s)) | ((v) << (32 - (s))))

#define THUMB_SHIFTED(n, shiftop) \
static void exec_##n##_imm(cpu_t *cpu) \
{ \
	uint32_t shift = (cpu->instr_opcode >> 6) & 0x1F; \
	uint32_t rsr = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t rdr = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rss = cpu_get_reg(cpu, rsr); \
	shiftop; \
	cpu_set_reg(cpu, rdr, rs); \
	CPU_SET_FLAG_Z(cpu, !rs); \
	CPU_SET_FLAG_N(cpu, rs & 0x80000000); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 1; \
} \
static void print_##n##_imm(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t shift = (cpu->instr_opcode >> 6) & 0x1F; \
	uint32_t rsr = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t rdr = (cpu->instr_opcode >> 0) & 0x7; \
	snprintf(data, size, #n " r%d, r%d, #0x%x", rdr, rsr, shift); \
} \
static const cpu_instr_t thumb_##n##_imm = \
{ \
	.name = #n " imm", \
	.exec = exec_##n##_imm, \
	.print = print_##n##_imm, \
}

#define THUMB_SHIFTED_LSL uint32_t rs = THUMB_LSL(rss, shift); if (shift) { CPU_SET_FLAG_C(cpu, rs & (1 << (32 - shift))); }
#define THUMB_SHIFTED_LSR shift = shift ? shift : 32; uint32_t rs = THUMB_LSR(rss, shift); CPU_SET_FLAG_C(cpu, rss >> (shift - 1));
#define THUMB_SHIFTED_ASR shift = shift ? shift : 32; uint32_t rs = THUMB_ASR(rss, shift); CPU_SET_FLAG_C(cpu, rss >> (shift - 1));

THUMB_SHIFTED(lsl, THUMB_SHIFTED_LSL);
THUMB_SHIFTED(lsr, THUMB_SHIFTED_LSR);
THUMB_SHIFTED(asr, THUMB_SHIFTED_ASR);

#define THUMB_ADDSUBR(n, v, add_sub, reg_imm) \
static void exec_##n##_##v(cpu_t *cpu) \
{ \
	uint32_t rsr = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t rdr = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rs = cpu_get_reg(cpu, rsr); \
	uint32_t val; \
	if (reg_imm) \
		val = (cpu->instr_opcode >> 6) & 0x7; \
	else \
		val = cpu_get_reg(cpu, (cpu->instr_opcode >> 6) & 0x7); \
	if (add_sub) \
	{ \
		uint32_t res = rs - val; \
		cpu_set_reg(cpu, rdr, res); \
		CPU_SET_FLAG_N(cpu, res & 0x80000000); \
		CPU_SET_FLAG_Z(cpu, !res); \
		CPU_SET_FLAG_C(cpu, val > rs); \
		CPU_SET_FLAG_V(cpu, ((rs ^ val) & 0x80000000) && ((rs ^ res) & 0x80000000)); \
	} \
	else \
	{ \
		uint32_t res = rs + val; \
		cpu_set_reg(cpu, rdr, res); \
		CPU_SET_FLAG_N(cpu, res & 0x80000000); \
		CPU_SET_FLAG_Z(cpu, !res); \
		CPU_SET_FLAG_C(cpu, res < rs); \
		CPU_SET_FLAG_V(cpu, ((rs ^ val) & 0x80000000) && ((rs ^ res) & 0x80000000)); \
	} \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 1; \
} \
static void print_##n##_##v(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rsr = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t rdr = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t val = (cpu->instr_opcode >> 6) & 0x7; \
	if (reg_imm) \
		snprintf(data, size, #n " r%d, r%d, #0x%x", rdr, rsr, val); \
	else \
		snprintf(data, size, #n " r%d, r%d, r%d", rdr, rsr, val); \
} \
static const cpu_instr_t thumb_##n##_##v = \
{ \
	.name = #n " " #v, \
	.exec = exec_##n##_##v, \
	.print = print_##n##_##v, \
}

THUMB_ADDSUBR(add, reg, 0, 0);
THUMB_ADDSUBR(sub, reg, 1, 0);
THUMB_ADDSUBR(add, imm, 0, 1);
THUMB_ADDSUBR(sub, imm, 1, 1);

static void mcas_mov(cpu_t *cpu, uint32_t r, uint32_t nn)
{
	cpu_set_reg(cpu, r, nn);
	CPU_SET_FLAG_Z(cpu, !nn);
	CPU_SET_FLAG_N(cpu, nn & 0x80000000);
}

static void mcas_cmp(cpu_t *cpu, uint32_t r, uint32_t nn)
{
	uint32_t rv = cpu_get_reg(cpu, r);
	uint32_t res = rv - nn;
	CPU_SET_FLAG_Z(cpu, !res);
	CPU_SET_FLAG_N(cpu, res & 0x80000000);
	CPU_SET_FLAG_C(cpu, nn > rv);
	CPU_SET_FLAG_V(cpu, ((rv ^ nn) & 0x80000000) && ((rv ^ res) & 0x80000000));
}

static void mcas_add(cpu_t *cpu, uint32_t r, uint32_t nn)
{
	uint32_t rv = cpu_get_reg(cpu, r);
	uint32_t res = rv + nn;
	cpu_set_reg(cpu, r, res);
	CPU_SET_FLAG_Z(cpu, !res);
	CPU_SET_FLAG_N(cpu, res & 0x80000000);
	CPU_SET_FLAG_C(cpu, res < nn);
	CPU_SET_FLAG_V(cpu, ((rv ^ nn) & 0x80000000) && ((rv ^ res) & 0x80000000));
}

static void mcas_sub(cpu_t *cpu, uint32_t r, uint32_t nn)
{
	uint32_t rv = cpu_get_reg(cpu, r);
	uint32_t res = rv - nn;
	cpu_set_reg(cpu, r, res);
	CPU_SET_FLAG_Z(cpu, !res);
	CPU_SET_FLAG_N(cpu, res & 0x80000000);
	CPU_SET_FLAG_C(cpu, nn > rv);
	CPU_SET_FLAG_V(cpu, ((rv ^ nn) & 0x80000000) && ((rv ^ res) & 0x80000000));
}

#define THUMB_MCAS_I8R(n, r) \
static void exec_##n##_i8r##r(cpu_t *cpu) \
{ \
	uint32_t nn = cpu->instr_opcode & 0xFF; \
	mcas_##n(cpu, r, nn); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 1; \
} \
static void print_##n##_i8r##r(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t nn = cpu->instr_opcode & 0xFF; \
	snprintf(data, size, #n " r%d, #0x%x", r, nn); \
} \
static const cpu_instr_t thumb_##n##_i8r##r = \
{ \
	.name = #n " i8r" #r, \
	.exec = exec_##n##_i8r##r, \
	.print = print_##n##_i8r##r, \
}

THUMB_MCAS_I8R(mov, 0);
THUMB_MCAS_I8R(mov, 1);
THUMB_MCAS_I8R(mov, 2);
THUMB_MCAS_I8R(mov, 3);
THUMB_MCAS_I8R(mov, 4);
THUMB_MCAS_I8R(mov, 5);
THUMB_MCAS_I8R(mov, 6);
THUMB_MCAS_I8R(mov, 7);
THUMB_MCAS_I8R(cmp, 0);
THUMB_MCAS_I8R(cmp, 1);
THUMB_MCAS_I8R(cmp, 2);
THUMB_MCAS_I8R(cmp, 3);
THUMB_MCAS_I8R(cmp, 4);
THUMB_MCAS_I8R(cmp, 5);
THUMB_MCAS_I8R(cmp, 6);
THUMB_MCAS_I8R(cmp, 7);
THUMB_MCAS_I8R(add, 0);
THUMB_MCAS_I8R(add, 1);
THUMB_MCAS_I8R(add, 2);
THUMB_MCAS_I8R(add, 3);
THUMB_MCAS_I8R(add, 4);
THUMB_MCAS_I8R(add, 5);
THUMB_MCAS_I8R(add, 6);
THUMB_MCAS_I8R(add, 7);
THUMB_MCAS_I8R(sub, 0);
THUMB_MCAS_I8R(sub, 1);
THUMB_MCAS_I8R(sub, 2);
THUMB_MCAS_I8R(sub, 3);
THUMB_MCAS_I8R(sub, 4);
THUMB_MCAS_I8R(sub, 5);
THUMB_MCAS_I8R(sub, 6);
THUMB_MCAS_I8R(sub, 7);

static void alu_flags_logical(cpu_t *cpu, uint32_t v)
{
	CPU_SET_FLAG_N(cpu, v & 0x80000000);
	CPU_SET_FLAG_Z(cpu, !v);
}

static void alu_flags_arithmetical(cpu_t *cpu, uint32_t v, uint32_t op1, uint32_t op2)
{
	CPU_SET_FLAG_V(cpu, ((op1 ^ op2) & 0x80000000) && ((op1 ^ v) & 0x80000000));
	CPU_SET_FLAG_N(cpu, v & 0x80000000);
	CPU_SET_FLAG_Z(cpu, !v);
}

static void alu_and(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t v = cpu_get_reg(cpu, rd) & rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

static void alu_eor(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t v = cpu_get_reg(cpu, rd) ^ rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

static void alu_lsr(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = THUMB_LSR(reg, rs);
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
	if (rs)
		CPU_SET_FLAG_C(cpu, reg & (1 << (rs - 1)));
}

static void alu_lsl(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = THUMB_LSL(reg, rs);
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
	if (rs)
		CPU_SET_FLAG_C(cpu, reg & (1 << (32 - rs)));
}

static void alu_asr(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = THUMB_ASR(reg, rs);
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
	if (rs)
		CPU_SET_FLAG_C(cpu, reg & (1 << (32 - rs)));
}

static void alu_adc(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t c = CPU_GET_FLAG_C(cpu);
	uint32_t v = reg + rs + c;
	cpu_set_reg(cpu, rd, v);
	alu_flags_arithmetical(cpu, v, reg, rs);
	CPU_SET_FLAG_C(cpu, c ? v <= reg : v < reg);
}

static void alu_sbc(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t c = CPU_GET_FLAG_C(cpu);
	uint32_t v = reg - rs + c - 1;
	cpu_set_reg(cpu, rd, v);
	alu_flags_arithmetical(cpu, v, reg, rs);
	CPU_SET_FLAG_C(cpu, c ? rs >= reg : rs > reg);
}

static void alu_ror(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = THUMB_ROR(reg, rs);
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
	if (rs)
		CPU_SET_FLAG_C(cpu, reg & (1 << (rs - 1)));
}

static void alu_tst(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg & rs;
	alu_flags_logical(cpu, v);
}

static void alu_neg(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t v = -rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_arithmetical(cpu, v, 0, rs);
	CPU_SET_FLAG_C(cpu, rs > 0);
}

static void alu_cmp(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg - rs;
	alu_flags_arithmetical(cpu, v, reg, rs);
	CPU_SET_FLAG_C(cpu, rs > reg);
}

static void alu_cmn(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg + rs;
	alu_flags_arithmetical(cpu, v, reg, rs);
	CPU_SET_FLAG_C(cpu, v < reg);
}

static void alu_orr(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg | rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

static void alu_mul(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg * rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

static void alu_bic(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t reg = cpu_get_reg(cpu, rd);
	uint32_t v = reg & ~rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

static void alu_mvn(cpu_t *cpu, uint32_t rd, uint32_t rs)
{
	uint32_t v = ~rs;
	cpu_set_reg(cpu, rd, v);
	alu_flags_logical(cpu, v);
}

#define THUMB_ALU(n) \
static void exec_alu_##n(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rs = (cpu->instr_opcode >> 3) & 0x7; \
	alu_##n(cpu, rd, rs); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 1; \
} \
static void print_alu_##n(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rs = (cpu->instr_opcode >> 3) & 0x7; \
	snprintf(data, size, #n " r%d, r%d", rd, rs); \
} \
static const cpu_instr_t thumb_alu_##n = \
{ \
	.name = #n, \
	.exec = exec_alu_##n, \
	.print = print_alu_##n, \
}

THUMB_ALU(and);
THUMB_ALU(eor);
THUMB_ALU(lsr);
THUMB_ALU(lsl);
THUMB_ALU(asr);
THUMB_ALU(adc);
THUMB_ALU(sbc);
THUMB_ALU(ror);
THUMB_ALU(tst);
THUMB_ALU(neg);
THUMB_ALU(cmp);
THUMB_ALU(cmn);
THUMB_ALU(orr);
THUMB_ALU(mul);
THUMB_ALU(bic);
THUMB_ALU(mvn);

static void hi_addh(cpu_t *cpu, uint32_t rd, uint32_t rdr, uint32_t rs)
{
	cpu_set_reg(cpu, rdr, rd + rs);
	if (rd == CPU_REG_PC)
		cpu->instr_delay += 2;
}

static void hi_cmph(cpu_t *cpu, uint32_t rd, uint32_t rdr, uint32_t rs)
{
	(void)rdr;
	uint32_t res = rd - rs;
	alu_flags_arithmetical(cpu, res, rd, rs);
	CPU_SET_FLAG_C(cpu, rs > rd);
}

static void hi_movh(cpu_t *cpu, uint32_t rd, uint32_t rdr, uint32_t rs)
{
	(void)rd;
	cpu_set_reg(cpu, rdr, rs);
	if (rd == CPU_REG_PC)
		cpu->instr_delay += 2;
}

#define THUMB_HI(n) \
static void exec_##n(cpu_t *cpu) \
{ \
	uint32_t msbd = (cpu->instr_opcode >> 7) & 0x1; \
	uint32_t msbs = (cpu->instr_opcode >> 6) & 0x1; \
	uint32_t rdr = ((cpu->instr_opcode >> 0) & 0x7) | (msbd << 3); \
	uint32_t rsr = ((cpu->instr_opcode >> 3) & 0x7) | (msbs << 3); \
	uint32_t rd = cpu_get_reg(cpu, rdr); \
	rd += (rdr == CPU_REG_PC) ? 4 : 0; \
	uint32_t rs = cpu_get_reg(cpu, rsr); \
	rs += (rsr == CPU_REG_PC) ? 4 : 0; \
	hi_##n(cpu, rd, rdr, rs); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay++; \
} \
static void print_##n(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t msbd = (cpu->instr_opcode >> 7) & 0x1; \
	uint32_t msbs = (cpu->instr_opcode >> 6) & 0x1; \
	uint32_t rd = ((cpu->instr_opcode >> 0) & 0x7) | (msbd << 3); \
	uint32_t rs = ((cpu->instr_opcode >> 3) & 0x7) | (msbs << 3); \
	snprintf(data, size, #n " r%d, r%d", rd, rs); \
} \
static const cpu_instr_t thumb_##n = \
{ \
	.name = #n, \
	.exec = exec_##n, \
	.print = print_##n, \
}

THUMB_HI(addh);
THUMB_HI(cmph);
THUMB_HI(movh);

static void exec_bx(cpu_t *cpu)
{
	uint32_t msbs = (cpu->instr_opcode >> 6) & 0x1;
	uint32_t rsr = ((cpu->instr_opcode >> 3) & 0x7) | (msbs << 3);
	uint32_t rs = cpu_get_reg(cpu, rsr);
	rs += (rsr == CPU_REG_PC) ? 4 : 0;
	if (!(rs & 1))
	{
		CPU_SET_FLAG_T(cpu, 0);
		rs &= ~3;
	}
	cpu_set_reg(cpu, CPU_REG_PC, rs);
	cpu->instr_delay += 3;
}

static void print_bx(cpu_t *cpu, char *data, size_t size)
{
	uint32_t msbs = (cpu->instr_opcode >> 6) & 0x1;
	uint32_t rs = ((cpu->instr_opcode >> 3) & 0x7) | (msbs << 3);
	snprintf(data, size, "bx r%d", rs);
}

static const cpu_instr_t thumb_bx =
{
	.name = "bx",
	.exec = exec_bx,
	.print = print_bx,
};

#define THUMB_LDRPC_R(r) \
static void exec_ldrpc_r##r(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = cpu->instr_opcode & 0xFF; \
	uint32_t v = mem_get32(cpu->mem, ((cpu_get_reg(cpu, 15) + 4) & ~2) + nn * 4); \
	cpu_set_reg(cpu, rd, v); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 3; \
} \
static void print_ldrpc_r##r(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = cpu->instr_opcode & 0xFF; \
	snprintf(data, size, "ldr r%d, [pc, #0x%x]", rd, nn * 4); \
} \
static const cpu_instr_t thumb_ldrpc_r##r = \
{ \
	.name = "ldrpc r" #r, \
	.exec = exec_ldrpc_r##r, \
	.print = print_ldrpc_r##r, \
}

THUMB_LDRPC_R(0);
THUMB_LDRPC_R(1);
THUMB_LDRPC_R(2);
THUMB_LDRPC_R(3);
THUMB_LDRPC_R(4);
THUMB_LDRPC_R(5);
THUMB_LDRPC_R(6);
THUMB_LDRPC_R(7);

static void stld_str(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	mem_set32(cpu->mem, rb + ro, cpu_get_reg(cpu, rd));
}

static void stld_strh(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	mem_set16(cpu->mem, rb + ro, cpu_get_reg(cpu, rd));
}

static void stld_strb(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	mem_set8(cpu->mem, rb + ro, cpu_get_reg(cpu, rd));
}

static void stld_ldr(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	cpu_set_reg(cpu, rd, mem_get32(cpu->mem, rb + ro));
}

static void stld_ldrh(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	cpu_set_reg(cpu, rd, mem_get16(cpu->mem, rb + ro));
}

static void stld_ldrb(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	cpu_set_reg(cpu, rd, mem_get8(cpu->mem, rb + ro));
}

static void stld_ldrsh(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	cpu_set_reg(cpu, rd, (int16_t)mem_get8(cpu->mem, rb + ro));
}

static void stld_ldrsb(cpu_t *cpu, uint32_t rd, uint32_t rb, uint32_t ro)
{
	cpu_set_reg(cpu, rd, (int8_t)mem_get8(cpu->mem, rb + ro));
}

#define THUMB_STLD_REG(n, st_ld) \
static void exec_##n##_reg(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rb = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t ro = (cpu->instr_opcode >> 6) & 0x7; \
	stld_##n(cpu, rd, cpu_get_reg(cpu, rb), cpu_get_reg(cpu, ro)); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = st_ld ? 3 : 2; \
} \
static void print_##n##_reg(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rb = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t ro = (cpu->instr_opcode >> 6) & 0x7; \
	snprintf(data, size, #n " r%d, [r%d, r%d]", rd, rb, ro); \
} \
static const cpu_instr_t thumb_##n##_reg = \
{ \
	.name = #n " reg", \
	.exec = exec_##n##_reg, \
	.print = print_##n##_reg, \
}

THUMB_STLD_REG(str  , 0);
THUMB_STLD_REG(strh , 0);
THUMB_STLD_REG(strb , 0);
THUMB_STLD_REG(ldr  , 1);
THUMB_STLD_REG(ldrh , 1);
THUMB_STLD_REG(ldrb , 1);
THUMB_STLD_REG(ldrsh, 1);
THUMB_STLD_REG(ldrsb, 1);

#define THUMB_STLD_IMM(n, st_ld, nn_mult) \
static void exec_##n##_imm(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rb = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t nn = (cpu->instr_opcode >> 6) & 0x7; \
	nn *= nn_mult; \
	stld_##n(cpu, rd, cpu_get_reg(cpu, rb), nn); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = st_ld ? 3 : 2; \
} \
static void print_##n##_imm(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 0) & 0x7; \
	uint32_t rb = (cpu->instr_opcode >> 3) & 0x7; \
	uint32_t nn = (cpu->instr_opcode >> 6) & 0x7; \
	snprintf(data, size, #n " r%d, [r%d, #0x%x]", rd, rb, nn * nn_mult); \
} \
static const cpu_instr_t thumb_##n##_imm = \
{ \
	.name = #n " imm", \
	.exec = exec_##n##_imm, \
	.print = print_##n##_imm, \
}

THUMB_STLD_IMM(str , 0, 4);
THUMB_STLD_IMM(strh, 0, 2);
THUMB_STLD_IMM(strb, 0, 1);
THUMB_STLD_IMM(ldr , 1, 4);
THUMB_STLD_IMM(ldrh, 1, 2);
THUMB_STLD_IMM(ldrb, 1, 1);

#define THUMB_STLDSP_R(n, r, st_ld) \
static void exec_##n##sp_r##r(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = ((cpu->instr_opcode >> 0) & 0xFF) * 4; \
	if (st_ld) \
		cpu_set_reg(cpu, rd, mem_get32(cpu->mem, cpu_get_reg(cpu, CPU_REG_SP) + nn)); \
	else \
		mem_set32(cpu->mem, cpu_get_reg(cpu, CPU_REG_SP) + nn, cpu_get_reg(cpu, rd)); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = st_ld ? 3 : 2; \
} \
static void print_##n##sp_r##r(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = (cpu->instr_opcode >> 0) & 0xFF; \
	snprintf(data, size, "str r%d, [sp, #0x%x]", rd, nn * 4); \
} \
static const cpu_instr_t thumb_##n##sp_r##r = \
{ \
	.name = #n "sp r" #r, \
	.exec = exec_##n##sp_r##r, \
	.print = print_##n##sp_r##r, \
}

THUMB_STLDSP_R(str, 0, 0);
THUMB_STLDSP_R(str, 1, 0);
THUMB_STLDSP_R(str, 2, 0);
THUMB_STLDSP_R(str, 3, 0);
THUMB_STLDSP_R(str, 4, 0);
THUMB_STLDSP_R(str, 5, 0);
THUMB_STLDSP_R(str, 6, 0);
THUMB_STLDSP_R(str, 7, 0);
THUMB_STLDSP_R(ldr, 0, 1);
THUMB_STLDSP_R(ldr, 1, 1);
THUMB_STLDSP_R(ldr, 2, 1);
THUMB_STLDSP_R(ldr, 3, 1);
THUMB_STLDSP_R(ldr, 4, 1);
THUMB_STLDSP_R(ldr, 5, 1);
THUMB_STLDSP_R(ldr, 6, 1);
THUMB_STLDSP_R(ldr, 7, 1);

#define THUMB_ADDPCSP_R(rr, r, pc_sp) \
static void exec_add##rr##_r##r(cpu_t *cpu) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = ((cpu->instr_opcode >> 0) & 0xFF) * 4; \
	if (pc_sp) \
		cpu_set_reg(cpu, rd, cpu_get_reg(cpu, CPU_REG_SP) + nn); \
	else \
		cpu_set_reg(cpu, rd, ((cpu_get_reg(cpu, CPU_REG_PC) + 4) & ~2) + nn); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay = 1; \
} \
static void print_add##rr##_r##r(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t rd = (cpu->instr_opcode >> 8) & 0x7; \
	uint32_t nn = (cpu->instr_opcode >> 0) & 0xFF; \
	snprintf(data, size, "add r%d, " #rr ", #0x%x", rd, nn * 4); \
} \
static const cpu_instr_t thumb_add##rr##_r##r = \
{ \
	.name = "add" #rr " r" #r, \
	.exec = exec_add##rr##_r##r, \
	.print = print_add##rr##_r##r, \
}

THUMB_ADDPCSP_R(pc, 0, 0);
THUMB_ADDPCSP_R(pc, 1, 0);
THUMB_ADDPCSP_R(pc, 2, 0);
THUMB_ADDPCSP_R(pc, 3, 0);
THUMB_ADDPCSP_R(pc, 4, 0);
THUMB_ADDPCSP_R(pc, 5, 0);
THUMB_ADDPCSP_R(pc, 6, 0);
THUMB_ADDPCSP_R(pc, 7, 0);
THUMB_ADDPCSP_R(sp, 0, 1);
THUMB_ADDPCSP_R(sp, 1, 1);
THUMB_ADDPCSP_R(sp, 2, 1);
THUMB_ADDPCSP_R(sp, 3, 1);
THUMB_ADDPCSP_R(sp, 4, 1);
THUMB_ADDPCSP_R(sp, 5, 1);
THUMB_ADDPCSP_R(sp, 6, 1);
THUMB_ADDPCSP_R(sp, 7, 1);

static void exec_addsp_imm(cpu_t *cpu)
{
	uint32_t add_sub = (cpu->instr_opcode >> 7) & 0x1;
	uint32_t nn = (cpu->instr_opcode >> 0) & 0x7F;
	if (add_sub)
		cpu_set_reg(cpu, CPU_REG_SP, cpu_get_reg(cpu, CPU_REG_SP) - nn * 4);
	else
		cpu_set_reg(cpu, CPU_REG_SP, cpu_get_reg(cpu, CPU_REG_SP) + nn * 4);
	cpu_inc_pc(cpu, 2);
	cpu->instr_delay = 1;
}

static void print_addsp_imm(cpu_t *cpu, char *data, size_t size)
{
	uint32_t add_sub = (cpu->instr_opcode >> 7) & 0x1;
	uint32_t nn = (cpu->instr_opcode >> 0) & 0x7F;
	snprintf(data, size, "%s sp, #0x%x", add_sub ? "sub" : "add", nn * 4);
}

static const cpu_instr_t thumb_addsp_imm =
{
	.name = "addsp imm",
	.exec = exec_addsp_imm,
	.print = print_addsp_imm,
};

#define THUMB_PUSHPOP(n, next, post_pre, down_up, st_ld, ext_reg, base_reg) \
static void exec_##n####next(cpu_t *cpu) \
{ \
	uint32_t rl = cpu->instr_opcode & 0xFF; \
	uint32_t sp = cpu_get_reg(cpu, base_reg); \
	if (!down_up) \
	{ \
		uint32_t nregs = 0; \
		for (int i = 0; i < 8; ++i) \
		{ \
			if (rl & (1 << i)) \
				nregs++; \
		} \
		if (ext_reg) \
			nregs++; \
		sp -= 4 * nregs; \
		cpu_set_reg(cpu, base_reg, sp); \
	} \
	for (int i = 0; i < 8; ++i) \
	{ \
		if (!(rl & (1 << i))) \
			continue; \
		if (post_pre == down_up) \
			sp += 4; \
		if (st_ld) \
			cpu_set_reg(cpu, i, mem_get32(cpu->mem, sp)); \
		else \
			mem_set32(cpu->mem, sp, cpu_get_reg(cpu, i)); \
		if (post_pre != down_up) \
			sp += 4; \
		cpu->instr_delay++; \
	} \
	if (ext_reg) \
	{ \
		if (post_pre == down_up) \
			sp += 4; \
		if (st_ld) \
			cpu_set_reg(cpu, CPU_REG_PC, mem_get32(cpu->mem, sp)); \
		else \
			mem_set32(cpu->mem, sp, cpu_get_reg(cpu, CPU_REG_LR)); \
		if (post_pre != down_up) \
			sp += 4; \
		cpu->instr_delay++; \
	} \
	if (down_up) \
		cpu_set_reg(cpu, base_reg, sp); \
	cpu_inc_pc(cpu, 2); \
	cpu->instr_delay++; \
} \
static void print_##n####next(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t regs = cpu->instr_opcode & 0xFF; \
	char  *tmpd = data; \
	size_t tmps = size; \
	snprintf(data, size, #n " "); \
	tmpd += (1 + strlen(#n)); \
	tmps -= (1 + strlen(#n)); \
	if (base_reg != CPU_REG_SP) \
	{ \
		snprintf(tmpd, tmps, "r%d!, ", base_reg); \
		tmpd += 5; \
		tmps -= 5; \
	} \
	snprintf(tmpd, tmps, "{"); \
	tmpd++; \
	tmps--; \
	if (regs) \
	{ \
		for (int i = 0; i < 8; ++i) \
		{ \
			if (!(regs & (1 << i))) \
				continue; \
			snprintf(tmpd, tmps, "r%d,", i); \
			tmpd += 3; \
			tmps -= 3; \
		} \
		tmps++; \
		tmpd--; \
	} \
	if (ext_reg) \
	{ \
		if (regs) \
		{ \
			tmps--; \
			tmpd++; \
		} \
		if (st_ld) \
		{ \
			snprintf(tmpd, tmps, "r%d", CPU_REG_PC); \
			tmpd += 3; \
			tmps -= 3; \
		} \
		else \
		{ \
			snprintf(tmpd, tmps, "r%d", CPU_REG_LR); \
			tmpd += 3; \
			tmps -= 3; \
		} \
	} \
	snprintf(tmpd, tmps, "}"); \
} \
static const cpu_instr_t thumb_##n####next = \
{ \
	.name = #n, \
	.exec = exec_##n####next, \
	.print = print_##n####next, \
}

THUMB_PUSHPOP(push ,    , 1, 0, 0, 0, CPU_REG_SP);
THUMB_PUSHPOP(pop  ,    , 0, 1, 1, 0, CPU_REG_SP);
THUMB_PUSHPOP(push , _lr, 1, 0, 0, 1, CPU_REG_SP);
THUMB_PUSHPOP(pop  , _pc, 0, 1, 1, 1, CPU_REG_SP);
THUMB_PUSHPOP(stmia, _r0, 0, 1, 0, 0, 0);
THUMB_PUSHPOP(stmia, _r1, 0, 1, 0, 0, 1);
THUMB_PUSHPOP(stmia, _r2, 0, 1, 0, 0, 2);
THUMB_PUSHPOP(stmia, _r3, 0, 1, 0, 0, 3);
THUMB_PUSHPOP(stmia, _r4, 0, 1, 0, 0, 4);
THUMB_PUSHPOP(stmia, _r5, 0, 1, 0, 0, 5);
THUMB_PUSHPOP(stmia, _r6, 0, 1, 0, 0, 6);
THUMB_PUSHPOP(stmia, _r7, 0, 1, 0, 0, 7);
THUMB_PUSHPOP(ldmia, _r0, 0, 1, 1, 0, 0);
THUMB_PUSHPOP(ldmia, _r1, 0, 1, 1, 0, 1);
THUMB_PUSHPOP(ldmia, _r2, 0, 1, 1, 0, 2);
THUMB_PUSHPOP(ldmia, _r3, 0, 1, 1, 0, 3);
THUMB_PUSHPOP(ldmia, _r4, 0, 1, 1, 0, 4);
THUMB_PUSHPOP(ldmia, _r5, 0, 1, 1, 0, 5);
THUMB_PUSHPOP(ldmia, _r6, 0, 1, 1, 0, 6);
THUMB_PUSHPOP(ldmia, _r7, 0, 1, 1, 0, 7);

static const cpu_instr_t thumb_bkpt =
{
	.name = "bkpt",
};

#define THUMB_BRANCH(n, cond) \
static void exec_##n(cpu_t *cpu) \
{ \
	if (cond) \
	{ \
		int8_t nn = cpu->instr_opcode & 0xFF; \
		cpu_inc_pc(cpu, (int)nn * 2 + 4); \
		cpu->instr_delay += 2; \
	} \
	else \
	{ \
		cpu_inc_pc(cpu, 2); \
	} \
	cpu->instr_delay++; \
} \
static void print_##n(cpu_t *cpu, char *data, size_t size) \
{ \
	uint32_t nn = cpu->instr_opcode & 0xFF; \
	snprintf(data, size, #n " #0x%x", nn); \
} \
static const cpu_instr_t thumb_##n = \
{ \
	.name = #n, \
	.exec = exec_##n, \
	.print = print_##n, \
}

THUMB_BRANCH(beq, CPU_GET_FLAG_Z(cpu));
THUMB_BRANCH(bne, !CPU_GET_FLAG_Z(cpu));
THUMB_BRANCH(bcs, CPU_GET_FLAG_C(cpu));
THUMB_BRANCH(bcc, !CPU_GET_FLAG_C(cpu));
THUMB_BRANCH(bmi, CPU_GET_FLAG_N(cpu));
THUMB_BRANCH(bpl, !CPU_GET_FLAG_N(cpu));
THUMB_BRANCH(bvs, CPU_GET_FLAG_V(cpu));
THUMB_BRANCH(bvc, !CPU_GET_FLAG_V(cpu));
THUMB_BRANCH(bhi, CPU_GET_FLAG_C(cpu) && !CPU_GET_FLAG_Z(cpu));
THUMB_BRANCH(bls, !CPU_GET_FLAG_C(cpu) && CPU_GET_FLAG_Z(cpu));
THUMB_BRANCH(bge, CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu));
THUMB_BRANCH(blt, CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu));
THUMB_BRANCH(bgt, !CPU_GET_FLAG_Z(cpu) && CPU_GET_FLAG_N(cpu) == CPU_GET_FLAG_V(cpu));
THUMB_BRANCH(ble, CPU_GET_FLAG_Z(cpu) || CPU_GET_FLAG_N(cpu) != CPU_GET_FLAG_V(cpu));

static const cpu_instr_t thumb_swi =
{
	.name = "swi",
};

static void exec_b(cpu_t *cpu)
{
	int32_t nn = cpu->instr_opcode & 0x7FF;
	if (nn & 0x400)
		nn = -(~nn & 0x3FF) - 1;
	nn *= 2;
	cpu_set_reg(cpu, CPU_REG_PC, cpu_get_reg(cpu, CPU_REG_PC) + 4 + nn);
	cpu->instr_delay = 3;
}

static void print_b(cpu_t *cpu, char *data, size_t size)
{
	int32_t nn = cpu->instr_opcode & 0x7FF;
	if (nn & 0x400)
		nn = -(nn& 0x3FF) - 1;
	nn *= 2;
	snprintf(data, size, "b #%s0x%x", nn < 0 ? "-" : "", nn < 0 ? -nn : nn);
}

static const cpu_instr_t thumb_b =
{
	.name = "b",
	.exec = exec_b,
	.print = print_b,
};

static void exec_blx_off(cpu_t *cpu)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	uint32_t pc = cpu_get_reg(cpu, CPU_REG_PC);
	uint32_t lr = (pc + 2) | 1;
	int32_t dst = (cpu_get_reg(cpu, CPU_REG_LR) & ~1) + (nn << 1);
	dst &= 0x7FFFFF;
	cpu_set_reg(cpu, CPU_REG_PC, dst);
	cpu_set_reg(cpu, CPU_REG_LR, lr);
	CPU_SET_FLAG_T(cpu, 0);
	cpu->instr_delay = 3;
}

static void print_blx_off(cpu_t *cpu, char *data, size_t size)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	snprintf(data, size, "blx #0x%x", nn);
}

static const cpu_instr_t thumb_blx_off =
{
	.name = "blx off",
	.exec = exec_blx_off,
	.print = print_blx_off
};

static void exec_bl_setup(cpu_t *cpu)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	cpu_set_reg(cpu, CPU_REG_LR, cpu_get_reg(cpu, CPU_REG_PC) + 4 + (nn << 12));
	cpu_inc_pc(cpu, 2);
	cpu->instr_delay = 1;
}

static void print_bl_setup(cpu_t *cpu, char *data, size_t size)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	snprintf(data, size, "bl #0x%x", nn);
}

static const cpu_instr_t thumb_bl_setup =
{
	.name = "bl setup",
	.exec = exec_bl_setup,
	.print = print_bl_setup,
};

static void exec_bl_off(cpu_t *cpu)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	uint32_t pc = cpu_get_reg(cpu, CPU_REG_PC);
	uint32_t lr = (pc + 2) | 1;
	int32_t dst = (cpu_get_reg(cpu, CPU_REG_LR) & ~1) + (nn << 1);
	dst &= 0x7FFFFF;
	cpu_set_reg(cpu, CPU_REG_PC, dst);
	cpu_set_reg(cpu, CPU_REG_LR, lr);
	cpu->instr_delay = 3;
}

static void print_bl_off(cpu_t *cpu, char *data, size_t size)
{
	uint32_t nn = cpu->instr_opcode & 0x7FF;
	snprintf(data, size, "bl #0x%x", nn);
}

static const cpu_instr_t thumb_bl_off =
{
	.name = "bl off",
	.exec = exec_bl_off,
	.print = print_bl_off,
};

static const cpu_instr_t thumb_undef =
{
	.name = "undef",
};

#define REPEAT1(v) &thumb_##v
#define REPEAT2(v) REPEAT1(v), REPEAT1(v)
#define REPEAT4(v) REPEAT2(v), REPEAT2(v)
#define REPEAT8(v) REPEAT4(v), REPEAT4(v)
#define REPEAT16(v) REPEAT8(v), REPEAT8(v)
#define REPEAT32(v) REPEAT16(v), REPEAT16(v)

const cpu_instr_t *cpu_instr_thumb[0x400] =
{
	/* 0x000 */ REPEAT32(lsl_imm),
	/* 0x020 */ REPEAT32(lsr_imm),
	/* 0x040 */ REPEAT32(asr_imm),
	/* 0x060 */ REPEAT8(add_reg),
	/* 0x068 */ REPEAT8(sub_reg),
	/* 0x070 */ REPEAT8(add_imm),
	/* 0x078 */ REPEAT8(sub_imm),
	/* 0x080 */ REPEAT4(mov_i8r0),
	/* 0x084 */ REPEAT4(mov_i8r1),
	/* 0x088 */ REPEAT4(mov_i8r2),
	/* 0x08C */ REPEAT4(mov_i8r3),
	/* 0x090 */ REPEAT4(mov_i8r4),
	/* 0x094 */ REPEAT4(mov_i8r5),
	/* 0x098 */ REPEAT4(mov_i8r6),
	/* 0x09C */ REPEAT4(mov_i8r7),
	/* 0x0A0 */ REPEAT4(cmp_i8r0),
	/* 0x0A4 */ REPEAT4(cmp_i8r1),
	/* 0x0A8 */ REPEAT4(cmp_i8r2),
	/* 0x0AC */ REPEAT4(cmp_i8r3),
	/* 0x0B0 */ REPEAT4(cmp_i8r4),
	/* 0x0B4 */ REPEAT4(cmp_i8r5),
	/* 0x0B8 */ REPEAT4(cmp_i8r6),
	/* 0x0BC */ REPEAT4(cmp_i8r7),
	/* 0x0C0 */ REPEAT4(add_i8r0),
	/* 0x0C4 */ REPEAT4(add_i8r1),
	/* 0x0C8 */ REPEAT4(add_i8r2),
	/* 0x0CC */ REPEAT4(add_i8r3),
	/* 0x0D0 */ REPEAT4(add_i8r4),
	/* 0x0D4 */ REPEAT4(add_i8r5),
	/* 0x0D8 */ REPEAT4(add_i8r6),
	/* 0x0DC */ REPEAT4(add_i8r7),
	/* 0x0E0 */ REPEAT4(sub_i8r0),
	/* 0x0E4 */ REPEAT4(sub_i8r1),
	/* 0x0E8 */ REPEAT4(sub_i8r2),
	/* 0x0EC */ REPEAT4(sub_i8r3),
	/* 0x0F0 */ REPEAT4(sub_i8r4),
	/* 0x0F4 */ REPEAT4(sub_i8r5),
	/* 0x0F8 */ REPEAT4(sub_i8r6),
	/* 0x0FC */ REPEAT4(sub_i8r7),
	/* 0x100 */ REPEAT1(alu_and),
	/* 0x101 */ REPEAT1(alu_eor),
	/* 0x102 */ REPEAT1(alu_lsl),
	/* 0x103 */ REPEAT1(alu_lsr),
	/* 0x104 */ REPEAT1(alu_asr),
	/* 0x105 */ REPEAT1(alu_adc),
	/* 0x106 */ REPEAT1(alu_sbc),
	/* 0x107 */ REPEAT1(alu_ror),
	/* 0x108 */ REPEAT1(alu_tst),
	/* 0x109 */ REPEAT1(alu_neg),
	/* 0x10A */ REPEAT1(alu_cmp),
	/* 0x10B */ REPEAT1(alu_cmn),
	/* 0x10C */ REPEAT1(alu_orr),
	/* 0x10D */ REPEAT1(alu_mul),
	/* 0x10E */ REPEAT1(alu_bic),
	/* 0x10F */ REPEAT1(alu_mvn),
	/* 0x110 */ REPEAT4(addh),
	/* 0x114 */ REPEAT4(cmph),
	/* 0x118 */ REPEAT4(movh),
	/* 0x11C */ REPEAT4(bx),
	/* 0x120 */ REPEAT4(ldrpc_r0),
	/* 0x124 */ REPEAT4(ldrpc_r1),
	/* 0x128 */ REPEAT4(ldrpc_r2),
	/* 0x12C */ REPEAT4(ldrpc_r3),
	/* 0x130 */ REPEAT4(ldrpc_r4),
	/* 0x134 */ REPEAT4(ldrpc_r5),
	/* 0x138 */ REPEAT4(ldrpc_r6),
	/* 0x13C */ REPEAT4(ldrpc_r7),
	/* 0x140 */ REPEAT8(str_reg),
	/* 0x148 */ REPEAT8(strh_reg),
	/* 0x150 */ REPEAT8(strb_reg),
	/* 0x158 */ REPEAT8(ldrsb_reg),
	/* 0x160 */ REPEAT8(ldr_reg),
	/* 0x168 */ REPEAT8(ldrh_reg),
	/* 0x170 */ REPEAT8(ldrb_reg),
	/* 0x178 */ REPEAT8(ldrsh_reg),
	/* 0x180 */ REPEAT32(str_imm),
	/* 0x1A0 */ REPEAT32(ldr_imm),
	/* 0x1C0 */ REPEAT32(strb_imm),
	/* 0x1E0 */ REPEAT32(ldrb_imm),
	/* 0x200 */ REPEAT32(strh_imm),
	/* 0x220 */ REPEAT32(ldrh_imm),
	/* 0x240 */ REPEAT4(strsp_r0),
	/* 0x244 */ REPEAT4(strsp_r1),
	/* 0x248 */ REPEAT4(strsp_r2),
	/* 0x24C */ REPEAT4(strsp_r3),
	/* 0x250 */ REPEAT4(strsp_r4),
	/* 0x254 */ REPEAT4(strsp_r5),
	/* 0x258 */ REPEAT4(strsp_r6),
	/* 0x25C */ REPEAT4(strsp_r7),
	/* 0x260 */ REPEAT4(ldrsp_r0),
	/* 0x264 */ REPEAT4(ldrsp_r1),
	/* 0x268 */ REPEAT4(ldrsp_r2),
	/* 0x26C */ REPEAT4(ldrsp_r3),
	/* 0x270 */ REPEAT4(ldrsp_r4),
	/* 0x274 */ REPEAT4(ldrsp_r5),
	/* 0x278 */ REPEAT4(ldrsp_r6),
	/* 0x27C */ REPEAT4(ldrsp_r7),
	/* 0x280 */ REPEAT4(addpc_r0),
	/* 0x284 */ REPEAT4(addpc_r1),
	/* 0x288 */ REPEAT4(addpc_r2),
	/* 0x28C */ REPEAT4(addpc_r3),
	/* 0x290 */ REPEAT4(addpc_r4),
	/* 0x294 */ REPEAT4(addpc_r5),
	/* 0x298 */ REPEAT4(addpc_r6),
	/* 0x29C */ REPEAT4(addpc_r7),
	/* 0x2A0 */ REPEAT4(addsp_r0),
	/* 0x2A4 */ REPEAT4(addsp_r1),
	/* 0x2A8 */ REPEAT4(addsp_r2),
	/* 0x2AC */ REPEAT4(addsp_r3),
	/* 0x2B0 */ REPEAT4(addsp_r4),
	/* 0x2B4 */ REPEAT4(addsp_r5),
	/* 0x2B8 */ REPEAT4(addsp_r6),
	/* 0x2BC */ REPEAT4(addsp_r7),
	/* 0x2C0 */ REPEAT4(addsp_imm),
	/* 0x2C4 */ REPEAT4(undef),
	/* 0x2C8 */ REPEAT8(undef),
	/* 0x2D0 */ REPEAT4(push),
	/* 0x2D4 */ REPEAT4(push_lr),
	/* 0x2D8 */ REPEAT8(undef),
	/* 0x2E0 */ REPEAT16(undef),
	/* 0x2F0 */ REPEAT4(pop),
	/* 0x2F4 */ REPEAT4(pop_pc),
	/* 0x2F8 */ REPEAT4(bkpt),
	/* 0x2FC */ REPEAT4(undef),
	/* 0x300 */ REPEAT4(stmia_r0),
	/* 0x304 */ REPEAT4(stmia_r1),
	/* 0x308 */ REPEAT4(stmia_r2),
	/* 0x30C */ REPEAT4(stmia_r3),
	/* 0x310 */ REPEAT4(stmia_r4),
	/* 0x314 */ REPEAT4(stmia_r5),
	/* 0x318 */ REPEAT4(stmia_r6),
	/* 0x31C */ REPEAT4(stmia_r7),
	/* 0x320 */ REPEAT4(ldmia_r0),
	/* 0x324 */ REPEAT4(ldmia_r1),
	/* 0x328 */ REPEAT4(ldmia_r2),
	/* 0x32C */ REPEAT4(ldmia_r3),
	/* 0x330 */ REPEAT4(ldmia_r4),
	/* 0x334 */ REPEAT4(ldmia_r5),
	/* 0x338 */ REPEAT4(ldmia_r6),
	/* 0x33C */ REPEAT4(ldmia_r7),
	/* 0x340 */ REPEAT4(beq),
	/* 0x344 */ REPEAT4(bne),
	/* 0x348 */ REPEAT4(bcs),
	/* 0x34C */ REPEAT4(bcc),
	/* 0x350 */ REPEAT4(bmi),
	/* 0x354 */ REPEAT4(bpl),
	/* 0x358 */ REPEAT4(bvs),
	/* 0x35C */ REPEAT4(bvc),
	/* 0x360 */ REPEAT4(bhi),
	/* 0x364 */ REPEAT4(bls),
	/* 0x368 */ REPEAT4(bge),
	/* 0x36C */ REPEAT4(blt),
	/* 0x370 */ REPEAT4(bgt),
	/* 0x374 */ REPEAT4(ble),
	/* 0x378 */ REPEAT4(undef),
	/* 0x37C */ REPEAT4(swi),
	/* 0x380 */ REPEAT32(b),
	/* 0x3A0 */ REPEAT32(blx_off),
	/* 0x3C0 */ REPEAT32(bl_setup),
	/* 0x3E0 */ REPEAT32(bl_off),
};
