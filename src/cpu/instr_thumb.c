#include "instr_thumb.h"

static const cpu_instr_t thumb_lsl_imm =
{
	.name = "lsl imm",
};

static const cpu_instr_t thumb_lsr_imm =
{
	.name = "lsr imm",
};

static const cpu_instr_t thumb_asr_imm =
{
	.name = "asr imm",
};

static const cpu_instr_t thumb_add_reg =
{
	.name = "add reg",
};

static const cpu_instr_t thumb_sub_reg =
{
	.name = "sub reg",
};

static const cpu_instr_t thumb_add_imm3 =
{
	.name = "add imm3",
};

static const cpu_instr_t thumb_sub_imm3 =
{
	.name = "sub imm3",
};

#define THUMB_MOV_I8R(r) \
static const cpu_instr_t thumb_mov_i8r##r = \
{ \
	.name = "mov i8r" #r, \
};

THUMB_MOV_I8R(0);
THUMB_MOV_I8R(1);
THUMB_MOV_I8R(2);
THUMB_MOV_I8R(3);
THUMB_MOV_I8R(4);
THUMB_MOV_I8R(5);
THUMB_MOV_I8R(6);
THUMB_MOV_I8R(7);

#define THUMB_CMP_I8R(r) \
static const cpu_instr_t thumb_cmp_i8r##r = \
{ \
	.name = "cmp i8r" #r, \
};

THUMB_CMP_I8R(0);
THUMB_CMP_I8R(1);
THUMB_CMP_I8R(2);
THUMB_CMP_I8R(3);
THUMB_CMP_I8R(4);
THUMB_CMP_I8R(5);
THUMB_CMP_I8R(6);
THUMB_CMP_I8R(7);

#define THUMB_ADD_I8R(r) \
static const cpu_instr_t thumb_add_i8r##r = \
{ \
	.name = "add i8r" #r, \
};

THUMB_ADD_I8R(0);
THUMB_ADD_I8R(1);
THUMB_ADD_I8R(2);
THUMB_ADD_I8R(3);
THUMB_ADD_I8R(4);
THUMB_ADD_I8R(5);
THUMB_ADD_I8R(6);
THUMB_ADD_I8R(7);

#define THUMB_SUB_I8R(r) \
static const cpu_instr_t thumb_sub_i8r##r = \
{ \
	.name = "sub i8r" #r, \
};

THUMB_SUB_I8R(0);
THUMB_SUB_I8R(1);
THUMB_SUB_I8R(2);
THUMB_SUB_I8R(3);
THUMB_SUB_I8R(4);
THUMB_SUB_I8R(5);
THUMB_SUB_I8R(6);
THUMB_SUB_I8R(7);

static const cpu_instr_t thumb_alu_and =
{
	.name = "and",
};

static const cpu_instr_t thumb_alu_eor =
{
	.name = "eor",
};

static const cpu_instr_t thumb_alu_lsl =
{
	.name = "lsl",
};

static const cpu_instr_t thumb_alu_lsr =
{
	.name = "lsr",
};

static const cpu_instr_t thumb_alu_asr =
{
	.name = "asr",
};

static const cpu_instr_t thumb_alu_add =
{
	.name = "add",
};

static const cpu_instr_t thumb_alu_sub =
{
	.name = "sub",
};

static const cpu_instr_t thumb_alu_ror =
{
	.name = "ror",
};

static const cpu_instr_t thumb_alu_tst =
{
	.name = "tst",
};

static const cpu_instr_t thumb_alu_neg =
{
	.name = "neg",
};

static const cpu_instr_t thumb_alu_cmp =
{
	.name = "cmp",
};

static const cpu_instr_t thumb_alu_cmn =
{
	.name = "cmn",
};

static const cpu_instr_t thumb_alu_orr =
{
	.name = "orr",
};

static const cpu_instr_t thumb_alu_mul =
{
	.name = "mul",
};

static const cpu_instr_t thumb_alu_bic =
{
	.name = "bic",
};

static const cpu_instr_t thumb_alu_mvn =
{
	.name = "mvn",
};

static const cpu_instr_t thumb_addh =
{
	.name = "addh",
};

static const cpu_instr_t thumb_cmph =
{
	.name = "cmph",
};

static const cpu_instr_t thumb_movh =
{
	.name = "movh",
};

static const cpu_instr_t thumb_bx_reg =
{
	.name = "bx reg",
};

#define THUMB_LDRPC_R(r) \
static const cpu_instr_t thumb_ldrpc_r##r = \
{ \
	.name = "ldrpc r" #r, \
};

THUMB_LDRPC_R(0);
THUMB_LDRPC_R(1);
THUMB_LDRPC_R(2);
THUMB_LDRPC_R(3);
THUMB_LDRPC_R(4);
THUMB_LDRPC_R(5);
THUMB_LDRPC_R(6);
THUMB_LDRPC_R(7);

static const cpu_instr_t thumb_str_reg =
{
	.name = "str reg",
};

static const cpu_instr_t thumb_strh_reg =
{
	.name = "strh reg",
};

static const cpu_instr_t thumb_strb_reg =
{
	.name = "strb reg",
};

static const cpu_instr_t thumb_ldrsb_reg =
{
	.name = "lsrsb reg",
};

static const cpu_instr_t thumb_ldr_reg =
{
	.name = "ldr reg",
};

static const cpu_instr_t thumb_ldrh_reg =
{
	.name = "ldrg reg",
};

static const cpu_instr_t thumb_ldrb_reg =
{
	.name = "ldrb reg",
};

static const cpu_instr_t thumb_ldrsh_reg =
{
	.name = "ldrsh reg",
};

static const cpu_instr_t thumb_str_imm5 =
{
	.name = "str imm5",
};

static const cpu_instr_t thumb_ldr_imm5 =
{
	.name = "ldr imm5",
};

static const cpu_instr_t thumb_strb_imm5 =
{
	.name = "strb imm5",
};

static const cpu_instr_t thumb_ldrb_imm5 =
{
	.name = "ldrb imm5",
};

static const cpu_instr_t thumb_strh_imm5 =
{
	.name = "strh imm5",
};

static const cpu_instr_t thumb_ldrh_imm5 =
{
	.name = "ldrh imm5",
};

#define THUMB_STRSP_R(r) \
static const cpu_instr_t thumb_strsp_r##r = \
{ \
	.name = "strsp r" #r, \
};

THUMB_STRSP_R(0);
THUMB_STRSP_R(1);
THUMB_STRSP_R(2);
THUMB_STRSP_R(3);
THUMB_STRSP_R(4);
THUMB_STRSP_R(5);
THUMB_STRSP_R(6);
THUMB_STRSP_R(7);

#define THUMB_LDRSP_R(r) \
static const cpu_instr_t thumb_ldrsp_r##r = \
{ \
	.name = "ldrsp r" #r, \
};

THUMB_LDRSP_R(0);
THUMB_LDRSP_R(1);
THUMB_LDRSP_R(2);
THUMB_LDRSP_R(3);
THUMB_LDRSP_R(4);
THUMB_LDRSP_R(5);
THUMB_LDRSP_R(6);
THUMB_LDRSP_R(7);

#define THUMB_ADDPC_R(r) \
static const cpu_instr_t thumb_addpc_r##r = \
{ \
	.name = "addpc r" #r, \
};

THUMB_ADDPC_R(0);
THUMB_ADDPC_R(1);
THUMB_ADDPC_R(2);
THUMB_ADDPC_R(3);
THUMB_ADDPC_R(4);
THUMB_ADDPC_R(5);
THUMB_ADDPC_R(6);
THUMB_ADDPC_R(7);

#define THUMB_ADDSP_R(r) \
static const cpu_instr_t thumb_addsp_r##r = \
{ \
	.name = "addsp r" #r, \
};

THUMB_ADDSP_R(0);
THUMB_ADDSP_R(1);
THUMB_ADDSP_R(2);
THUMB_ADDSP_R(3);
THUMB_ADDSP_R(4);
THUMB_ADDSP_R(5);
THUMB_ADDSP_R(6);
THUMB_ADDSP_R(7);

static const cpu_instr_t thumb_addsp_imm7 =
{
	.name = "addsp imm7",
};

static const cpu_instr_t thumb_push =
{
	.name = "push",
};

static const cpu_instr_t thumb_push_lr =
{
	.name = "push lr",
};

static const cpu_instr_t thumb_pop =
{
	.name = "pop",
};

static const cpu_instr_t thumb_pop_pc =
{
	.name = "pop pc",
};

static const cpu_instr_t thumb_bkpt =
{
	.name = "bkpt",
};

#define THUMB_STMIA_R(r) \
static const cpu_instr_t thumb_stmia_r##r = \
{ \
	.name = "stmia r" #r, \
};

THUMB_STMIA_R(0);
THUMB_STMIA_R(1);
THUMB_STMIA_R(2);
THUMB_STMIA_R(3);
THUMB_STMIA_R(4);
THUMB_STMIA_R(5);
THUMB_STMIA_R(6);
THUMB_STMIA_R(7);

#define THUMB_LDMIA_R(r) \
static const cpu_instr_t thumb_ldmia_r##r = \
{ \
	.name = "ldmia r" #r, \
};

THUMB_LDMIA_R(0);
THUMB_LDMIA_R(1);
THUMB_LDMIA_R(2);
THUMB_LDMIA_R(3);
THUMB_LDMIA_R(4);
THUMB_LDMIA_R(5);
THUMB_LDMIA_R(6);
THUMB_LDMIA_R(7);

static const cpu_instr_t thumb_beq =
{
	.name = "beq",
};

static const cpu_instr_t thumb_bne =
{
	.name = "bne",
};

static const cpu_instr_t thumb_bcs =
{
	.name = "bcs",
};

static const cpu_instr_t thumb_bcc =
{
	.name = "bcc",
};

static const cpu_instr_t thumb_bmi =
{
	.name = "bmi",
};

static const cpu_instr_t thumb_bpl =
{
	.name = "bpl",
};

static const cpu_instr_t thumb_bvs =
{
	.name = "bvs",
};

static const cpu_instr_t thumb_bvc =
{
	.name = "bvc",
};

static const cpu_instr_t thumb_bhi =
{
	.name = "bhi",
};

static const cpu_instr_t thumb_bls =
{
	.name = "bls",
};

static const cpu_instr_t thumb_bge =
{
	.name = "bge",
};

static const cpu_instr_t thumb_blt =
{
	.name = "blt",
};

static const cpu_instr_t thumb_bgt =
{
	.name = "bgt",
};

static const cpu_instr_t thumb_ble =
{
	.name = "ble",
};

static const cpu_instr_t thumb_swi =
{
	.name = "swi",
};

static const cpu_instr_t thumb_b =
{
	.name = "b",
};

static const cpu_instr_t thumb_blx_off =
{
	.name = "blx off",
};

static const cpu_instr_t thumb_bl_setup =
{
	.name = "bl setup",
};

static const cpu_instr_t thumb_bl_off =
{
	.name = "bl off",
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
	/* 0x070 */ REPEAT8(add_imm3),
	/* 0x078 */ REPEAT8(sub_imm3),
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
	/* 0x105 */ REPEAT1(alu_add),
	/* 0x106 */ REPEAT1(alu_sub),
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
	/* 0x11C */ REPEAT4(bx_reg),
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
	/* 0x180 */ REPEAT32(str_imm5),
	/* 0x1A0 */ REPEAT32(ldr_imm5),
	/* 0x1C0 */ REPEAT32(strb_imm5),
	/* 0x1E0 */ REPEAT32(ldrb_imm5),
	/* 0x200 */ REPEAT32(strh_imm5),
	/* 0x220 */ REPEAT32(ldrh_imm5),
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
	/* 0x2C0 */ REPEAT4(addsp_imm7),
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
