/* SPDX-License-Identifier: GPL-2.0 */
/*
 * elf.h - macOS shim for building Linux kernel host tools
 *
 * macOS doesn't provide <elf.h>. Homebrew's libelf (via gelf.h) provides
 * the ELF struct types and most constants, but is missing relocation type
 * constants and a few newer machine types that the kernel's host tools need.
 *
 * Based on https://seiya.me/blog/building-linux-on-macos-natively
 */
#pragma once

/* Pull in struct types, scalar types, and base constants from libelf */
#include <libelf/gelf.h>

/*
 * STT_SPARC_REGISTER - used by modpost.c
 */
#ifndef STT_SPARC_REGISTER
#define STT_SPARC_REGISTER	13
#endif

/*
 * Machine types not in Homebrew's ancient libelf (0.8.13)
 * The kernel source guards some with #ifndef, but not all.
 */
#ifndef EM_AARCH64
#define EM_AARCH64	183
#endif
#ifndef EM_ARCOMPACT
#define EM_ARCOMPACT	93
#endif
#ifndef EM_ARCV2
#define EM_ARCV2	195
#endif
#ifndef EM_MICROBLAZE
#define EM_MICROBLAZE	189
#endif
#ifndef EM_RISCV
#define EM_RISCV	243
#endif
#ifndef EM_LOONGARCH
#define EM_LOONGARCH	258
#endif

/*
 * x86 relocations
 */
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2

#define R_X86_64_NONE	0
#define R_X86_64_64	1

/*
 * ARM relocations
 */
#define R_ARM_NONE		0
#define R_ARM_PC24		1
#define R_ARM_ABS32		2
#define R_ARM_REL32		3
#define R_ARM_THM_CALL		10
#define R_ARM_THM_PC22		10	/* alias for R_ARM_THM_CALL */
#define R_ARM_CALL		28
#define R_ARM_JUMP24		29
#define R_ARM_THM_JUMP24	30
#define R_ARM_MOVW_ABS_NC	43
#define R_ARM_MOVT_ABS		44
#define R_ARM_THM_MOVW_ABS_NC	47
#define R_ARM_THM_MOVT_ABS	48
#define R_ARM_THM_JUMP19	51

/*
 * ARM ELF flags
 */
#define EF_ARM_EABI_MASK	0xFF000000
#define EF_ARM_EABI_VERSION(flags)	((flags) & EF_ARM_EABI_MASK)

/*
 * AArch64 relocations
 */
#define R_AARCH64_NONE		0
#define R_AARCH64_ABS64		257
#define R_AARCH64_PREL64	260
#define R_AARCH64_CALL26	283

/*
 * MIPS relocations
 */
#define R_MIPS_NONE	0
#define R_MIPS_32	2
#define R_MIPS_26	4
#define R_MIPS_HI16	5
#define R_MIPS_LO16	6
#define R_MIPS_64	18

/*
 * PowerPC relocations
 */
#define R_PPC_ADDR32	1
#define R_PPC64_ADDR64	38

/*
 * IA-64 relocations
 */
#define R_IA64_IMM64	0x23

/*
 * SH relocations
 */
#define R_SH_DIR32	1

/*
 * SPARC relocations
 */
#define R_SPARC_64	12

/*
 * RISC-V relocations
 */
#ifndef R_RISCV_SUB32
#define R_RISCV_SUB32	39
#endif

/*
 * LoongArch relocations
 */
#ifndef R_LARCH_32
#define R_LARCH_32	1
#endif
#ifndef R_LARCH_64
#define R_LARCH_64	2
#endif
#ifndef R_LARCH_MARK_LA
#define R_LARCH_MARK_LA	20
#endif
#ifndef R_LARCH_SOP_PUSH_PLT_PCREL
#define R_LARCH_SOP_PUSH_PLT_PCREL	29
#endif
#ifndef R_LARCH_SUB32
#define R_LARCH_SUB32	55
#endif
