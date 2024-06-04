/**/
OUTPUT_ARCH(sh2)

MEMORY
{
 hiram : ORIGIN = 0x06004000, LENGTH = 0xFC000
 loram (w!xi) : ORIGIN = 0x00200000, LENGTH = 1M - 640K
 loram_uncached	(w!xi) : ORIGIN = 0x20260000, LENGTH = 640K
 vdp1_vram_uncached (w!xi) : ORIGIN = 0x25C00000, LENGTH = 512K
 vdp2_vram (w!xi) : ORIGIN = 0x25E40000, LENGTH = 256K
}

SECTIONS
{
 HIRAM          		: { *(.init*) *(.text.cold) *(.text*) *(.rodata*) *(.data*) *(.eh_frame)} >hiram
 HIRAM_BSS (NOLOAD)		: { bss_start = .; *(.bss*) bss_bound = .;} >hiram

 LORAM_BSS (NOLOAD)		: { *(.loram.bss) } >loram
 LORAM_BSS_UNCACHED (NOLOAD)	: { *(.loram.bss.uncached) } >loram_uncached

 VDP1_BSS_UNCACHED (NOLOAD)	: { *(.vdp1.bss.uncached) } >vdp1_vram_uncached
 VDP2_BSS (NOLOAD)		: { *(.vdp2.bss) } >vdp2_vram
}

