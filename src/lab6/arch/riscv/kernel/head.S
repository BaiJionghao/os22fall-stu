.extern start_kernel
.extern setup_vm
.extern relocate
    .section .text.init
    .globl _start
_start:
        # YOUR CODE HERE

    # ------------------

        # set stvec = _traps

    # ------------------

        # set sie[STIE] = 1

    # ------------------

        # set first time interrupt

    # ------------------

        # set sstatus[SIE] = 1

    # ------------------
    # ------------------
    # - your code here -
    # ------------------
    call setup_vm
    call relocate
    la sp, boot_stack_top
    j start_kernel 
# relocate:
#     # set ra = ra + PA2VA_OFFSET
#     # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
#     addi ra, ra, PA2VA_OFFSET # ra =ra+ PA2VA_OFFSET
#     addi sp, sp, PA2VA_OFFSET
#     ###################### 
#     #   YOUR CODE HERE   #
#     ######################

#     # set satp with early_pgtbl
#     la t1, early_pgtbl # 
#     csrw satp, t1
#     ###################### 
#     #   YOUR CODE HERE   #
#     ######################

#     # flush tlb
#     sfence.vma zero, zero

#     # flush icache
#     fence.i

#     ret
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space  1024*4# <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: