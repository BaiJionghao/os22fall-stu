#include"proc.h"
#include"mm.h"
#include"rand.h"
#include"defs.h"
//arch/riscv/kernel/proc.c

extern void __dummy();

struct task_struct* idle;           // idle process
struct task_struct* current;        // 指向当前运行线程的 `task_struct`
struct task_struct* task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

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
        task[i]->priority = rand();
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;
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

extern void __switch_to(struct task_struct* prev, struct task_struct* next);

void switch_to(struct task_struct* next) {
    /* YOUR CODE HERE */
    if (next == current) {
        return;
    } else {
        struct task_struct* tmp = current;
        current = next;
        printk("\nswitch to [PID = %d COUNTER = %d]\n", next->pid, next->counter);
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
		for(p = &task[NR_TASKS-1]; p > &task[0] ; --p)
			if (*p)
				(*p)->counter = rand();
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