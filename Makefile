######################################
# target
######################################
TARGET = bootloader


######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -Og


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
Src/main.c \
Src/bootloader.c \
Src/ymodem.c \
Src/mini_print.c \
Src/common.c \
Src/stm32f1xx_it.c \
Src/system_stm32f1xx.c \
Src/stm32f1xx_hal_msp.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
lib/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
# lib/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

# ASM sources
ASM_SOURCES =  \
startup/startup_stm32f103c8tx.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m3

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi


# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F103xB


# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-IInc \
-Ilib/STM32F1xx_HAL_Driver/Inc \
-Ilib/STM32F1xx_HAL_Driver/Inc/Legacy \
-Ilib/CMSIS/Device/ST/STM32F1xx/Include \
-Ilib/CMSIS/Include


# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
else
CFLAGS += -Os
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = linker/STM32F103C8TX_BOOTLOADER.ld

# libraries
LIBS =
LIBDIR =
LDFLAGS = $(MCU) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
# LDFLAGS += -nostdlib

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin example_app


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# Program the device
#######################################
flash: $(BUILD_DIR)/$(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program ${BUILD_DIR}/${TARGET}.bin 0x08000000 verify reset exit"

#######################################
# Debug with OpenOCD
#######################################
debug: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset"

#######################################
# Size information
#######################################
size: $(BUILD_DIR)/$(TARGET).elf
	$(SZ) $<

#######################################
# Clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
	$(MAKE) -C example_app clean


#######################################
# Show help
#######################################
help:
	@echo "Available targets:"
	@echo "  all     - Build the bootloader"
	@echo "  clean   - Clean build files"
	@echo "  flash   - Program the device using st-link"
	@echo "  debug   - Debug using OpenOCD"
	@echo "  size    - Show size information"
	@echo "  help    - Show this help"

#######################################
# Application build helper
#######################################
app_example:
	@echo "To build an application that works with this bootloader:"
	@echo "1. Set the application start address to 0x08004000 in your linker script"
	@echo "2. Add this to your application's main() function to enter bootloader:"
	@echo "   // To enter bootloader on next reset"
	@echo "   *((uint32_t *)0x20000000) = 0xDEADBEEF;"
	@echo "   HAL_NVIC_SystemReset();"
	$(MAKE) -C example_app

# merge bootloader and app in one firmware
merge: app_example $(BUILD_DIR)/$(TARGET).bin
	./merge.py build/bootloader.bin app_example/build/app.bin

flash_all:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program firmware_all.bin 0x08000000 verify reset exit"

#######################################
# Dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
