#include"proc.h"
#include"mm.h"
#include"rand.h"
#include"defs.h"
#include"elf.h"
#include"printk.h"
#include"riscv.h"
//arch/riscv/kernel/proc.c
#define switch_reg(x) register uint64 x asm(#x);  prev->thread.x = x; x = next->thread.x;//only for my_switch_to
#define switch_sreg(x) register uint64 s##x asm("s"#x);  prev->thread.s[x] = s##x; s##x = next->thread.s[x];//only for my_switch_to
extern uint64 uapp_start;
extern uint64 uapp_end;
extern void __dummy();
extern void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
extern void copy_page_table(uint64* pre_table, uint64* new_table);
extern unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern int get_perm(uint64* pgtbl, uint64 va);
struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此
static uint64_t load_program(struct task_struct* task) {
    task->pgd = alloc_page();
    copy_page_table(swapper_pg_dir, task->pgd);
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)ADDRESS(uapp_start);
    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff;
    int phdr_cnt = ehdr->e_phnum;

    Elf64_Phdr* phdr;
    int load_phdr_cnt = 0;
    for (int i = 0; i < phdr_cnt; i++) {
        phdr = (Elf64_Phdr*)(phdr_start + sizeof(Elf64_Phdr) * i);
        if (phdr->p_type == PT_LOAD) {
            // do mapping
            // code...
            char* p;
            uint64 n = (((uint64)(phdr->p_memsz)+(uint64)phdr->p_vaddr%(1<<12)-1)>>12)+1;
            p = alloc_pages(n);
            char* t_va = ((uint64)phdr->p_offset)+(uint64_t)ehdr;
            for (int k = 0; k < phdr->p_memsz; k++) {
                p[k+(uint64)phdr->p_vaddr%(1<<12)] = t_va[k];
            }
            for (int j = 0; j < n; j++) {
                // printk("%lx\n", phdr->p_vaddr);
                create_mapping(task->pgd, (uint64)phdr->p_vaddr+(j<<12), p-PA2VA_OFFSET+(j<<12), 1<<12, 31);
            }
        }
    }
    // allocate user stack and do mapping
    // code...
    char* stack = alloc_page();
    create_mapping(task->pgd, USER_END-(1<<12), (uint64)stack-PA2VA_OFFSET, (1<<12), 23);
    // following code has been written for you
    // set user stack
    // pc for the user program
    task->thread.sepc = ehdr->e_entry;
    // sstatus bits set
    task->thread.sstatus = (r_sstatus()&(~SSTATUS_SPP))| SSTATUS_SPIE|(1<<18);
    // user stack for user program
    task->thread.sscratch = USER_END;
}
static uint64_t t_load_program(struct task_struct* task) {
    task->pgd = alloc_page();
    copy_page_table(swapper_pg_dir, task->pgd);
    char* p;
    uint64 n = (((uint64)(ADDRESS(uapp_end)-ADDRESS(uapp_start))-1)>>12)+1;
    p = alloc_pages(n);
    char* t_va = ADDRESS(uapp_start);
    for (int k = 0; k < ADDRESS(uapp_end)-ADDRESS(uapp_start); k++) {
        p[k] = t_va[k];
    }
    for (int j = 0; j < n; j++) {
        create_mapping(task->pgd, USER_START+(j<<12), p-PA2VA_OFFSET+(j<<12), 1<<12, 31);
    }
    char* stack = alloc_page();
    create_mapping(task->pgd, USER_END-(1<<12), (uint64)stack-PA2VA_OFFSET, (1<<12), 23);
    // following code has been written for you
    // set user stack
    // pc for the user program
    task->thread.sepc = USER_START;
    // sstatus bits set
    task->thread.sstatus = (r_sstatus()&(~SSTATUS_SPP))| SSTATUS_SPIE|(1<<18);
    // user stack for user program
    task->thread.sscratch = USER_END;
}
void task_init() {
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    idle = kalloc();
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;
    /* YOUR CODE HERE */
    for (int i = 1; i < NR_TASKS; i++) {
        task[i] = kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand()%(PRIORITY_MAX-PRIORITY_MIN+1)+PRIORITY_MIN;
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;
        load_program(task[i]);
    }
    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */

    printk("...proc_init done!\n");
}
// arch/riscv/kernel/proc.c

void dummy() {
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    printk("0\n");
    while(1) {
        if (last_counter == -1 || current->counter != last_counter) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            // printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
            printk("[PID = %d] is running! thread space begin at 0xffffffe0%x \n", current->pid, (((uint64)&MOD)>>12)<<12);
        }
    }
}
// arch/riscv/kernel/proc.c

// extern void __switch_to(struct task_struct* prev, struct task_struct* next);
void my_switch_to(struct task_struct* prev, struct task_struct* next)
{
    prev->thread.sepc = r_sepc();
    w_sepc(next->thread.sepc);
    prev->thread.sscratch = r_sscratch();
    w_sscratch(next->thread.sscratch);
    prev->thread.sstatus = r_sstatus();
    w_sstatus(next->thread.sstatus);
    register uint64 t0 asm("t0");
    t0 = ((uint64)8<<60)+(((uint64)next->pgd-PA2VA_OFFSET)>>12);
    asm volatile("csrw satp, t0");
    asm volatile("sfence.vma zero, zero");
    // flush icache
    asm volatile("fence.i");
    // printk("the perm at page va USER_START: %d\n",get_perm((r_satp()<<12)+PA2VA_OFFSET, USER_START));
    // printk("%d\n", (r_sstatus()>>8)&1);
    // switch_reg(ra);
    // switch_reg(sp);
    // switch_sreg(1);
    // switch_sreg(2);
    // switch_sreg(3);
    // switch_sreg(4);
    // switch_sreg(5);
    // switch_sreg(6);
    // switch_sreg(7);
    // switch_sreg(8);
    // switch_sreg(9);
    // switch_sreg(10);
    // switch_sreg(11);
    return;
    // asm volatile("ret");
}
void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if (next == current) {
        return;
    } else {
        struct task_struct* tmp = current;
        current = next;
        // printk("\nswitch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
        my_switch_to(tmp, next);
        __switch_to(tmp, next);
    }
}
void do_timer(void) {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
    if (current == idle) {
        schedule();
    } else {
        current->counter--;
        if (current->counter > 0) {
            return;
        } else {
            schedule();
        }
    }
    /* YOUR CODE HERE */
    return;
}
void schedule(void) {
    /* YOUR CODE HERE */
    uint64 next = current;
    #ifdef SJF
    while (1) {
		int c = INF;
		next = 0;
		uint64 i = NR_TASKS;
		struct task_struct** p = &task[NR_TASKS];
		while (--i) {
			if (!(*--p))
				continue;
			if ((*p)->state == TASK_RUNNING && (int)((*p)->counter) < c && (*p)->counter != 0)
				c = (*p)->counter, next = i;
		}
		if (c != INF) break;
        printk("\n");
		for(p = &task[NR_TASKS-1]; p > &task[0] ; --p)
			if (*p)
				(*p)->counter = rand()%(PRIORITY_MAX-PRIORITY_MIN+1)+PRIORITY_MIN;
	}
    
    #else
    while (1) {
		int c = -1;
		next = 0;
		uint64 i = NR_TASKS;
		struct task_struct** p = &task[NR_TASKS];
		while (--i) {
			if (!(*--p))
				continue;
			if ((*p)->state == TASK_RUNNING && (int)((*p)->counter) > c)
				c = (*p)->counter, next = i;
		}
		if (c) break;
		for(p = &task[NR_TASKS-1]; p > &task[0] ; --p)
			if (*p)
				(*p)->counter = ((*p)->counter >> 1) + (*p)->priority;
	}
    #endif
	switch_to(task[next]);
    return;
}