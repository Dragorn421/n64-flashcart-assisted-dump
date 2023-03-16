#include <libdragon.h>
#include <string.h>

#include "QRCode/src/qrcode.h"

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

volatile int timer_i = 0;

uint8_t prev_buf[0x40] = {0};

void timer_callback(int ovfl)
{
    timer_i++;
    printf("timer_callback: timer_i = %d\n", timer_i);

    uint8_t buf[0x40];
    dma_read(buf, 0x10000000, 0x40);
    if (memcmp(prev_buf, buf, 0x40))
    {
        printf("timer_callback: new data!\n");
        memcpy(prev_buf, buf, 0x40);

        int off = 0;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 16; j++)
            {
                printf(" %02X", buf[off]);
                off++;
            }
            printf("\n");
        }
    }

    // TODO should not be called while interrupts are disabled, yet called from a timer which is in a interrupt handler?
    if (timer_i % 3 != 0)
    {
        console_render();
    }
    else
    {

        uint8_t buf_stringified[sizeof(buf) * 2 + 1] = {0};

        {
            char *buf_stringified_p = (char *)&buf_stringified[0];

            for (int i = 0; i < sizeof(prev_buf); i++)
            {
                buf_stringified_p += sprintf(buf_stringified_p, "%02X", prev_buf[i]);
            }

            printf("%d \"%s\"\n", sizeof(buf_stringified), (char *)buf_stringified);
        }

        surface_t *surf = NULL;
        while (!(surf = display_lock()))
            ;
        {
            graphics_fill_screen(surf, graphics_make_color(255, 0, 255, 255));

            // The structure to manage the QR code
            QRCode qrcode;

            {
                // Allocate a chunk of memory to store the QR code
                uint8_t qrcodeBytes[qrcode_getBufferSize(7)];

                qrcode_initBytes(&qrcode, qrcodeBytes, 7, ECC_LOW, buf_stringified, sizeof(buf_stringified));
            }

            {
                int offX = 10, offY = 10;
                int elemW = 10, elemH = 5;

                for (int y = 0; y < qrcode.size; y++)
                {
                    for (int x = 0; x < qrcode.size; x++)
                    {
                        uint32_t color;
                        if (qrcode_getModule(&qrcode, x, y))
                        {
                            color = graphics_make_color(0, 0, 0, 255);
                        }
                        else
                        {
                            color = graphics_make_color(255, 255, 255, 255);
                        }
                        graphics_draw_box(surf, offX + x * elemW, offY + y * elemH, elemW, elemH, color);
                    }
                }
            }
        }
        display_show(surf);
    }
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

    printf("my_exception_handler C0_STATUS() = 0x%08lX\n", C0_STATUS());
    printf("my_exception_handler ex->regs->sr = 0x%08lX\n", ex->regs->sr);

    // IPL sets sr to something that disables timer interrupts
    ex->regs->sr =
        // from libdragon/src/entrypoint.S
        SR_CU1 | SR_PE | SR_FR | SR_KX | SR_SX | SR_UX
        // from set_TI_interrupt
        | C0_INTERRUPT_TIMER
        // from set_CART_interrupt
        | C0_INTERRUPT_CART // ?
        // from set_RESET_interrupt
        | C0_INTERRUPT_PRENMI // ?
        // from __init_interrupts
        | C0_STATUS_IE | C0_INTERRUPT_RCP | C0_INTERRUPT_PRENMI
        //
        ;

    printf("my_exception_handler ex->regs->sr := 0x%08lX\n", ex->regs->sr);

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
    console_set_render_mode(RENDER_MANUAL);
    timer_init();

    C0_WRITE_WATCHLO(0);
    C0_WRITE_WATCHHI(0);

    new_timer(TICKS_FROM_MS(2000), TF_CONTINUOUS, timer_callback);

    register_RESET_handler(my_reset_handler);
    register_exception_handler(my_exception_handler);

    printf("main C0_STATUS() = 0x%08lX\n", C0_STATUS());

    while (1)
        ;
}
