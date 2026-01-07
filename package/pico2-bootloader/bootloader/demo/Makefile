BUILD_DIR = build

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
LD = $(CROSS_COMPILE)ld
# LD may need to be LD=riscv64-linux-gnu-ld

CFLAGS = -march=rv32izicsr -mabi=ilp32 -O2 -Wall -Wextra
LDFLAGS = -m elf32lriscv -nostdlib --gc-sections --no-dynamic-linker

S_SOURCES = src/start.S
C_SOURCES = src/demo.c

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.S=.o)))
DEPS = $(OBJECTS:%.o=%.d)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.S $(sort $(dir $(S_SOURCES)))
vpath %.lds src
vpath %.o $(BUILD_DIR)
vpath %.elf $(BUILD_DIR)
vpath %.bin $(BUILD_DIR)

.PHONY: all clean demo

all: demo

demo: demo.bin
	cp $(BUILD_DIR)/demo.bin ../build/

$(OBJECTS): | $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

-include $(DEPS)
$(BUILD_DIR)/%.o: %.c
	$(CC) -MMD -c $(CFLAGS) $< -o $@

-include $(DEPS)
$(BUILD_DIR)/%.o: %.S
	$(CC) -MMD -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/demo.elf: demo.lds $(OBJECTS)
	$(LD) $(LDFLAGS) -T $^ -o $@

$(BUILD_DIR)/demo.bin: demo.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)
