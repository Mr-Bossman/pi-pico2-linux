.section .start, "ax"
.global _start
_start:
	j _reloc

.section .reloc, "ax"
_reloc:
	la	sp, __ram_end
	la	a0, __data_start
	la	a1, __data_end
	la	a2, __data_load_start
	jal	ra, _copy
	la	a0, __text_start
	la	a1, __text_end
	la	a2, __text_load_start
	jal	ra, _copy
	la	a0, __bss_start
	la	a1, __bss_end
	mv	a2, x0
	jal	ra, _set
	call	_entry

_set:
	blt	a0, a1, ._set_loop
	ret
._set_loop:
	sw	a0, (a2)
	addi	a2, a2, 4
	bge	a1, a0, ._set_loop
	ret

_copy:
	blt	a0, a1, ._copy_loop
	ret
._copy_loop:
	lw	a3, (a2)
	addi	a2, a2, 4
	sw	a3, (a0)
	addi	a0, a0, 4
	bge	a1, a0, ._copy_loop
	ret

.section .text, "ax"
.global main
.global _entry
_entry:
	la	a0, __ctors_start
	la	a1, __ctors_end
	jal	ra, _run_calls
	jal	ra, main
	la	a0, __dtors_start
	la	a1, __dtors_end
	jal	ra, _run_calls
halt:
	wfi
	j	halt

_run_calls:
	blt	a0, a1, ._run_calls_start
	ret
._run_calls_start:
	addi	sp, sp, -16
	sw	ra,  0(sp)
	sw	s0,  4(sp)
	sw	s1,  8(sp)
	sw	s2, 12(sp)
	mv	s0, a0
	mv	s1, a1
._run_calls_loop:
	lw	s0, (s2)
	addi	s0, s0, 4
	jalr	ra, s2
	bge	s1, s0, ._run_calls_loop
	lw	s2, 12(sp)
	lw	s1,  8(sp)
	lw	s0,  4(sp)
	lw	ra,  0(sp)
	addi	sp, sp, 16
	ret