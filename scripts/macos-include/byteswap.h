/* SPDX-License-Identifier: GPL-2.0 */
/*
 * byteswap.h - macOS shim for building Linux kernel host tools
 *
 * macOS doesn't provide <byteswap.h>; map to compiler builtins.
 */
#pragma once
#define bswap_16 __builtin_bswap16
#define bswap_32 __builtin_bswap32
#define bswap_64 __builtin_bswap64
