BUILD_DIR = build
PROGRAM = bootloader

CROSS_COMPILE := riscv32-unknown-elf-

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
LD = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc

PICOTOOL := picotool

CFLAGS :=	-std=gnu11 \
		-march=rv32imazicsr \
		-mabi=ilp32 \
		-Os \
		-ffunction-sections \
		-fdata-sections \
		-Wall \
		-Wextra \
		-Wno-unused-function \
		-Isrc \
		-DPRINTF_INCLUDE_CONFIG_H

ASFLAGS := $(CFLAGS)
LDFLAGS := -nostdlib -Wl,--gc-sections

S_SOURCES = src/rp2350-crt.S src/rp2350-hdr.S
C_SOURCES = src/main.c src/uart.c src/string.c src/qmi.c src/image.c src/printf.c

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.S=.o)))
DEPS = $(OBJECTS:%.o=%.d)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.S $(sort $(dir $(S_SOURCES)))
vpath %.lds src
vpath %.o $(BUILD_DIR)
vpath %.elf $(BUILD_DIR)

.PHONY: all clean debug flash flash-payload

all: $(PROGRAM).uf2 $(PROGRAM)-payload.uf2

debug: CFLAGS += -g
debug: all

$(OBJECTS): | $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

-include $(DEPS)
$(BUILD_DIR)/%.o: %.c
	$(CC) -MMD -c $(CFLAGS) $< -o $@

-include $(DEPS)
$(BUILD_DIR)/%.o: %.S
	$(CC) -MMD -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(PROGRAM).elf: rp2350.lds $(OBJECTS)
	$(LD) $(LDFLAGS) -T $^ -o $@

$(PROGRAM).uf2: $(BUILD_DIR)/$(PROGRAM).uf2
	cp $< $@

$(PROGRAM).uf2: $(BUILD_DIR)/$(PROGRAM).uf2
	cp $< $@

$(PROGRAM)-payload.uf2: $(BUILD_DIR)/$(PROGRAM)-payload.uf2
	cp $< $@

$(PROGRAM).bin: $(BUILD_DIR)/$(PROGRAM).bin
	cp $< $@

$(PROGRAM)-picosdk.uf2: $(BUILD_DIR)/psram-bootloader.uf2
	cp $< $@

$(PROGRAM)-picosdk-payload.uf2: $(BUILD_DIR)/psram-bootloader-payload.uf2
	cp $< $@

$(BUILD_DIR)/$(PROGRAM).uf2: $(BUILD_DIR)/$(PROGRAM).elf
	$(PICOTOOL) uf2 convert $< $@ --family rp2350-riscv

$(BUILD_DIR)/$(PROGRAM).bin: $(BUILD_DIR)/$(PROGRAM).elf
	$(OBJCOPY) -O binary $< $@

# Dummy payload file so we won't get errors if it doesn't exist
payload.bin:
	touch payload.bin

$(BUILD_DIR)/$(PROGRAM)-payload.uf2: $(BUILD_DIR)/$(PROGRAM).bin payload.bin
	cat $^ > $(BUILD_DIR)/$(PROGRAM)-payload.bin
	$(PICOTOOL) uf2 convert $(BUILD_DIR)/$(PROGRAM)-payload.bin $@ --family rp2350-riscv

flash: $(PROGRAM).uf2
	$(PICOTOOL) load -fx $<

flash-payload: $(PROGRAM)-payload.uf2
	$(PICOTOOL) load -fx $<

# picosdk build targets
$(BUILD_DIR)/Makefile: CMakeLists.txt | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

$(BUILD_DIR)/psram-bootloader.bin: $(BUILD_DIR)/Makefile FORCE
	@$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/psram-bootloader.uf2: $(BUILD_DIR)/psram-bootloader.bin

$(BUILD_DIR)/psram-bootloader-payload.uf2: $(BUILD_DIR)/psram-bootloader.bin payload.bin
	truncate --size %$$((0x10)) $<
	cat $^ > $(BUILD_DIR)/psram-bootloader-payload.bin
	$(PICOTOOL) uf2 convert $(BUILD_DIR)/psram-bootloader-payload.bin $@ --family rp2350-riscv

FORCE:

clean:
	-rm -rf $(BUILD_DIR)
	-rm -f $(PROGRAM).uf2 $(PROGRAM)-payload.uf2
	-rm -f $(PROGRAM)-picosdk.uf2 $(PROGRAM)-picosdk-payload.uf2
