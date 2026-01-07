#ifndef HELPERS_H
#define HELPERS_H

#define BIT(nr)				(1U << (nr))
#define NBIT(nr)			(~BIT(nr))

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
 #define GENMASK(h, l) \
 (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (31 - (h))))

#define bit_is_set(REG, nr)		((REG) & BIT(nr))
#define bit_is_clear(REG, nr)		(!bit_is_set(REG, nr))

#define loop_until_bit_is_set(REG, nr) \
	do {} while (bit_is_clear(REG, nr))

#define loop_until_bit_is_clear(REG, nr) \
	do {} while (bit_is_set(REG, nr))

#define set_bit(REG, nr)		((REG) | BIT(nr))
#define clear_bit(REG, nr)		((REG) & NBIT(nr))

#define assign_bit(REG, nr, VAL) \
	((VAL) ? set_bit(REG, nr) : clear_bit(REG, nr))

#define set_bitmask(REG, MASK)		((REG) | (MASK))
#define clear_bitmask(REG, MASK)	((REG) & ~(MASK))

#define assign_bitmask(REG, MASK, VAL)	\
	(clear_bitmask(REG, MASK) | (VAL))


static inline void nop(void) {
	__asm__ volatile ("nop");
}

static inline void delay(uint32_t count) {
	__asm__ volatile (
		"1: addi %[count], %[count], -1\n"
		"   bne %[count], x0, 1b\n"
		: [count] "+r" (count) : : "cc"
	);
}

#define mb()	__asm volatile ("fence rw, rw" : : : "memory")
#define barrier()	__asm volatile ("" : : : "memory")

#endif /* HELPERS_H */
