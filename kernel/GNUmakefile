# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := bin/kernel.elf

override CROSS := /usr/local/x86_64elfgcc/bin/x86_64-elf-g++
override LD := /usr/local/x86_64elfgcc/bin/x86_64-elf-ld

# User controllable CFLAGS.
CROSSFLAGS ?= -g -I lib -Wall -mgeneral-regs-only -Ttext 0xffffffff80000000

# User controllable nasm flags.
NASMFLAGS ?= -F dwarf -g

# User controllable linker flags. We set none by default.
LDFLAGS ?=

# Internal C flags that should not be changed by the user.
override CROSSFLAGS +=   \
    -std=c++17           \
    -ffreestanding       \
    -fno-stack-protector \
    -fno-stack-check     \
    -fno-lto             \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -march=x86-64        \
    -mabi=sysv           \
    -mno-80387           \
    -mno-mmx             \
    -mno-sse             \
    -mno-sse2            \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -MMD                 \

# Internal linker flags that should not be changed by the user.
override LDFLAGS +=         \
    -z max-page-size=0x1000 \
    -T linker.ld            \

# Check if the linker supports -no-pie and enable it if it does
ifeq ($(shell $(LD) --help 2>&1 | grep 'no-pie' >/dev/null 2>&1; echo $$?),0)
    override LDFLAGS += -no-pie
endif

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS += \
    -f elf64

# Use find to glob all *.c, *.S, and *.asm files in the directory and extract the object names.
override CFILES := $(shell find ./src -type f -name '*.cpp')
override ASFILES := $(shell find ./src -type f -name '*.S')
override NASMFILES := $(shell find ./src -type f -name '*.asm')
override OBJ := $(CFILES:.cpp=.o) $(ASFILES:.S=.o) $(NASMFILES:.asm=.o)
override HEADER_DEPS := $(CFILES:.cpp=.d) $(ASFILES:.S=.d)

# Default target.
.PHONY: all
all: $(KERNEL)

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.cpp files.
%.o: %.cpp
	$(CROSS) $(CROSSFLAGS) -c $< -o $@

# Compilation rules for *.S files.
%.o: %.S
	$(CROSS) $(CROSSFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf $(OBJ) $(HEADER_DEPS)

