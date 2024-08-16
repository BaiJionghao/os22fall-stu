#include "print.h"
#include "printk.h"
#include "sbi.h"
#include"riscv.h"
extern void test();
extern void _traps();
void start_time_trap()
{
    w_stvec(_traps);
    w_sie(r_sie()|SIE_STIE);
    sbi_ecall(0, 0, r_time() + 10000000, 0, 0, 0, 0, 0);
    w_sstatus(r_sstatus()|SSTATUS_SIE);
}
int start_kernel() {
    start_time_trap();
    printk("%d", 2022);
    puts(" Hello RISC-V\n");
    test(); // DO NOT DELETE !!!

	return 0;
}
