#include "types.h"
#include "printk.h"
#include "proc.h"
#include"sbi.h"
#include"mm.h"
#include"rand.h"
#include"defs.h"
#include"elf.h"
#include"riscv.h"
extern void clock_set_next_event();
extern void __ret_from_fork();
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern int get_perm(uint64* pgtbl, uint64 va);
extern uint64 get_address(uint64* pgtbl, uint64 va);
extern uint64 ramdisk_start;
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
struct pt_regs {
    uint64 sepc;
    uint64 x[32];
    uint64 sstatus;
    uint64 stval;
};
extern struct task_struct* current;
extern struct task_struct* task[NR_TASKS];
extern uint64 task_num;
extern uint64_t load_program6(struct task_struct* task);
void do_page_fault(struct pt_regs *regs) {
    uint64 address = regs->stval; //address为出现缺页报错的地址
    int t = get_perm((r_satp()<<12)+PA2VA_OFFSET, address);
    //这里得到address处是否有分配页，以及如果分配有页该页的参数值。
    //get_perm函数lab5有实现，只有返回值大于0，才能证明有分配页，此时返回值为该页的参数值。否则证明这里没有分配页
    if (t > 0) {
        //这里进行判断，只有尚未分配页，程序才能正常运行。如果有分配页，则理应不会出现page fault。
        //这里达到类似断言的效果
        printk("%d\n", t);
        while(1);
    }
    struct vm_area_struct* f = find_vma(current, address);//找到address处的vma的地址，如果没有相关的vma则返回值为NULL
    if (f == NULL) {
        //我们程序不涉及找不到vma的情况，这里对vma的查询情况做断言。即只有查询到了vma地址程序才能继续运行。
        printk("fault!\n");
        while(1);
    }
    char* p = alloc_page();//该页就是为缺页的用户态地址分配的。
    if (f->vm_flags) {
        //这里是非匿名空间
        uint64 start_address = ADDRESS(ramdisk_start) + f->vm_content_offset_in_file;
        char* sourse_start = start_address + (address - f->vm_start) - (address % (1<<12));
        //上述两行目的在于找到新分配的页的原文件的相应字节段的起始地址是何处。
        for (int i = 0; i < 1<<12; i++) {
            p[i] = sourse_start[i];//将ramdisk上的文件复制到新页中。
        }
        uint64 r = (f->vm_flags)>>2;
        uint64 w = ((f->vm_flags)>>1) & 1;
        uint64 e = (f->vm_flags) & 1;
        create_mapping(current->pgd, (uint64)address, p-PA2VA_OFFSET, 1<<12, e*8+w*4+r*2+1+16);
        //为新页创建页表映射
    } else {
        //这里是匿名空间
        for (int i = 0; i < 1<<12; i++) {
            p[i] = 0;//将新页数据全部置零
        }
        create_mapping(current->pgd, (uint64)address, p-PA2VA_OFFSET, 1<<12, (VM_R_MASK | VM_W_MASK)+1+16);
        //为新页创建页表映射
    }
}
uint64_t sys_clone(struct pt_regs *regs) {
    if (task_num >= NR_TASKS) {
        //如果进程数已经达到最大进程数，则报错返回（即a0设为-1）
        regs->x[10] = -1;
        return;
    }
    uint64 i = task_num++;//创建了新的进程，进程数加一
    task[i] = kalloc();//上面i得到的值为新创建进程的索引，这里为该进程分配task_struct以及内核栈的空间。
    for (int j = 0; j < (1<<12); j++) {
        *((char*)(task[i])+j) = *((char*)(current)+j);
        //这里先将父进程的内核态信息页完全复制到子进程的内核态信息页中。
    }
    task[i]->pid = i;//设置子进程id
    task[i]->thread.ra = (uint64)__ret_from_fork;//子进程的返回地址与父进程不同，需要单独处理
    task[i]->thread.sp = (uint64)regs - (uint64)current + (uint64)task[i];  //计算出子进程被context_switch后栈指针应该指向哪里
    task[i]->thread.sscratch = r_sscratch();//子进程的用户态栈指针为当前父进程的用户态栈指针所处位置（用户态虚拟地址相同）。
    //父进程只有发生context_switch是才会把task_struct中的sscratch更新为实时的sscratch，因此此时的current->thread.sscratch不可靠。

    struct pt_regs * child_regs = (uint64)regs - (uint64)current + (uint64)task[i];
    //找到子进程的regs位置。
    child_regs->x[10] = 0;//子进程a0设置为0
    child_regs->x[2] = regs->x[2] - (uint64)current + (uint64)task[i];//子进程sp通过父进程sp的计算得到。
    task[i]->pgd = alloc_page();//为子进程申请根页表
    copy_page_table(swapper_pg_dir, task[i]->pgd);//现将内核态的全部拷贝到新建的页表中，这是深度拷贝。
    for (int j = 0; j < task[i]->vma_cnt; j++) {
        //循环遍历所有的vma，找到父进程已经分配的页，为子进程创建新的页并完成内容复制和页表映射。

        struct vm_area_struct vma = task[i]->vmas[j];//该次循环的vma
        for (uint64 k = (vma.vm_start>>12) << 12; k < vma.vm_end; k += (1 << 12)) {
            //遍历所有该vma涉及到的页的初始地址。（初始地址代表了这一整张页）
            int perm = get_perm(current->pgd, k);//该函数的判定是，perm小于0则未在页表中找到该地址。大于零则为该对应页的属性权限。
            if (perm > 0) {//此时意味着父进程已经对该页进行了分配，因此子进程要创建新的页并完成内容复制和页表映射
                char* p = alloc_page();//创建新的页
                char* father_p = get_address(current->pgd, k);//找到父进程分配的该页的起始地址（内核态虚拟地址）
                for (int l = 0; l < (1 << 12); l++) {
                    p[l] = father_p[l];//完成内容复制
                }
                create_mapping(task[i]->pgd, k, p-PA2VA_OFFSET, 1<<12, perm);//完成页表映射
            }
        }
    }
    regs->x[10] = task[i]->pid;//父进程的返回值设置为子进程的pid
    printk("[S] New task: %d\n", i);
    return task[i]->pid;//函数的返回值为子进程的pid
}
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
            } else if (regs->x[17] == 220) {
                sys_clone(regs);
            }
        } else if (((scause & ~(m << 63)) == 12)||((scause & ~(m << 63)) == 13)||((scause & ~(m << 63)) == 15)) {
            printk("[S] Supervisor Page Fault, scause: %lx, stval: %lx, sepc: %lx\n", scause, regs->stval, regs->sepc);
            do_page_fault(regs);
        }else {
            printk("unhandled trap: %llx\n", scause);
            printk("sstatus: %lx\n", regs->sstatus);
            printk("sepc: %lx\n", regs->sepc);
            while(1);
        }
    } else {
        if ((scause & ~(m << 63)) == 5) {
            clock_set_next_event();
            printk("[S] Supervisor Mode Timer Interrupt\n");
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