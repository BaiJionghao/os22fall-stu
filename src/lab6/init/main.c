#include "print.h"
#include "printk.h"
#include "sbi.h"
#include"riscv.h"
#include"proc.h"
extern void test();
extern void _traps();
extern void setup_vm_final();
void start_time_trap()
{
    w_stvec(_traps);
    w_sie(r_sie()|SIE_STIE);
    sbi_ecall(0, 0, r_time() + 10000000, 0, 0, 0, 0, 0);
    // w_sstatus(r_sstatus()|SSTATUS_SIE);
}
int start_kernel() {
    start_time_trap();
    mm_init();
    setup_vm_final();
    int i;
    i = 12345;
    task_init();
    printk("[S] %d ", 2022);
    puts("Hello RISC-V\n");
    printk("[S] Value of sstatus is %lx\n", r_sstatus());
    do_timer();
    test(); // DO NOT DELETE !!!

	return 0;
}
