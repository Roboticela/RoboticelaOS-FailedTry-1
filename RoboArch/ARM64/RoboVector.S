.section .text.vectors, "ax"
.balign 2048
.global RoboVectorTable
RoboVectorTable:
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync

.global RoboVectorSync
RoboVectorSync:
    wfi
    b RoboVectorSync

.global RoboVectorIrq
RoboVectorIrq:
    eret
