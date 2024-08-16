#include "print.h"
#include "sbi.h"

void puts(char *s) {
    // unimplemented
    while (*s) {
        sbi_ecall(1, 0, *s++, 0, 0, 0, 0, 0);
    }
}

void puti(int x) {
    // unimplemented
    int k = 0;
    while (x) {
        k = k*10 + x%10;
        x /= 10;
    }
    while (k) {
        sbi_ecall(1, 0, k%10+'0', 0, 0, 0, 0, 0);
        k /= 10;
    }
}
