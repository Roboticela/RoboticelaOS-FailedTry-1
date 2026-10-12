ENTRY(RoboKernelMain)

PHDRS
{
    robo PT_LOAD FLAGS(7);
}

SECTIONS
{
    . = 0x42000000;

    .text : ALIGN(2048)
    {
        *(.text.vectors)
        *(.text .text.*)
    } :robo

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    } :robo

    .data : ALIGN(16)
    {
        *(.data .data.*)
    } :robo

    .bss : ALIGN(16)
    {
        *(COMMON)
        *(.bss .bss.*)
    } :robo

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment)
        *(.note*)
        *(.llvm_addrsig)
    }
}
