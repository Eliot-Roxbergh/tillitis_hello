P := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
LIBDIR ?= $(P)/../tkey-libs
RM=/bin/rm
OBJCOPY ?= llvm-objcopy
CC = clang
CFLAGS = -g -target riscv32-unknown-none-elf -march=rv32iczmmul -mabi=ilp32 -mcmodel=medany \
   -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf \
   -fno-builtin-putchar -nostdlib -mno-relax -flto \
   -Wall -Werror=implicit-function-declaration \
   -I $(LIBDIR)/include -I $(LIBDIR)  \
   -DNODEBUG

INCLUDE=$(LIBDIR)/include

LDFLAGS=-T $(LIBDIR)/app.lds -L $(LIBDIR)/libcommon/ -lcommon -L $(LIBDIR)/libcrt0/ -lcrt0

.PHONY: all
all: coin_race.bin

# Turn elf into bin for device
%.bin: %.elf
	$(OBJCOPY) --input-target=elf32-littleriscv --output-target=binary $^ $@
	chmod a-x $@

BLUEOBJS=coin_race.o
coin_race.elf: coin_race.o
	$(CC) $(CFLAGS) $(BLUEOBJS) $(LDFLAGS) -I $(LIBDIR) -o $@

.PHONY: clean
clean:
	$(RM) -f coin_race.bin coin_race.elf $(BLUEOBJS)
