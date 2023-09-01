#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include <stdbool.h>
#include <stdint.h>

// start from libdragon/src/regs.S
/* Standard Status Register bitmasks: */
#define SR_CU1 0x20000000 /* Mark CP1 as usable */
#define SR_FR 0x04000000  /* Enable MIPS III FP registers */
#define SR_BEV 0x00400000 /* Controls location of exception vectors */
#define SR_PE 0x00100000  /* Mark soft reset (clear parity error) */

#define SR_KX 0x00000080 /* Kernel extended addressing enabled */
#define SR_SX 0x00000040 /* Supervisor extended addressing enabled */
#define SR_UX 0x00000020 /* User extended addressing enabled */

#define SR_ERL 0x00000004 /* Error level */
#define SR_EXL 0x00000002 /* Exception level */
#define SR_IE 0x00000001  /* Interrupts enabled */
// end

#define C0_WRITE_WATCHLO(x) ({            \
    asm volatile("mtc0 %0,$18" ::"r"(x)); \
})
#define C0_WRITE_WATCHHI(x) ({            \
    asm volatile("mtc0 %0,$19" ::"r"(x)); \
})

struct sprafp
{
    uint32_t sp, ra, fp;
};
struct sprafp get_sprafp(void);

void set_cpu_watch_regs(uint32_t physical_addr, bool exception_on_load, bool exception_on_write);

#endif
