CXX=g++
CXXFLAGS=-std=gnu++11 -O2 -fwrapv -Wall
CPPFLAGS=-D_GNU_SOURCE=1
CPP=cpp
#
M68K_CC=m68k-unknown-elf-gcc
M68K_CFLAGS=-Os -g -std=gnu99 -march=68000 -fwrapv -fsigned-char -fno-strict-aliasing -Wall -Werror -Werror="stack-usage=" -Werror=implicit-function-declaration -Wstack-usage=64
M68K_AS=m68k-unknown-elf-as
M68K_ASFLAGS=-march=68000 -mcpu=68ec000
M68K_OBJCOPY=m68k-unknown-elf-objcopy
#
SH2_CC=sh-elf-gcc
SH2_CFLAGS=-O2 -std=gnu99 -m2 -fwrapv -fno-aggressive-loop-optimizations -fsigned-char -fno-inline -fno-unit-at-a-time -fno-common -fno-asynchronous-unwind-tables -Wall -Werror="stack-usage=" -Werror=implicit-function-declaration -Wstack-usage=1024 -Istdlib
SH2_OBJCOPY=sh-elf-objcopy
#
#
#
XORRISOFS_FLAGS=--norock --set_all_file_dates 2038011903140800 --modification-date=2038011903140800 -preparer "" -appid "" -volset "CRONSF"
XORRISOFS_TRACK1_FLAGS=$(XORRISOFS_FLAGS) -iso-level 1 -sysid "SEGA SEGASATURN" -volid "CRONSF" -G sec_cd.bin
XORRISOFS_TRACK2_FLAGS=$(XORRISOFS_FLAGS) -iso-level 4 -sysid "" -volid "CRONSF_NSFS"
#
#
#
all:		cronsf.ss cd-image
#test.iso

init68k.elf:	init68k.c init68k.ld gs/types.h gs/scsp.h
		$(M68K_CC) $(M68K_CFLAGS) -nostdlib -Xlinker -Tinit68k.ld -o init68k.elf init68k.c -lgcc

init68k.bin:	init68k.elf
		$(M68K_OBJCOPY) -O binary init68k.elf init68k.bin

init68k.bin.h:	init68k.bin bintoinc
		cat init68k.bin | ./bintoinc > init68k.bin.h

gentables:	gentables.cpp
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o gentables gentables.cpp
		./gentables

scspntsc.bin:	gentables
scsppal.bin:	gentables
madrs.h:	gentables

bintoinc:	bintoinc.cpp
		$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o bintoinc bintoinc.cpp
#
#
#
#
#

STDLIB_OBJS=	stdlib/crt0_sh2.o stdlib/stdlib.o stdlib/string.o stdlib/assert.o stdlib/unistd.o stdlib/printf.o stdlib/stdio.o stdlib/ctype.o stdlib/error.o
GS_OBJS=	gs/smpc.o gs/scsp.o gs/cdb.o gs/sh2.o

stdlib/crt0_sh2.o: stdlib/crt0_sh2.S
		$(SH2_CC) $(SH2_CFLAGS) -o stdlib/crt0_sh2.o -c stdlib/crt0_sh2.S

stdlib/%.o:	stdlib/%.c stdlib/*.h
		$(SH2_CC) $(SH2_CFLAGS) -o stdlib/$*.o -c stdlib/$*.c

gs/%.o:		gs/%.c gs/*.h
		$(SH2_CC) $(SH2_CFLAGS) -o gs/$*.o -c gs/$*.c

%.o:		%.c *.h gs/*.h
		$(SH2_CC) $(SH2_CFLAGS) -o $*.o -c $*.c

%_s.o:		%_s.S common.S.inc
		$(SH2_CC) $(SH2_CFLAGS) -o $*_s.o -c $*_s.S
#
#
s6502.o:	s6502.S s6502.h common.S.inc
		$(SH2_CC) $(SH2_CFLAGS) -Wa,--fatal-warnings -o s6502.o -c s6502.S

apu.o:		apu.c apu.h config.h s6502.h madrs.h init68k.bin.h gs/types.h gs/scsp.h gs/smpc.h
		$(SH2_CC) $(SH2_CFLAGS) -o apu.o -c apu.c

nsfcore.o:	nsfcore.c nsfcore.h config.h apu.h s6502.h shim_6502.bin.h iso88591_to_cp850.h gs/types.h gs/endian.h gs/scsp.h
		$(SH2_CC) $(SH2_CFLAGS) -o nsfcore.o -c nsfcore.c

shim_6502.bin:	shim_6502.asm
		xa -C -o shim_6502.bin shim_6502.asm

shim_6502.bin.h: shim_6502.bin bintoinc
		cat shim_6502.bin | ./bintoinc > shim_6502.bin.h

cronsf.o:	cronsf.c config.h version.h gfx.h fsys.h nsfcore.h gs/*.h
		$(SH2_CC) $(SH2_CFLAGS) -o cronsf.o -c cronsf.c

cronsf.elf:	cronsf.o gfx.o fsys.o nsfcore.o apu.o s6502.o cronsf.ld $(GS_OBJS) $(STDLIB_OBJS)
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tcronsf.ld -o cronsf.elf cronsf.o gfx.o fsys.o nsfcore.o apu.o s6502.o $(GS_OBJS) $(STDLIB_OBJS) -lgcc

cronsf.bin:	cronsf.elf
		$(SH2_OBJCOPY) -O binary -j HIRAM cronsf.elf cronsf.bin

cronsf.ss:	cronsf.bin sec_cart.bin
		cat sec_cart.bin cronsf.bin > cronsf.ss	

nsfs/Tests/testapu.nsfe:	testapu.asm
		xa -C -o nsfs/Tests/testapu.nsfe testapu.asm

cronsf-track1.iso:	Makefile sec_cd.bin font.bin cronsf.bin scspntsc.bin scsppal.bin
		xorrisofs $(XORRISOFS_TRACK1_FLAGS) -o cronsf-track1.iso cronsf.bin font.bin scspntsc.bin scsppal.bin

cronsf-track2-nsfs.iso:	Makefile nsfs/Tests/testapu.nsfe nsfs nsfs/* nsfs/*/*
		xorrisofs $(XORRISOFS_TRACK2_FLAGS) -o cronsf-track2-nsfs.iso nsfs/

cd-image:	cronsf-track1.iso cronsf-track2-nsfs.iso

#
#
#
#
test_stdlib.elf:	test_stdlib.c gs/*.h $(GS_OBJS) $(STDLIB_OBJS)
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tcronsf.ld -o test_stdlib.elf test_stdlib.c $(GS_OBJS) $(STDLIB_OBJS) -lgcc

test_stdlib.bin:	test_stdlib.elf
		$(SH2_OBJCOPY) -O binary -j HIRAM test_stdlib.elf test_stdlib.bin

test_stdlib.ss:	test_stdlib.bin sec_cart.bin
		cat sec_cart.bin test_stdlib.bin > test_stdlib.ss

test_stdlib.iso:	Makefile sec_cd.bin font.bin test_stdlib.bin
		xorrisofs $(XORRISOFS_TRACK1_FLAGS) -o test_stdlib.iso test_stdlib.bin font.bin

#
#
#
.PHONY:		clean
clean:
		rm --force --one-file-system -- bintoinc gentables scspntsc.bin scsppal.bin madrs.h madrs-pal.h cronsf.elf cronsf.bin cronsf.o fsys.o gfx.o nsfcore.o apu.o s6502.o s6502_s.o test6502.o test6502.elf test6502.bin test6502.ss cronsf.ss cronsf-track1.iso cronsf-track2-nsfs.iso init68k.elf init68k.bin init68k.bin.h shim_6502.o shim_6502.bin shim_6502.bin.h test_stdlib.o test_stdlib.bin test_stdlib.elf test_stdlib.ss test_stdlib.iso $(STDLIB_OBJS) $(GS_OBJS)
