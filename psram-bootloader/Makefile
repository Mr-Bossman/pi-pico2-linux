.PHONY: all clean flash picotool demo flash-demo flash-kernel

all: build/Makefile
	@$(MAKE) -C build

build:
	mkdir -p build

build/Makefile: CMakeLists.txt | build
	cd build && cmake ..

picotool:
ifeq (0, $(shell id -u))
	picotool $(PICOTOOL_ARGS)
else
	@echo You must be root!
ifeq (, $(shell which sudo))
	su -c "$(MAKE) $@ PICOTOOL_ARGS='$(PICOTOOL_ARGS)'"
else
	sudo $(MAKE) $@ PICOTOOL_ARGS='$(PICOTOOL_ARGS)'
endif
endif

copy:
	$(eval MNT_PATH := $(shell lsblk -o MOUNTPOINTS | grep RP2350))
	$(if $(value MNT_PATH),,$(error No RP2350 device found))
	cp build/psram-bootloader.uf2 "$(MNT_PATH)"/

flash: all
ifeq (, $(shell which picotool))
	@$(MAKE) copy
else
	@$(MAKE) picotool PICOTOOL_ARGS="load -fu build/psram-bootloader.uf2"
endif

# make CROSS_COMPILE=riscv32-unknown-elf- LD=riscv64-linux-gnu-ld demo
demo: | build
	@$(MAKE) -C demo

flash-demo: demo flash
	sleep 2
	@$(MAKE) picotool PICOTOOL_ARGS="load -fxup 0 build/demo.bin"

flash-kernel: ../buildroot/output/images/flash-image.bin flash
	sleep 3
	@$(MAKE) picotool PICOTOOL_ARGS="load -fxup 0 ../buildroot/output/images/flash-image.bin"

clean:
	@$(MAKE) -C demo clean
	rm -rf build
