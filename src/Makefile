ARCH_DIR = arch/riscv
KERNEL = kernel.bin
OBJ_DIR = build

# trouver toutes les sources sauf crt0
SRCS = $(shell find . -type f \( -name '*.c' -o -name '*.S' \) ! -name 'crt0.S')

# CRT0
CRT0_SRC = $(ARCH_DIR)/crt0.S
CRT0_OBJ = $(OBJ_DIR)/$(CRT0_SRC:.S=.o)

# Tous les autres objets dans build/
OBJS = $(CRT0_OBJ)
OBJS += $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
OBJS := $(patsubst %.S,$(OBJ_DIR)/%.o,$(OBJS))

CMDBASE = riscv64-unknown-elf
AS = $(CMDBASE)-gcc
ASFLAGS = -g -c -I.
CC = $(AS)
CFLAGS = -march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -Wno-main -Wformat-truncation=0 -pipe -nostdinc -Wall -Wextra -std=c11 -mabi=lp64 -ffunction-sections -fdata-sections -O0 -ggdb3  -D verb=$(VERB)
CFLAGS += -I.
ASFLAGS = $(CFLAGS) -c
LD = $(AS)
LDFLAGS = -nostdlib -nostartfiles -static -lgcc -Wl,--nmagic -Wl,--gc-sections
OBJCOPY = $(CMDBASE)-objcopy
QEMU = qemu-system-riscv64

QEMUBASEOPT = -machine virt -m 128M -bios none
QEMUOPTSRUN = -nographic $(QEMUBASEOPT)
QEMUOPTS = -nographic -gdb tcp::1234 -S $(QEMUBASEOPT)
QEMUOPTSRUNG = -device bochs-display -serial mon:stdio -display default,show-cursor=on $(QEMUBASEOPT)
QEMUOPTSG = -gdb tcp::1234 -S $(QEMUOPTSRUNG)

# cible principale, on nettoie systematiquement le repertoire avant
.PHONY: all
all: clean kernel.bin

kernel.bin: $(ARCH_DIR)/ld/kernel.lds $(OBJS)
	$(LD) $(LDFLAGS) -e entry -T $(ARCH_DIR)/ld/kernel.lds $(OBJS) -o $@


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(OBJ_DIR) $(KERNEL) task_dump_screen.o test test.bin *.log .gdb_history
	$(RM) -rf badges

debug: $(KERNEL)
	$(QEMU) $(QEMUOPTS) -kernel kernel.bin

run: $(KERNEL)
	$(QEMU) $(QEMUOPTSRUN) -kernel kernel.bin

debugg: $(KERNEL)
	$(QEMU) $(QEMUOPTSG) -kernel kernel.bin

rung: $(KERNEL)
	$(QEMU) $(QEMUOPTSRUNG) -kernel kernel.bin

VERB ?= 0

test.bin: test.lds $(OBJS)
	$(LD) $(LDFLAGS) -e entry_test -T test.lds $(OBJS) -o $@

.PHONY: test

test: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSRUNG) -kernel test.bin -m 32M > test
test_to: test.bin
	$(QEMU) -machine cep -bios none -kernel test.bin -m 32M -nographic
badges: test
	mkdir -p badges;cat test | awk 'match($$0,/^.*\[(.*)\] \((.*)\/(.*)\)/,a){gsub(" ","_",a[1]);system("rm -f \"badges/"a[1]".svg\";anybadge -l \""a[1]"\" -v \""((a[2]==a[3])?"ok":"failed")"\" -f \"badges/"a[1]".svg\" -c "((a[2]==a[3])?"green":"red"))}'
testg: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSRUNG) -kernel test.bin 
testgd: test.bin
	DISPLAY=:0 $(QEMU) $(QEMUOPTSG) -kernel test.bin
testd: test.bin
	DISPLAY=:0 $(QEMU)  $(QEMUOPTSG) -kernel test.bin
testdebug: test.bin
	tmux new-session -s debugsession -d -n debug "DISPLAY=:0 $(QEMU) $(QEMUOPTSG) -kernel test.bin"
	tmux split-window -t debugsession -t debugsession -v gdb-multiarch test.bin
	tmux attach-session -t debugsession
