.section .asm, "ax"
.global _start
#define REGS_ARR_SZ (4*31)
#define STORE_REGS(reg) sw x##reg, ((reg*4) - 0x4)(sp)
#define LOAD_REGS(reg) lw x##reg, ((reg*4) - 0x4)(sp)

.align 4
_start:
	//Save ra to old stack
	sw	ra, -16(sp)
	//Move sp into ra
	mv	ra, sp

	la	sp, _sstack
	addi	sp,sp,-REGS_ARR_SZ

	// Save sp on stack
	sw ra, 0x4(sp)
	// load ra from old stack into ra
	lw ra, -16(ra)

	STORE_REGS(1)

	STORE_REGS(3)
	STORE_REGS(4)
	STORE_REGS(5)
	STORE_REGS(6)
	STORE_REGS(7)
	STORE_REGS(8)
	STORE_REGS(9)
	STORE_REGS(10)
	STORE_REGS(11)
	STORE_REGS(12)
	STORE_REGS(13)
	STORE_REGS(14)
	STORE_REGS(15)
	STORE_REGS(16)
	STORE_REGS(17)
	STORE_REGS(18)
	STORE_REGS(19)
	STORE_REGS(20)
	STORE_REGS(21)
	STORE_REGS(22)
	STORE_REGS(23)
	STORE_REGS(24)
	STORE_REGS(25)
	STORE_REGS(26)
	STORE_REGS(27)
	STORE_REGS(28)
	STORE_REGS(29)
	STORE_REGS(30)
	STORE_REGS(31)

	mv a0, sp
	jal ra, _entry_

	LOAD_REGS(1)
	LOAD_REGS(3)
	LOAD_REGS(4)
	LOAD_REGS(5)
	LOAD_REGS(6)
	LOAD_REGS(7)
	LOAD_REGS(8)
	LOAD_REGS(9)
	LOAD_REGS(10)
	LOAD_REGS(11)
	LOAD_REGS(12)
	LOAD_REGS(13)
	LOAD_REGS(14)
	LOAD_REGS(15)
	LOAD_REGS(16)
	LOAD_REGS(17)
	LOAD_REGS(18)
	LOAD_REGS(19)
	LOAD_REGS(20)
	LOAD_REGS(21)
	LOAD_REGS(22)
	LOAD_REGS(23)
	LOAD_REGS(24)
	LOAD_REGS(25)
	LOAD_REGS(26)
	LOAD_REGS(27)
	LOAD_REGS(28)
	LOAD_REGS(29)
	LOAD_REGS(30)
	LOAD_REGS(31)

	// Load sp last
	LOAD_REGS(2)
	ret
