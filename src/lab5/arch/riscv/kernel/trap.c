#include "types.h"
#include "printk.h"
#include "proc.h"
#include"sbi.h"
extern void clock_set_next_event();
struct pt_regs {
    uint64 sepc;
    uint64 x[32];
    uint64 sstatus;
};
extern struct task_struct* current;
void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs)
{
    // 通过 `scause` 判断trap类型

    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    // YOUR CODE HERE
    uint64 m = 1;
    if ((scause >> 63) == 0) {
        if ((scause & ~(m << 63)) == 8) {
            regs->sepc += 4;
            if (regs->x[17] == 64) {
                char t[regs->x[12]+1];
                for (int i = 0; i < regs->x[12]; i++) {
                    t[i] = ((char*)regs->x[11])[i];
                }
                t[regs->x[12]] = 0;
                regs->x[10] = printk("%s", t);
            } else if (regs->x[17] == 172) {
                regs->x[10] = current->pid;
            }
        } else {
            printk("unhandled trap: %llx\n", scause);
            printk("%lx\n", regs->sstatus);
            printk("%lx\n", regs->sepc);
            while(1);
        }
    } else {
        if ((scause & ~(m << 63)) == 5) {
            clock_set_next_event();
            do_timer();
        } else {
            printk("unhandled trap: %llx\n", scause);
            printk("%lx\n", regs->sstatus);
            printk("%lx\n", regs->sepc);
            while(1);
        }
    }
    // printk("kernel is running!\n[S] Supervisor Mode Timer Interrupt\n");
    return;
    
}