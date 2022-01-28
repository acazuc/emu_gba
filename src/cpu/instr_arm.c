#include "instr_arm.h"
#include "../cpu.h"
#include <stdlib.h>
#include <stdio.h>

#define ARM_ALU_OP(op) \
static const cpu_instr_t arm_##op##_lli = \
{ \
	.name = #op " lli", \
}; \
static const cpu_instr_t arm_##op##_llr = \
{ \
	.name = #op " llr", \
}; \
static const cpu_instr_t arm_##op##_lri = \
{ \
	.name = #op " lri", \
}; \
static const cpu_instr_t arm_##op##_lrr = \
{ \
	.name = #op " lrr", \
}; \
static const cpu_instr_t arm_##op##_ari = \
{ \
	.name = #op " ari", \
}; \
static const cpu_instr_t arm_##op##_arr = \
{ \
	.name = #op " arr", \
}; \
static const cpu_instr_t arm_##op##_rri = \
{ \
	.name = #op " rri", \
}; \
static const cpu_instr_t arm_##op##_rrr = \
{ \
	.name = #op " rrr", \
}; \
static const cpu_instr_t arm_##op##_imm = \
{ \
	.name = #op " imm", \
};

#define ARM_ALU_OPS(op) \
	ARM_ALU_OP(op) \
	ARM_ALU_OP(op##s)

ARM_ALU_OPS(and);
ARM_ALU_OPS(eor);
ARM_ALU_OPS(sub);
ARM_ALU_OPS(rsb);
ARM_ALU_OPS(add);
ARM_ALU_OPS(adc);
ARM_ALU_OPS(sbc);
ARM_ALU_OPS(rsc);
ARM_ALU_OP(tsts);
ARM_ALU_OP(teqs);
ARM_ALU_OP(cmps);
ARM_ALU_OP(cmns);
ARM_ALU_OPS(orr);
ARM_ALU_OPS(mov);
ARM_ALU_OPS(bic);
ARM_ALU_OPS(mvn);

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

#define STLD_PTR(v) \
static const cpu_instr_t arm_str_##v##ll = \
{ \
	.name = "str " #v "ll", \
}; \
static const cpu_instr_t arm_strh_##v = \
{ \
	.name = "strh " #v, \
}; \
static const cpu_instr_t arm_strd_##v = \
{ \
	.name = "strd " #v, \
}; \
static const cpu_instr_t arm_strb_##v##ll = \
{ \
	.name = "strb " #v "ll", \
}; \
static const cpu_instr_t arm_strt_##v##ll = \
{ \
	.name = "strt " #v "ll", \
}; \
static const cpu_instr_t arm_strbt_##v##ll = \
{ \
	.name = "strbt " #v "ll", \
}; \
static const cpu_instr_t arm_ldr_##v##ll = \
{ \
	.name = "ldr " #v "ll", \
}; \
static const cpu_instr_t arm_ldrh_##v = \
{ \
	.name = "ldrh " #v, \
}; \
static const cpu_instr_t arm_ldrd_##v = \
{ \
	.name = "ldrd " #v, \
}; \
static const cpu_instr_t arm_ldrb_##v##ll = \
{ \
	.name = "ldrb " #v "ll", \
}; \
static const cpu_instr_t arm_ldrt_##v##ll = \
{ \
	.name = "ldrt " #v "ll", \
}; \
static const cpu_instr_t arm_ldrbt_##v##ll = \
{ \
	.name = "ldrbt " #v "ll", \
}; \
static const cpu_instr_t arm_ldrsb_##v = \
{ \
	.name = "ldrsb " #v, \
}; \
static const cpu_instr_t arm_ldrsh_##v = \
{ \
	.name = "ldrsh " #v, \
};

STLD_PTR(ptrm);
STLD_PTR(ptrp);

#define STLD_PTI(v) \
static const cpu_instr_t arm_str_##v = \
{ \
	.name = "str " #v, \
}; \
static const cpu_instr_t arm_strh_##v = \
{ \
	.name = "strh " #v, \
}; \
static const cpu_instr_t arm_strd_##v = \
{ \
	.name = "strd " #v, \
}; \
static const cpu_instr_t arm_strb_##v = \
{ \
	.name = "strb " #v, \
}; \
static const cpu_instr_t arm_strt_##v = \
{ \
	.name = "strt " #v, \
}; \
static const cpu_instr_t arm_strbt_##v = \
{ \
	.name = "strbt " #v, \
}; \
static const cpu_instr_t arm_ldr_##v = \
{ \
	.name = "ldr " #v, \
}; \
static const cpu_instr_t arm_ldrh_##v = \
{ \
	.name = "ldrh " #v, \
}; \
static const cpu_instr_t arm_ldrd_##v = \
{ \
	.name = "ldrd " #v, \
}; \
static const cpu_instr_t arm_ldrb_##v = \
{ \
	.name = "ldrb " #v, \
}; \
static const cpu_instr_t arm_ldrt_##v = \
{ \
	.name = "ldrt " #v, \
}; \
static const cpu_instr_t arm_ldrbt_##v = \
{ \
	.name = "ldrbt " #v, \
}; \
static const cpu_instr_t arm_ldrsb_##v = \
{ \
	.name = "ldrsb " #v, \
}; \
static const cpu_instr_t arm_ldrsh_##v = \
{ \
	.name = "ldrsh " #v, \
}; \

STLD_PTI(ptim);
STLD_PTI(ptip);

#define STLD_OFR(v) \
static const cpu_instr_t arm_str_##v##ll = \
{ \
	.name = "str " #v "ll", \
}; \
static const cpu_instr_t arm_strh_##v = \
{ \
	.name = "strh " #v, \
}; \
static const cpu_instr_t arm_strd_##v = \
{ \
	.name = "strd " #v, \
}; \
static const cpu_instr_t arm_strb_##v##ll = \
{ \
	.name = "strb " #v "ll", \
}; \
static const cpu_instr_t arm_ldr_##v##ll = \
{ \
	.name = "ldr " #v "ll", \
}; \
static const cpu_instr_t arm_ldrh_##v = \
{ \
	.name = "ldrh " #v, \
}; \
static const cpu_instr_t arm_ldrd_##v = \
{ \
	.name = "ldrd " #v, \
}; \
static const cpu_instr_t arm_ldrb_##v##ll = \
{ \
	.name = "ldrb " #v "ll", \
}; \
static const cpu_instr_t arm_ldrsb_##v = \
{ \
	.name = "ldrsb " #v, \
}; \
static const cpu_instr_t arm_ldrsh_##v = \
{ \
	.name = "ldrsh " #v, \
};

STLD_OFR(ofrm);
STLD_OFR(ofrp);

#define STLD_PRR(v) \
static const cpu_instr_t arm_strh_##v = \
{ \
	.name = "strh " #v, \
}; \
static const cpu_instr_t arm_strd_##v = \
{ \
	.name = "strd " #v, \
}; \
static const cpu_instr_t arm_ldrh_##v = \
{ \
	.name = "ldrh " #v, \
}; \
static const cpu_instr_t arm_ldrd_##v = \
{ \
	.name = "ldrd " #v, \
}; \
static const cpu_instr_t arm_ldrsb_##v = \
{ \
	.name = "ldrsb " #v, \
}; \
static const cpu_instr_t arm_ldrsh_##v = \
{ \
	.name = "ldrsh " #v, \
};

STLD_PRR(prrm);
STLD_PRR(prrp);

#define STLD_PPR(v) \
static const cpu_instr_t arm_str_##v##ll = \
{ \
	.name = "str " #v "ll", \
}; \
static const cpu_instr_t arm_strb_##v##ll = \
{ \
	.name = "strb " #v "ll", \
}; \
static const cpu_instr_t arm_ldr_##v##ll = \
{ \
	.name = "ldr " #v "ll", \
}; \
static const cpu_instr_t arm_ldrb_##v##ll = \
{ \
	.name = "ldrb " #v "ll", \
};

STLD_PPR(pprm);
STLD_PPR(pprp);

#define STLD_OFI(v) \
static const cpu_instr_t arm_str_##v = \
{ \
	.name = "str " #v, \
}; \
static const cpu_instr_t arm_strh_##v = \
{ \
	.name = "strh " #v, \
}; \
static const cpu_instr_t arm_strd_##v = \
{ \
	.name = "strd " #v, \
}; \
static const cpu_instr_t arm_strb_##v = \
{ \
	.name = "strb " #v, \
}; \
static const cpu_instr_t arm_ldr_##v = \
{ \
	.name = "ldr " #v, \
}; \
static const cpu_instr_t arm_ldrh_##v = \
{ \
	.name = "ldrh " #v, \
}; \
static const cpu_instr_t arm_ldrd_##v = \
{ \
	.name = "ldrd " #v, \
}; \
static const cpu_instr_t arm_ldrb_##v = \
{ \
	.name = "ldrb " #v, \
}; \
static const cpu_instr_t arm_ldrsb_##v = \
{ \
	.name = "ldrsb " #v, \
}; \
static const cpu_instr_t arm_ldrsh_##v = \
{ \
	.name = "ldrsh " #v, \
};

STLD_OFI(ofim);
STLD_OFI(ofip);
STLD_OFI(prim);
STLD_OFI(prip);

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

static const cpu_instr_t arm_mrs_rc =
{
	.name = "mrs rc",
};

static const cpu_instr_t arm_mrs_rs =
{
	.name = "mrs rs",
};

static const cpu_instr_t arm_msr_rs =
{
	.name = "msr rs",
};

static const cpu_instr_t arm_msr_ic =
{
	.name = "msr ic",
};

static const cpu_instr_t arm_msr_is =
{
	.name = "msr is",
};

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

static bool exec_b(cpu_t *cpu)
{
	cpu->regs.r[15] += 4 + 4 * (cpu->instr_opcode & 0xFFFFFF);
	return true;
}

static void print_b(cpu_t *cpu, char *data, size_t size)
{
	snprintf(data, size, "b %x", cpu->instr_opcode & 0xFFFFFF);
}

static const cpu_instr_t arm_b =
{
	.name = "b",
	.exec = exec_b,
	.print = print_b,
};

static const cpu_instr_t arm_bl =
{
	.name = "bl",
};

static const cpu_instr_t arm_bx =
{
	.name = "bx",
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
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef), \
	REPEAT1(v##ll), REPEAT1(undef)

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
	/* 0x120 */ REPEAT1(mrs_rc) , REPEAT1(bx)       , REPEAT1(undef)  , REPEAT1(blx_reg),
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
	/* 0x720 */ STLD_BLANK(str_pprm),
	/* 0x730 */ STLD_BLANK(ldr_pprm),
	/* 0x740 */ STLD_BLANK(strb_ofrm),
	/* 0x750 */ STLD_BLANK(ldrb_ofrm),
	/* 0x760 */ STLD_BLANK(strb_pprm),
	/* 0x770 */ STLD_BLANK(ldrb_pprm),
	/* 0x780 */ STLD_BLANK(str_ofrp),
	/* 0x790 */ STLD_BLANK(ldr_ofrp),
	/* 0x7A0 */ STLD_BLANK(str_pprp),
	/* 0x7B0 */ STLD_BLANK(ldr_pprp),
	/* 0x7C0 */ STLD_BLANK(strb_ofrp),
	/* 0x7D0 */ STLD_BLANK(ldrb_ofrp),
	/* 0x7E0 */ STLD_BLANK(strb_pprp),
	/* 0x7F0 */ STLD_BLANK(ldrb_pprp),
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
