#include"defs.h"
#include"riscv.h"
#include"mm.h"
#include"printk.h"
#include"string.h"
// arch/riscv/kernel/vm.c
extern uint64 _start;
extern uint64 _srodata;
extern uint64 _sdata;
/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long  early_pgtbl[512] __attribute__((__aligned__(0x1000)));
void relocate(void) {
    register uint64 ra asm("ra");
    register uint64 t0 asm("t0");
    // register uint64 sp asm("sp");
    ra = ra + PA2VA_OFFSET;
    // sp = sp + PA2VA_OFFSET;
    t0 = ((uint64)8<<60)+((uint64)early_pgtbl>>12);
    asm volatile("csrw satp, t0");
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    asm volatile("ret");
}
void setup_vm(void) {
    /* 
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表 
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。 
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    early_pgtbl[(PHY_START>>30)&511] = (PHY_START>>2)+15;
    early_pgtbl[(VM_START>>30)&511] = (PHY_START>>2)+15;
    return;
}
// arch/riscv/kernel/vm.c 

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
int get_perm(uint64* pgtbl, uint64 va)
{
    uint64 v2 = (va >> 30)&511;
    uint64 v1 = (va >> 21)&511;
    uint64 v0 = (va >> 12)&511;
    unsigned long * p_now;
    p_now = pgtbl;
    if ((p_now[v2]&1) == 0) {
        return -2;
    }
    p_now = PA2VA_OFFSET+(uint64)(((p_now[v2])>>10)<<12);
    if ((p_now[v1]&1) == 0) {
        return -1;
    }
    p_now = PA2VA_OFFSET+(uint64)(((p_now[v1])>>10)<<12);
    return p_now[v0]&((1<<10)-1);
}
uint64 get_address(uint64* pgtbl, uint64 va)
{
    uint64 v2 = (va >> 30)&511;
    uint64 v1 = (va >> 21)&511;
    uint64 v0 = (va >> 12)&511;
    unsigned long * p_now;
    p_now = pgtbl;
    if ((p_now[v2]&1) == 0) {
        return 0;
    }
    p_now = PA2VA_OFFSET+(uint64)(((p_now[v2])>>10)<<12);
    if ((p_now[v1]&1) == 0) {
        return 0;
    }
    p_now = PA2VA_OFFSET+(uint64)(((p_now[v1])>>10)<<12);
    return PA2VA_OFFSET+(uint64)(((p_now[v0])>>10)<<12) + (va & 0xfff);
}
void setup_vm_final(void) {
    memset(swapper_pg_dir, 0x0, PGSIZE);
    
    // No OpenSBI mapping required
    // printk("%x\n", (uint64)&_start>>32);
    // mapping kernel text X|-|R|V
    for (int i = 0; i < (((uint64)&_srodata-(uint64)&_start)>>12); i++) {
        // printk("%d\n", i);
        create_mapping(swapper_pg_dir,0xffffffe000200000 + (i<<12), 0x0000000080200000 + (i<<12), 1 << 12, 11);
    }
    // create_mapping(swapper_pg_dir,0xffffffe000200000, 0x0000000080200000, 1 << 12, 11);
    // create_mapping(swapper_pg_dir,0xffffffe000201000, 0x0000000080201000, 1 << 12, 11);
    // create_mapping(swapper_pg_dir,0xffffffe000202000, 0x0000000080202000, 1 << 12, 11);
    // printk("text: %x, %x\n", get_perm(swapper_pg_dir, 0xffffffe000200000), get_perm(swapper_pg_dir, 0xffffffe000201000));
    // mapping kernel rodata -|-|R|V
    for (int i = (((uint64)&_srodata-(uint64)&_start)>>12); i < (((uint64)&_sdata-(uint64)&_start)>>12); i++) {
        // printk("%d\n", i);
        create_mapping(swapper_pg_dir,0xffffffe000200000 + (i<<12), 0x0000000080200000 + (i<<12), 1 << 12, 3);
    }
    // create_mapping(swapper_pg_dir,0xffffffe000203000, 0x0000000080203000, 1 << 12, 3);
    // printk("rodata: %x\n", get_perm(swapper_pg_dir, 0xffffffe000202000));

    // mapping other memory -|W|R|V
    for (uint64 i = ((uint64)&_sdata>>12)&0xffff; i < (1<<9)+ 0x200; i++) {
        create_mapping(swapper_pg_dir,0xffffffe000000000+(i<<12), 0x0000000080000000+(i<<12), 1 << 12, 7);
    }
    for (uint64 i = 2; i < 64; i++) {
        create_mapping(swapper_pg_dir,0xffffffe000000000+(i<<21), 0x0000000080000000+(i<<21), 1 << 21, 7);
    }
    register uint64 t0 asm("t0");
    t0 = ((uint64)8<<60)+(((uint64)swapper_pg_dir-PA2VA_OFFSET)>>12);
    asm volatile("csrw satp, t0");
    // set satp with swapper_pg_dir

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
    
}


/* 创建多级页表映射关系 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小
    perm 为映射的读写权限

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    uint64 v2 = (va >> 30)&511;
    uint64 v1 = (va >> 21)&511;
    uint64 v0 = (va >> 12)&511;
    unsigned long * p_now;
    p_now = pgtbl;
    if (sz == 1 << 12) {
        if ((pa&4095) != 0) {
        } else {
            if ((p_now[v2]&1) == 0) {
                unsigned long *p = kalloc();
                memset(p, 0x0, PGSIZE);
                p_now[v2] = (((uint64)p-PA2VA_OFFSET)>>2)+1;
            }
            p_now = PA2VA_OFFSET+(uint64)(((p_now[v2])>>10)<<12);
            if ((p_now[v1]&1) == 0) {
                unsigned long *p = kalloc();
                memset(p, 0x0, PGSIZE);
                p_now[v1] = (((uint64)p-PA2VA_OFFSET)>>2)+1;
            }
            p_now = PA2VA_OFFSET+(uint64)(((p_now[v1])>>10)<<12);
            p_now[v0] = (pa>>2) + perm;
        }
    } else if (sz == 1 << 21) {
        if ((pa & ((1 << 21)-1)) != 0) {
        } else {
            if ((p_now[v2]&1) == 0) {   
                unsigned long *p = kalloc();
                memset(p, 0x0, PGSIZE);
                p_now[v2] = (((uint64)p-PA2VA_OFFSET)>>2)+1;
            }
            p_now = (uint64)PA2VA_OFFSET+(uint64)(((p_now[v2])>>10)<<12);
            if ((p_now[v1]&1) == 0) {
                unsigned long *p = kalloc();
                memset(p, 0x0, PGSIZE);
                p_now[v1] = (((uint64)p-PA2VA_OFFSET)>>2)+1;
            }
            p_now = (uint64)PA2VA_OFFSET+(uint64)(((p_now[v1])>>10)<<12);
            for (uint64 i = 0; i < (1<<9); i++) {
                p_now[i] = (pa>>2)+ (i << 10)+perm;
            }
        }
    }
    return;
}


void copy_page_table(uint64* pre_table, uint64* new_table) {
    uint64* t;


    for (int i = 0; i < 512; i++) {
        if ((pre_table[i]&1) != 0) {
            if ((new_table[i]&1) == 0) {
                t = alloc_page();
                new_table[i] = (((uint64)t-PA2VA_OFFSET)>>2)+1;
            }
            // printk("%d\n", (new_table[i]&1)==0);
            uint64* p1;
            uint64* n1;
            p1 = (uint64)PA2VA_OFFSET+(uint64)(((pre_table[i])>>10)<<12);
            n1 = (uint64)PA2VA_OFFSET+(uint64)(((new_table[i])>>10)<<12);
                    // printk("t:%x\n", (((uint64)t-PA2VA_OFFSET)>>2)+1);
                    // printk("new_table[i]:%x\n", new_table[i]);

            
            for (int j = 0; j < 512; j++) {
                // printk("%x\n", p1[j]);
                if (p1[j]&1 != 0) {
                    if ((n1[j]&1) == 0) {
                        t = alloc_page();
                        n1[j] = (((uint64)t-PA2VA_OFFSET)>>2)+1;
                    }
                    uint64* p0;
                    uint64* n0;
                    // printk("1\n");
                    // printk("%x\n", (uint64)PA2VA_OFFSET+(uint64)(((p1[j])>>10)<<12));
                    // printk("%x\n", n1);
                    p0 = (uint64)PA2VA_OFFSET+(uint64)(((p1[j])>>10)<<12);
                    n0 = (uint64)PA2VA_OFFSET+(uint64)(((n1[j])>>10)<<12);
                    for (int k = 0; k < 512; k++) {
                        if (p0[k]&1 != 0) {
                            n0[k] = p0[k];
                        }
                    }
                }
            }
        }
    }
    return;
}