/* SPDX-License-Identifier: BSD-2-Clause */
// LZ4 compatibility wrapper for Linux kernel

#ifndef __LINUX_LZ4_H__
#define __LINUX_LZ4_H__

#include "../../lib/lz4/lz4.h"
#include "../../lib/lz4/lz4hc.h"

#define LZ4_MEM_COMPRESS	LZ4_STREAM_MINSIZE
#define LZ4HC_MEM_COMPRESS	LZ4_STREAMHC_MINSIZE
/*-************************************************************************
 *	CONSTANTS
 **************************************************************************/
/*
 * LZ4_MEMORY_USAGE :
 * Memory usage formula : N->2^N Bytes
 * (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
 * Increasing memory usage improves compression ratio
 * Reduced memory usage can improve speed, due to cache effect
 * Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
 */
#define LZ4_MEMORY_USAGE 16

#define LZ4HC_MIN_CLEVEL	LZ4HC_CLEVEL_MIN
#define LZ4HC_MAX_CLEVEL	LZ4HC_CLEVEL_MAX

#endif
