.arm
.section .text


@ Shamelessly based from Steveice's memchunkhax2 repo. I miss those old days
@ Credits to TuxSH for finding this leak
@ Please don't expect KTM for this
.global svcCreateSemaphoreKAddr
.type   svcCreateSemaphoreKAddr, %function
svcCreateSemaphoreKAddr:
    str r0, [sp, #-4]!
    str r3, [sp, #-4]!
    svc 0x15
    ldr r3, [sp], #4
    sub r2, r2, #4                                @ Fix the kobject ptr
    str r2, [r3]
    ldr r3, [sp], #4
    str r1, [r3]
    bx lr


@ TuxSH definitely is the bestest
.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    mov r1, #0x1000
    sub r1, #1
    and r2, r0, r1
    bic r0, r1
    mcr p15, 0, r0, c7, c8, 0    @ VA to PA translation with privileged read permission check
    mrc p15, 0, r0, c7, c4, 0    @ read PA register
    tst r0, #1                   @ failure bit
    bic r0, r1
    addeq r0, r2
    movne r0, #0
    bx lr


.global flushEntireCaches
.type   flushEntireCaches, %function
flushEntireCaches:
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 0  @ clean entire DCache
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 5
    mcr p15, 0, r0, c7, c5,  0  @ invalidate the entire ICache & branch target cache
    mcr p15, 0, r0, c7, c10, 4  @ data synchronization barrier
    bx lr


@ Original svcBackdoor
@ 100% original pls do not copy. you wouldn't download a car.
.global svcBackdoor_original
.type svcBackdoor_original, %function
svcBackdoor_original:
    bic r1, sp, #0xff
    orr r1, r1, #0xf00
    add r1, r1, #0x28
    ldr r2, [r1]
    stmdb r2!, {sp, lr}
    mov sp, r2
    blx r0
    pop {r0, r1}
    mov sp, r0
    bx r1
svcBackdoor_original_end:


.global svc_7b
.type   svc_7b, %function
svc_7b:
    push {r0, r1, r2}
    mov r3, sp
    add r0, pc, #12
    svc 0x7b
    add sp, sp, #8
    ldr r0, [sp], #4
    bx lr
    cpsid aif
    ldr r2, [r3], #4
    ldmfd r3!, {r0, r1}
    push {r3, lr}
    blx r2
    pop {r3, lr}
    str r0, [r3, #-4]!
    mov r0, #0
    bx lr



.section .rodata

.global svcBackdoor_original_size
svcBackdoor_original_size:
    .word (svcBackdoor_original_end - svcBackdoor_original)
