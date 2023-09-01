#include "cpu_utils.h"

struct sprafp get_sprafp(void)
{
    // from libdragon backtrace_foreach
    uint32_t sp, ra, fp;
    asm volatile(
        "move %0, $ra\n"
        "move %1, $sp\n"
        "move %2, $fp\n"
        : "=r"(ra), "=r"(sp), "=r"(fp));
    return (struct sprafp){sp, ra, fp};
}

void set_cpu_watch_regs(uint32_t physical_addr, bool exception_on_load, bool exception_on_write){
    // Following the names from the vr4300 manual,
    // section "6.3.8 WatchLo (18) and WatchHi (19) Registers"
    uint32_t PAddr1 = 0;
    uint32_t PAddr0 = physical_addr & 0xFFFFFFF8;
    uint32_t r = exception_on_load ? (1 << 1) : 0;
    uint32_t w = exception_on_write ? (1 << 0) : 0;
    C0_WRITE_WATCHLO(PAddr0 | r | w);
    C0_WRITE_WATCHHI(PAddr1);
}
