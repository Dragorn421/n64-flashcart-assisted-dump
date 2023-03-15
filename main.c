#include <libdragon.h>

#define C0_WRITE_WATCHLO(x) ({            \
    asm volatile("mtc0 %0,$18" ::"r"(x)); \
})
#define C0_WRITE_WATCHHI(x) ({            \
    asm volatile("mtc0 %0,$19" ::"r"(x)); \
})

volatile int timer_i = 0;

void timer_callback(int ovfl)
{
    timer_i++;
    printf("timer_callback: timer_i = %d\n", timer_i);
}

void my_reset_handler(void)
{
    printf("> my_reset_handler\n");

    C0_WRITE_WATCHLO(PhysicalAddr(SP_STATUS) | 3);
    C0_WRITE_WATCHHI(0);

    printf("< my_reset_handler\n");
}

void return_from_watch_exception(void);

void my_exception_handler(exception_t *ex)
{
    printf("> my_exception_handler\n");

    if (ex->code != EXCEPTION_CODE_WATCH)
    {
        exception_default_handler(ex);
        return;
    }

    printf("Watch exception @ 0x%08lX\n", ex->regs->epc);

    // (idk if this is safe wrt sp and possibly other things,
    // so don't have return_from_watch_exception do much)
    ex->regs->epc = (uint32_t)return_from_watch_exception;

    C0_WRITE_WATCHLO(0);
    C0_WRITE_WATCHHI(0);

    printf("< my_exception_handler\n");
}

void return_from_watch_exception(void)
{
    // not sure it's safe to put much code in here, see function usage
    printf("> return_from_watch_exception\n");
    while (1)
        ;
}

int main(void)
{
    console_init();
    console_set_render_mode(RENDER_AUTOMATIC);
    timer_init();

    C0_WRITE_WATCHLO(0);
    C0_WRITE_WATCHHI(0);

    new_timer(TICKS_FROM_MS(500), TF_CONTINUOUS, timer_callback);

    register_reset_handler(my_reset_handler);
    register_exception_handler(my_exception_handler);

    while (1)
        ;
}
