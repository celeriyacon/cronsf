/*OUTPUT_FORMAT(binary)*/
OUTPUT_ARCH(sh2)

MEMORY
{
 ipl : ORIGIN = 0x06002F00, LENGTH = 0x0500
}

SECTIONS
{
 IPL : { *(.init*) *(.text*) *(.rodata*) *(.data*) *(.eh_frame)} >ipl
}
