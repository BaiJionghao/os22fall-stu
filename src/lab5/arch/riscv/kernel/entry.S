    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    # YOUR CODE HERE
    # -----------

        # 1. save 32 registers and sepc to stack
        csrr t0, sscratch
        beq t0, x0, _traps_kernel; # if t0 == t1 then target
        
        csrw sscratch, sp 
        add sp, t0, x0 
        _traps_kernel:
        addi sp, sp, -272
        sd x0, 8(sp)
        sd x1, 16(sp)
        sd x2, 24(sp)
        sd x3, 32(sp)
        sd x4, 40(sp)
        sd x5, 48(sp)
        sd x6, 56(sp)
        sd x7, 64(sp)
        sd x8, 72(sp)
        sd x9, 80(sp)
        sd x10, 88(sp)
        sd x11, 96(sp)
        sd x12, 104(sp)
        sd x13, 112(sp)
        sd x14, 120(sp)
        sd x15, 128(sp)
        sd x16, 136(sp)
        sd x17, 144(sp)
        sd x18, 152(sp)
        sd x19, 160(sp)
        sd x20, 168(sp)
        sd x21, 176(sp)
        sd x22, 184(sp)
        sd x23, 192(sp)
        sd x24, 200(sp)
        sd x25, 208(sp)
        sd x26, 216(sp)
        sd x27, 224(sp)
        sd x28, 232(sp)
        sd x29, 240(sp)
        sd x30, 248(sp)
        sd x31, 256(sp)
        csrr t0, sepc
        sd t0, 0(sp)
        csrr t0, sstatus
        sd t0, 264(sp)

    # -----------

        # 2. call trap_handler
        csrr a0, scause 
        csrr a1, sepc 
        mv a2,  sp
        call trap_handler

    # -----------

        # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack

        ld t0, 0(sp)
        csrw sepc, t0

        ld t0, 264(sp)
        csrw sstatus, t0
        ld x0, 8(sp)
        ld x1, 16(sp)
        ld x3, 32(sp)
        ld x4, 40(sp)
        ld x5, 48(sp)
        ld x6, 56(sp)
        ld x7, 64(sp)
        ld x8, 72(sp)
        ld x9, 80(sp)
        ld x10, 88(sp)
        ld x11, 96(sp)
        ld x12, 104(sp)
        ld x13, 112(sp)
        ld x14, 120(sp)
        ld x15, 128(sp)
        ld x16, 136(sp)
        ld x17, 144(sp)
        ld x18, 152(sp)
        ld x19, 160(sp)
        ld x20, 168(sp)
        ld x21, 176(sp)
        ld x22, 184(sp)
        ld x23, 192(sp)
        ld x24, 200(sp)
        ld x25, 208(sp)
        ld x26, 216(sp)
        ld x27, 224(sp)
        ld x28, 232(sp)
        ld x29, 240(sp)
        ld x30, 248(sp)
        ld x31, 256(sp)
        ld x2, 24(sp)
        addi sp, sp, 272

        csrr t0, sscratch
        csrw sscratch, sp 
        add sp, t0, x0 
    # -----------

        # 4. return from trap
        sret 
    # -----------
# arch/riscv/kernel/entry.S

    .global __dummy
__dummy:
    # YOUR CODE HERE
    

# -----------

    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    # la t0, dummy
    # csrw sepc, t0
    csrr t0, sscratch
    csrw sscratch, sp 
    add sp, t0, x0 
# -----------

    # 4. return from trap
    sret 
    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE

    # restore state from next process
    # YOUR CODE HERE
    sd ra, 40(a0)
    sd sp, 48(a0)
    sd s0, 56(a0)
    sd s1, 64(a0)
    sd s2, 72(a0)
    sd s3, 80(a0)
    sd s4, 88(a0)
    sd s5, 96(a0)
    sd s6, 104(a0)
    sd s7, 112(a0)
    sd s8, 120(a0)
    sd s9, 128(a0)
    sd s10, 136(a0)
    sd s11, 144(a0)

    ld ra, 40(a1)
    ld sp, 48(a1)
    ld s0, 56(a1)
    ld s1, 64(a1)
    ld s2, 72(a1)
    ld s3, 80(a1)
    ld s4, 88(a1)
    ld s5, 96(a1)
    ld s6, 104(a1)
    ld s7, 112(a1)
    ld s8, 120(a1)
    ld s9, 128(a1)
    ld s10, 136(a1)
    ld s11, 144(a1)
    ret