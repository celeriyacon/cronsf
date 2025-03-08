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
SH2_CFLAGS=-O2 -std=gnu99 -m2 -fwrapv -fno-aggressive-loop-optimizations -fsigned-char -fno-inline -fno-unit-at-a-time -fno-common -fno-asynchronous-unwind-tables -Wall -Werror=implicit-function-declaration -Wstack-usage=192 -Istdlib
SH2_OBJCOPY=sh-elf-objcopy
#
#
#
XORRISOFS_FLAGS=--norock --set_all_file_dates 2038011903140800 --modification-date=2038011903140800 -preparer "" -appid "" -volset "CRONSF"
XORRISOFS_TRACK1_FLAGS=$(XORRISOFS_FLAGS) -iso-level 1 -sysid "SEGA SEGASATURN" -volid "CRONSF" -G sysarea.bin --sort-weight-list cronsf-track1-weights.txt -graft-points
XORRISOFS_TRACK2_FLAGS=$(XORRISOFS_FLAGS) -iso-level 4 -sysid "" -volid "CRONSF_NSFS" -graft-points
XORRISOFS_CART_FLAGS=$(XORRISOFS_FLAGS) -iso-level 4 -sysid "SEGA SEGASATURN" -volid "CRONSF" -G sysarea.bin --sort-weight-list cronsf-track1-weights.txt -graft-points
#
XORRISOFS_TEST_FLAGS=$(XORRISOFS_FLAGS) -iso-level 1 -sysid "SEGA SEGASATURN" -volid "CRONSF" -G sysarea.bin -graft-points
#
#
#
all:		cd-image cart-image
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
madrs_pal.h:	gentables
exchip.bin:	gentables
exchip_lut.h:	gentables

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

%_s.o:		%_s.S s6502.h
		$(SH2_CC) $(SH2_CFLAGS) -o $*_s.o -c $*_s.S
#
#
s6502.o:	s6502.S s6502.h
		$(SH2_CC) $(SH2_CFLAGS) -Wa,--fatal-warnings -o s6502.o -c s6502.S

apu.o:		apu.c apu.h config.h s6502.h madrs.h init68k.bin.h gs/types.h gs/scsp.h gs/smpc.h
		$(SH2_CC) $(SH2_CFLAGS) -o apu.o -c apu.c

nsfcore.o:	nsfcore.c nsfcore.h config.h apu.h s6502.h shim_6502.bin.h iso88591_to_cp850.h gs/types.h gs/endian.h gs/scsp.h
		$(SH2_CC) $(SH2_CFLAGS) -o nsfcore.o -c nsfcore.c

vrc6.o:		vrc6.c vrc6.h exchip.h exchip_lut.h config.h s6502.h gs/types.h gs/sh2.h
		$(SH2_CC) $(SH2_CFLAGS) -o $*.o -c $*.c

mmc5.o:		mmc5.c mmc5.h exchip.h exchip_lut.h config.h s6502.h gs/types.h gs/sh2.h
		$(SH2_CC) $(SH2_CFLAGS) -o $*.o -c $*.c

shim_6502.bin:	shim_6502.asm
		xa -C -o shim_6502.bin shim_6502.asm

shim_6502.bin.h: shim_6502.bin bintoinc
		cat shim_6502.bin | ./bintoinc > shim_6502.bin.h

sys_ipl.bin:	sys_ipl.c sys_ipl.ld
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tsys_ipl.ld -o sys_ipl.elf sys_ipl.c
		$(SH2_OBJCOPY) -O binary -j IPL sys_ipl.elf sys_ipl.bin

sysarea.bin:	sys_ipl.bin sys_id.bin sys_sec.bin
		cat sys_id.bin sys_sec.bin sys_ipl.bin > sysarea.bin

cronsf.o:	cronsf.c config.h version.h gfx.h fsys.h nsfcore.h gs/*.h
		$(SH2_CC) $(SH2_CFLAGS) -o cronsf.o -c cronsf.c

cronsf.elf:	cronsf.o cronsf_s.o gfx.o fsys.o nsfcore.o apu.o s6502.o vrc6.o sun5b.o n163.o mmc5.o cronsf.ld $(GS_OBJS) $(STDLIB_OBJS)
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tcronsf.ld -o cronsf.elf cronsf.o cronsf_s.o gfx.o fsys.o nsfcore.o apu.o s6502.o vrc6.o sun5b.o n163.o mmc5.o $(GS_OBJS) $(STDLIB_OBJS) -lgcc

cronsf.bin:	cronsf.elf
		$(SH2_OBJCOPY) -O binary -j HIRAM -j ".sun5b_reloc" cronsf.elf cronsf.bin

nsfs/Tests/testapu.nsfe:	testapu.asm
		xa -C -o nsfs/Tests/testapu.nsfe testapu.asm

nsfs/Tests/testvrc6.nsfe:	testvrc6.asm
		xa -C -o nsfs/Tests/testvrc6.nsfe testvrc6.asm

nsfs/Tests/test5b.nsfe:		test5b.asm
		xa -C -o nsfs/Tests/test5b.nsfe test5b.asm

nsfs/Tests/testmmc5.nsfe:	testmmc5.asm
		xa -C -o nsfs/Tests/testmmc5.nsfe testmmc5.asm

cronsf-track1.iso:	Makefile sysarea.bin font.bin cronsf.bin scspntsc.bin scsppal.bin
		xorrisofs $(XORRISOFS_TRACK1_FLAGS) -o cronsf-track1.iso /0.bin=cronsf.bin font.bin scspntsc.bin scsppal.bin exchip.bin

cronsf-track2-nsfs.iso:	Makefile nsfs/Tests/testapu.nsfe nsfs/Tests/testvrc6.nsfe nsfs/Tests/test5b.nsfe nsfs/Tests/testmmc5.nsfe nsfs nsfs/* nsfs/*/*
		xorrisofs $(XORRISOFS_TRACK2_FLAGS) -o cronsf-track2-nsfs.iso nsfs/

cd-image:	cronsf-track1.iso cronsf-track2-nsfs.iso

cronsf.ss:	Makefile sysarea.bin font.bin cronsf.bin scspntsc.bin scsppal.bin nsfs/Tests/testapu.nsfe nsfs/Tests/testvrc6.nsfe nsfs/Tests/test5b.nsfe nsfs/Tests/testmmc5.nsfe nsfs nsfs/* nsfs/*/*
		xorrisofs $(XORRISOFS_CART_FLAGS) -o cronsf.ss /0.bin=cronsf.bin font.bin scspntsc.bin scsppal.bin exchip.bin /nsfs=nsfs

cart-image:	cronsf.ss

#
#
#
#
test_stdlib.elf:	test_stdlib.c gs/*.h $(GS_OBJS) $(STDLIB_OBJS)
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tcronsf.ld -o test_stdlib.elf test_stdlib.c $(GS_OBJS) $(STDLIB_OBJS) -lgcc

test_stdlib.bin:	test_stdlib.elf
		$(SH2_OBJCOPY) -O binary -j HIRAM test_stdlib.elf test_stdlib.bin

test_stdlib.iso:	Makefile sysarea.bin font.bin test_stdlib.bin
		xorrisofs $(XORRISOFS_TEST_FLAGS) -o test_stdlib.iso /0.bin=test_stdlib.bin font.bin

#
#
#
test_dsp.elf:	test_dsp.c gs/*.h $(GS_OBJS) $(STDLIB_OBJS)
		$(SH2_CC) $(SH2_CFLAGS) -nostdlib -Xlinker -Tcronsf.ld -o test_dsp.elf test_dsp.c $(GS_OBJS) $(STDLIB_OBJS) -lgcc

test_dsp.bin:	test_dsp.elf
		$(SH2_OBJCOPY) -O binary -j HIRAM test_dsp.elf test_dsp.bin

test_dsp.iso:	Makefile sysarea.bin font.bin test_dsp.bin
		xorrisofs $(XORRISOFS_TEST_FLAGS) -o test_dsp.iso /0.bin=test_dsp.bin font.bin
#
#
#
.PHONY:		clean
clean:
		rm --force --one-file-system -- bintoinc gentables scspntsc.bin scsppal.bin exchip.bin exchip_lut.h madrs.h madrs_pal.h cronsf.elf cronsf.bin cronsf.o fsys.o gfx.o nsfcore.o apu.o s6502.o s6502_s.o vrc6.o sun5b.o cronsf.ss cronsf-track1.iso cronsf-track2-nsfs.iso init68k.elf init68k.bin init68k.bin.h shim_6502.o shim_6502.bin shim_6502.bin.h test_stdlib.o test_stdlib.bin test_stdlib.elf test_stdlib.iso test_dsp.elf test_dsp.ss $(STDLIB_OBJS) $(GS_OBJS)
