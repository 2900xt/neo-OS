override KERNEL := ../filesystem/bin/kernel.elf

override CROSS := /usr/local/x86_64elfgcc/bin/x86_64-elf-g++
override LD := /usr/local/x86_64elfgcc/bin/x86_64-elf-ld

# User controllable CFLAGS.
CROSSFLAGS ?= -g -I include -Wall -Wno-deprecated-declarations  -Ttext 0xffffffff80000000 -Wno-char-subscripts -Wno-sign-compare -Wno-int-to-pointer-cast -O0
# User controllable nasm flags.
NASMFLAGS ?= -F dwarf -g

# User controllable linker flags
LDFLAGS ?=

# Internal C++ flags that should not be changed by the user.
override CROSSFLAGS +=   \
	-std=c++17           \
	-fno-exceptions      \
	-ffreestanding       \
	-fno-stack-protector \
	-fpermissive		 \
	-fno-stack-check     \
	-fno-lto             \
	-fno-pie             \
	-fno-pic             \
	-m64                 \
	-march=x86-64        \
	-mabi=sysv           \
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
override CPPFILES := $(shell find ./src -type f -name '*.cpp')
override CFILES  := $(shell find ./src -type f -name '*.c')
override ASFILES := $(shell find ./src -type f -name '*.S')
override NASMFILES := $(shell find ./src -type f -name '*.asm') 
override OBJ :=  $(CPPFILES:.cpp=.o) $(ASFILES:.S=.o) $(NASMFILES:.asm=.o) $(CFILES:.c=.o) 
override HEADER_DEPS := $(CPPFILES:.cpp=.d) $(ASFILES:.S=.d) $(CFILES:.c=.d)

# Default target.
.PHONY: all
all: $(KERNEL)

# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	@echo linking kernel elf...
	@$(LD) $(OBJ) $(LDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.cpp files.
%.o: %.cpp
	@$(CROSS) $(CROSSFLAGS) -c $< -o $@
	@echo $<

%.o: %.c
	@echo $<
	@$(CROSS) $(CROSSFLAGS) -mgeneral-regs-only -c $< -o $@

# Compilation rules for *.S files.
%.o: %.S
	@echo $<
	@$(CROSS) $(CROSSFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
%.o: %.asm
	@echo $<
	@nasm $(NASMFLAGS) $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf $(OBJ) $(HEADER_DEPS)
