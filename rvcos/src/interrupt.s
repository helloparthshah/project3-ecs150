.section .text, "ax"
.global _interrupt_handler

_interrupt_handler:
    csrw    mscratch,ra
    csrr    ra,mcause
    addi    ra,ra,-11 #Sub 11 is 0 then ecall else something else
    bnez    ra,hardware_interrupt # if not system call then jump to hardware interrupt
    csrr    ra,mscratch
    addi    sp,sp,-8
    sw      ra,4(sp)
    sw      gp,0(sp)
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop
    call    c_syscall_handler
    lw      ra,4(sp)
    lw      gp,0(sp)
    addi    sp,sp,8
    csrw    mepc,ra
    mret

hardware_interrupt: # Saves the regs, calls the c handler and then restores the registers
    csrr    ra,mscratch
    addi	sp,sp,-40 # Move back 40 to save 10 regs
    sw	    ra,36(sp)
    sw	    t0,32(sp)
    sw	    t1,28(sp)
    sw	    t2,24(sp)
    sw	    a0,20(sp)
    sw	    a1,16(sp)
    sw	    a2,12(sp)
    sw	    a3,8(sp)
    sw	    a4,4(sp)
    sw	    a5,0(sp)
    call    c_interrupt_handler # Calling the c function
    lw	    ra,36(sp)
    lw	    t0,32(sp)
    lw	    t1,28(sp)
    lw	    t2,24(sp)
    lw	    a0,20(sp)
    lw	    a1,16(sp)
    lw	    a2,12(sp)
    lw	    a3,8(sp)
    lw	    a4,4(sp)
    lw	    a5,0(sp)
    addi    sp,sp,40
    mret
