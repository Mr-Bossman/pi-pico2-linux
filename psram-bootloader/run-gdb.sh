#!/bin/bash
# Compile https://github.com/raspberrypi/openocd to support Raspberry Pi Pico2
sudo openocd -f interface/jlink.cfg -f target/rp2350-riscv.cfg > /dev/null 2>&1 &
gdb-multiarch build/psram-bootloader.elf --command="debug.gdb"
