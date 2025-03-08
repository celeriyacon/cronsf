/**/
OUTPUT_ARCH(sh2)

MEMORY
{
 hiram : ORIGIN = 0x06004000, LENGTH = 0xBC000 - 36
 hiram_uncached : ORIGIN = 0x260C0000 - 36, LENGTH = 256K + 36

 loram	(w!xi) : ORIGIN = 0x00200000 + (1M - 152K), LENGTH = 152K
 loram_uncached	(w!xi) : ORIGIN = 0x20200000, LENGTH = 1M - 152K

 vdp1_vram_uncached (w!xi) : ORIGIN = 0x25C00000, LENGTH = 512K
 vdp2_vram (w!xi) : ORIGIN = 0x25E24000, LENGTH = 0x80000 - 0x24000
}

SECTIONS
{
 HIRAM          		: { *(.init*) *(.text.cold) *(.text*) *(.rodata*) *(.data*) *(.eh_frame)} >hiram

 .sun5b_reloc 0xC0000000	: AT(LOADADDR(HIRAM)+SIZEOF(HIRAM)) { *(.sun5b_reloc) }
 _sun5b_reloc_dest = ADDR(.sun5b_reloc);
 _sun5b_reloc_start = LOADADDR(.sun5b_reloc);
 _sun5b_reloc_bound = LOADADDR(.sun5b_reloc) + SIZEOF(.sun5b_reloc);

 HIRAM_BSS (ADDR(HIRAM) + SIZEOF(HIRAM) + SIZEOF(.sun5b_reloc)) (NOLOAD) : { bss_start = .; *(.bss*) bss_bound = .;} >hiram
 HIRAM_BSS_UNCACHED (NOLOAD)	: { *(.hiram.bss.uncached) } >hiram_uncached

 LORAM_BSS (NOLOAD)		: { *(.loram.bss) } >loram
 LORAM_BSS_UNCACHED (NOLOAD)	: { *(.loram.bss.uncached.exchip_rb) *(.loram.bss.uncached) } >loram_uncached

 VDP1_BSS_UNCACHED (NOLOAD)	: { *(.vdp1.bss.uncached) } >vdp1_vram_uncached
 VDP2_BSS (NOLOAD)		: { *(.vdp2.bss) } >vdp2_vram
}

