set architecture riscv:rv32
target remote localhost:3333
add-auto-load-safe-path ../buildroot/output/build/linux-6.10
cd ../buildroot/output/build/linux-6.10
layout asm
layout reg split
add-symbol-file vmlinux
focus cmd
