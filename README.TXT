CroNSF is a basic NSF player for the Sega Saturn.

Expansion sound chips and IRQs are not supported.

ROM data size is limited to 896KiB(the NSF format itself can support
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

Bundled NSFs can be replaced with the user's choice by generating a new
ISO-9660 filesystem image to replace "cronsf-track2-nsfs.iso".  For nice
filenames, generate the filesystem to "ISO 9660 level 4" or "ISO 9660:1999"
specifications, as CroNSF currently supports neither Rock Ridge nor Joliet
extensions.

--------------------------------------------------------------------------------

Building CroNSF requires cross-compiling GNU toolchains targeting the SH-2
and M68K, the 6502 cross-assembler "xa", and the ISO-9660 filesystem
generator "xorriso".

NSFs should be placed in the "nsfs" subdirectory before building, so
that they will be included in the generated track 2 ISO image.

The included "stdlib" ideally shouldn't be used in other projects,
as it is incomplete by design and probably buggy.
