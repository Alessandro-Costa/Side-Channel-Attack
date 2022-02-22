#pragma once

#include <stdint.h>
#define PAGE_SIZE 4096

static inline void lfence()
{
	asm("lfence" ::: "memory");
}

static inline void clflush(volatile void *p)
{
	asm volatile("clflush (%0)\n"
		:: "r" (p));
}

static inline int memaccess(void *v) {
	int rv;
	asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
	return rv;
}

static inline uint64_t rdtsc(void)
{
	uint64_t lo, hi;

	asm volatile("rdtscp\n"
	: "=a" (lo), "=d" (hi)
	:: "%rcx");

	lo |= (hi << 32);

	return lo;
}


#define XBEGIN_INIT     (~0u)
#define XABORT_EXPLICIT (1 << 0)
#define XABORT_RETRY    (1 << 1)
#define XABORT_CONFLICT (1 << 2)
#define XABORT_CAPACITY (1 << 3)
#define XABORT_DEBUG    (1 << 4)
#define XABORT_NESTED   (1 << 5)
#define XABORT_CODE(x)  (((x) >> 24) & 0xff)

static inline unsigned xbegin(void)
{
	uint32_t ret = XBEGIN_INIT;

	asm volatile(
	"xbegin 1f\n"
	"1:\n"
	: "+a" (ret)
	:: "memory");

	return ret;
}

static inline void xend(void)
{
	asm volatile(
	"xend\n"
	::: "memory");
}
