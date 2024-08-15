.PHONY: all clean flash picotool

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
	@$(MAKE) picotool PICOTOOL_ARGS="load -fx build/psram-bootloader.uf2"
endif

clean:
	rm -rf build
