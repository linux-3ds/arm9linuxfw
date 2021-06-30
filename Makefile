TARGET  := $(shell basename $(CURDIR))

TRIPLET ?= arm-none-eabi
CC := $(TRIPLET)-gcc
OBJCOPY := $(TRIPLET)-objcopy

SOURCE  := source
BUILD   := build
INCDIR	:= include
SUBARCH := -mcpu=arm946e-s -mfloat-abi=soft -marm -mno-thumb-interwork \
	-ggdb -nostdlib -ffreestanding -nostartfiles -lgcc -flto

DBG_FLAG :=
# ifneq ($(RELEASE),)
# DBG_FLAG := -DNDEBUG
# endif

INCLUDE := -I$(SOURCE) -I$(INCDIR)
ASFLAGS := $(SUBARCH) $(INCLUDE) -x assembler-with-cpp
CFLAGS := $(SUBARCH) $(INCLUDE) -MMD -MP -std=c11 -Os -pipe -Wall -Wextra \
			-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function \
			-ffunction-sections -fomit-frame-pointer -ffast-math $(DBG_FLAG)

LDFLAGS := -Tlink.ld -Wl,--gc-sections,--nmagic,-z,max-page-size=4 $(SUBARCH)

rwildcard = $(foreach d, $(wildcard $1*), \
            $(filter $(subst *, %, $2), $d) \
            $(call rwildcard, $d/, $2))

SOURCE_OUTPUT := $(patsubst $(SOURCE)/%.c, $(BUILD)/%.c.o, \
				$(patsubst $(SOURCE)/%.s, $(BUILD)/%.s.o, \
				$(call rwildcard, $(SOURCE), *.s *.c)))

.PHONY: all
all: $(TARGET).bin

.PHONY: clean
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	@$(OBJCOPY) -O binary $^ $@

$(TARGET).elf: $(SOURCE_OUTPUT)
	@mkdir -p "$(@D)"
	@$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/%.c.o: $(SOURCE)/%.c
	@mkdir -p "$(@D)"
	@echo "  $<"
	@$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD)/%.s.o: $(SOURCE)/%.s
	@mkdir -p "$(@D)"
	@echo "  $<"
	@$(CC) -c $(ASFLAGS) -o $@ $<

include $(call rwildcard, $(BUILD), *.d)
