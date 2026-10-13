ENTRY(RoboAppMain)
SECTIONS {
    . = 0x7FFF00000000;
    .text : {
        *(.text.startup)
        *(.text .text.*)
        *(.rodata .rodata.*)
    }
}
