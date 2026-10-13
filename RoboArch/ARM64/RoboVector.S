.section .text.vectors, "ax"
.balign 2048
.global RoboVectorTable
RoboVectorTable:
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorSync
    .balign 128
    b RoboVectorLowerSync
    .balign 128
    b RoboVectorIrq
    .balign 128
    b RoboVectorIrq
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

.macro RoboSaveCaller
    stp x0, x1, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x18, x30, [sp, #-16]!
.endm

.macro RoboRestoreCaller
    ldp x18, x30, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x0, x1, [sp], #16
.endm

.global RoboVectorSync
RoboVectorSync:
    b RoboVectorSync

.global RoboVectorIrq
RoboVectorIrq:
    RoboSaveCaller
    bl RoboArmInterrupt
    RoboRestoreCaller
    eret

.global RoboVectorLowerSync
RoboVectorLowerSync:
    RoboSaveCaller
    mov x0, sp
    bl RoboArmLowerFrame
    RoboRestoreCaller
    eret
