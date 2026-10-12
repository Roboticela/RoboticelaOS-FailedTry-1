ENTRY(RoboKernelMain)

PHDRS
{
    robo PT_LOAD FLAGS(7);
}

SECTIONS
{
    /* Keep in sync with ROBO_KERNEL_LOAD_ADDRESS in RoboInclude/RoboBootInfo.hpp */
    . = 0x08000000;

    .text : ALIGN(16)
    {
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
