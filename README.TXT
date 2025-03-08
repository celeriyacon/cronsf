CroNSF is a basic NSF player for the Sega Saturn.

Due to heavy reliance on SCSP functionality, including the SCSP DSP, CroNSF
will not work properly on emulators with grossly-inaccurate SCSP emulation;
run CroNSF on real hardware, Mednafen, or a recent version of srg320's
MiSTer Saturn core.

NSFs that require more than one expansion chip are not supported.  Emulation
of NSFs that use expansion chips is far more susceptible to running out of
CPU time if APU or expansion chip registers are written to excessively.
The following expansion chips are supported, rendered on the slave CPU:
	Konami VRC6
	Namco 163 (support for reading phase accumulators is unimplemented)
	Nintendo MMC5 (PCM read mode is not supported)
	Sunsoft 5B (sans janky non-linear amplifier)

IRQs are not supported.

ROM data size is limited to 640KiB(the NSF format itself can support
around 1MiB).

The APU emulation is designed to sound decent with most
commercially-released NES games from the 1980s and 1990s.
Unconventional music or homebrew sound drivers that fully exploit all
of the NES APU's quirks and nuances may fail spectacularly.

Sound drivers that are sloppy with register writes will produce more
crackles than they should, due to 44.1KHz sample timing granularity
limitations; there are, however, a few kludges implemented to mitigate
this issue with the triangle wave channel, where crackles are more
obnoxious.

PCM/DMC is not properly resampled to 44.1KHz, due to resource constraints,
so it may sound harsher than it should.

NSFs that execute code or relative branches across non-contiguous
ROM banks(or ROM<->RAM 16-bit wraparound) will break due to the
6502 emulator design.  The issue can be fixed fairly easily, but
doing so will significantly hurt performance.

NSFs that write to APU hardware addresses continuously in tight
loops will cause the emulator to use too much CPU time, breaking the APU
waveform playback(control ringbuffer/FIFO to the SCSP DSP program will
underflow and things will go haywire).

--------------------------------------------------------------------------------

General Controls:
	Z - Toggle hires mode(ignored in hex viewer, which is always hires).
	Start - Load selected file; Exit to file selector.


File Selector Controls:
	A - Load selected file/directory.
	B - Ascend to parent directory.
	C - Load file in hex viewer.

	Up/Down - Scroll up/down, with wrapping.
	X/Y - Scroll up/down by 12.
	L/R - Scroll up/down by 84.


NSF Player Controls:
	A - Restart song.
	B(hold) - Exit to file selector.

	Left/Right - Change song -1/+1, with wrapping.
	Up/Down - Change song -10/+10.
	X/Y - Change song -50/+50.


Hex/Text Viewer Controls:
	B(hold) - Exit to file selector.

	Up/Down - Scroll up/down.
	X/Y - Scroll up/down by 12.
	L/R - Scroll up/down by 84.


--------------------------------------------------------------------------------

Bundled NSFs can be replaced with the user's choice by generating a new
ISO-9660 filesystem image to replace "cronsf-track2-nsfs.iso".  For nice
filenames, generate the filesystem to "ISO 9660 level 4" or "ISO 9660:1999"
specifications, as CroNSF currently supports neither Rock Ridge nor Joliet
extensions.

"cronsf-track1.iso" will also function as a bootable ROM cart image, in which
case CroNSF will load NSFs from the first track of an inserted CD, rather
than the second track.  For an entirely self-contained ROM cart image, build
CroNSF from source code to generate a "cronsf.ss" image that will include all
files in the "nsfs" subdirectory; the "cronsf.ss" file may also be used as
a single-track bootable ISO-9660 CD image.

--------------------------------------------------------------------------------

Building CroNSF requires cross-compiling GNU toolchains targeting the SH-2
and M68K, the 6502 cross-assembler "xa", and the ISO-9660 filesystem
generator "xorriso".

NSFs should be placed in the "nsfs" subdirectory before building, so
that they will be included in the generated track 2 ISO image.

The included "stdlib" ideally shouldn't be used in other projects,
as it is incomplete by design and probably buggy.
