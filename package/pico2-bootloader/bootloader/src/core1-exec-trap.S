/*
 * core1-exec-trap.S — safe call trampoline for running an untrusted-but-buggy
 * blob on core1 (RV32, M-mode).
 *
 * int32_t c1x_run(void *fn, const struct c1_services *svc,
 *                 uint32_t arg, uint32_t stacktop);
 *   a0 = fn   (blob entry)
 *   a1 = svc  (services table -> passed to blob in a0)
 *   a2 = arg  (-> passed to blob in a1)
 *   a3 = stacktop (blob stack, grows down)
 *
 * Saves core1 context, installs a private mtvec, switches to the blob stack and
 * calls fn(svc, arg). On normal return -> returns fn's value. On ANY exception
 * inside the blob (illegal instr, load/store fault, ecall, ...) the trap handler
 * abandons the blob and long-jumps back, returning C1X_ERR_TRAP | (mcause & 0xff)
 * instead of hanging core1.
 *
 * Note: this recovers from FAULTS, not from an infinite loop with no fault
 * (no timer preemption here). For that see INTEGRATION.md (optional mtimecmp).
 * Note: core1 runs its polling server with interrupts disabled, so installing a
 * private mtvec for the duration of the call is safe.
 */
	.section .text
	.align 2
	.globl c1x_run
	.type c1x_run, @function
c1x_run:
	addi	sp, sp, -64
	sw	ra,  0(sp)
	sw	gp,  4(sp)
	sw	tp,  8(sp)
	sw	s0, 12(sp)
	sw	s1, 16(sp)
	sw	s2, 20(sp)
	sw	s3, 24(sp)
	sw	s4, 28(sp)
	sw	s5, 32(sp)
	sw	s6, 36(sp)
	sw	s7, 40(sp)
	sw	s8, 44(sp)
	sw	s9, 48(sp)
	sw	s10, 52(sp)
	sw	s11, 56(sp)
	csrr	t0, mtvec
	sw	t0, 60(sp)

	/* remember the frame for the trap handler, install our vector */
	csrw	mscratch, sp
	la	t0, c1x_trap
	csrw	mtvec, t0

	/* marshal: target=fn, a0=svc, a1=arg, sp=blob stack (16-byte aligned) */
	mv	t0, a0
	mv	a0, a1
	mv	a1, a2
	mv	sp, a3
	andi	sp, sp, -16
	jalr	t0			/* call blob; result in a0 */

	/* ---- shared return/recovery path ---- */
c1x_done:
	csrr	sp, mscratch		/* restore core1 stack (frame) */
	lw	t0, 60(sp)
	csrw	mtvec, t0		/* restore mtvec */
	lw	ra,  0(sp)
	lw	gp,  4(sp)
	lw	tp,  8(sp)
	lw	s0, 12(sp)
	lw	s1, 16(sp)
	lw	s2, 20(sp)
	lw	s3, 24(sp)
	lw	s4, 28(sp)
	lw	s5, 32(sp)
	lw	s6, 36(sp)
	lw	s7, 40(sp)
	lw	s8, 44(sp)
	lw	s9, 48(sp)
	lw	s10, 52(sp)
	lw	s11, 56(sp)
	addi	sp, sp, 64
	ret				/* a0 = result (normal) or error (trap) */
	.size c1x_run, .-c1x_run

	/* trap vector: must stay 4-byte aligned (direct mode, low 2 bits = 0) */
	.align 2
	.type c1x_trap, @function
c1x_trap:
	csrr	sp, mscratch		/* sp = saved frame (blob's sp is unusable) */
	csrr	a0, mcause
	andi	a0, a0, 0xff
	li	t0, 0xBADC0D00		/* C1X_ERR_TRAP */
	or	a0, a0, t0
	j	c1x_done		/* longjmp back through the restore path */
	.size c1x_trap, .-c1x_trap
