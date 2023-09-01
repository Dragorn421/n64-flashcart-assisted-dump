#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>

#include <libdragon.h>

#include "qrencode/qrencode.h"

#include "cpu_utils.h"

void watch_ipl1_set(void)
{
    // Set the cpu watch registers to trigger an exception on reading SP_STATUS,
    // to interrupt IPL1
    set_cpu_watch_regs(PhysicalAddr(SP_STATUS), true, false);
}

void watch_ipl1_clear(void)
{
    // Clear the watch regs
    set_cpu_watch_regs(0, false, false);
}

void my_exception_handler(exception_t *ex);

void my_reset_handler(void)
{
    printf("> my_reset_handler\n");

    exception_handler_t old_exception_handler = register_exception_handler(my_exception_handler);
    assert(old_exception_handler == exception_default_handler);
    watch_ipl1_set();

    printf("< my_reset_handler\n");
}

void return_from_watch_exception(void);

void my_exception_handler(exception_t *ex)
{
    // We really only want to clear the watch regs once we interrupted IPL1,
    // but we clear them early here (and set again later if needed), to
    // avoid leaving the watch regs set in case something goes wrong
    // (e.g. the cpu breaks), because then the watch regs persist for
    // a while (?) after powering the console off, and it prevents booting
    // until the regs lose their value.
    watch_ipl1_clear();

    printf("> my_exception_handler\n");

    if (ex->code != EXCEPTION_CODE_WATCH)
    {
        exception_default_handler(ex);

        watch_ipl1_set();
        return;
    }

    /*
    The epc is expected to be in PIF rom (physical address 0x1FC00000-0x1FC007BF, see https://n64brew.dev/wiki/Memory_map)

    For all regions the exception should happen at IPL1+0x18:
        ...
        14:	3c08a404 	lui	t0,0xa404
        18:	8d080010 	lw	t0,0x10(t0)  <- reading 0xa4040010 = SP_STATUS

    And the cpu is running from uncached memory at this point,
    so the expected epc is 0xA0000000 | 0x1FC00000 + 0x18 = 0xBFC00018
    */
    printf("Watch exception @ 0x%08lX\n", ex->regs->epc);

    if (ex->regs->epc != 0xBFC00018)
    {
        printf("Ignored watch exception, not in pif rom\n");

        watch_ipl1_set();
        return;
    }

    // Unregister our exception handler
    register_exception_handler(exception_default_handler);

    // Resume from the exception in our code instead of back in pif rom.
    // This makes return_from_watch_exception the "main thread" code,
    // basically where the pc is outside of exception handling,
    // instead of the main function before the reset.
    // It is ok to continue execution there because memory was untouched
    // through the reset, as well as the stack pointer which should
    // still be high enough for all stack usage.
    // (the bit of IPL1 that runs before being interrupted doesn't touch
    //  memory nor the stack pointer)
    ex->regs->epc = (uint32_t)return_from_watch_exception;

    printf("my_exception_handler C0_STATUS() = 0x%08lX\n", C0_STATUS());
    printf("my_exception_handler ex->regs->sr = 0x%08lX\n", ex->regs->sr);

    // IPL1 sets sr to SR_CU0 | SR_CU1 | SR_FR,
    // disabling interrupts and other stuff (?).
    // Have the interrupt system set sr to this more useful state instead
    // when resuming from the exception.
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

int main_dumping(void);

void return_from_watch_exception(void)
{
    // This function must not return, it is at the "top" of execution,
    // see where it's used in our exception handler. So exit()
    exit(main_dumping());
}

void flashcart_unlock_sc64_regs(void)
{
    // from libdragon's usb.c

#define SC64_REGS_BASE 0x1FFF0000

#define SC64_REG_KEY (SC64_REGS_BASE + 0x10)

#define SC64_KEY_RESET 0x00000000
#define SC64_KEY_UNLOCK_1 0x5F554E4C
#define SC64_KEY_UNLOCK_2 0x4F434B5F
    io_write(SC64_REG_KEY, SC64_KEY_RESET);
    io_write(SC64_REG_KEY, SC64_KEY_UNLOCK_1);
    io_write(SC64_REG_KEY, SC64_KEY_UNLOCK_2);
}

int main_dumping(void)
{
    /*
     * On NMI the SummerCart64 locks its registers, preventing usb communication.
     * Unlock the regs again to keep communicating.
     * Note: we may want to do this earlier, like near the start of my_exception_handler
     */
    //! TODO what about not-sc64 flashcarts
    flashcart_unlock_sc64_regs();

    printf("> return_from_watch_exception\n");

    struct sprafp sprafp = get_sprafp();
    printf("rfwe  sp %08lX  ra %08lX  fp%08lX\n", sprafp.sp, sprafp.ra, sprafp.fp);

    console_set_render_mode(RENDER_MANUAL);

    bool show_console = true;

    bool auto_advance = false;

    // A qr code's "version" denotes its size
    // The version, width and max_bytes here must be kept in sync
    // See https://www.qrcode.com/en/about/version.html
// test results are "at least with my capture card" (which is bad)
// a better cpature card may do better
// a camera filming a screen with the n64 output may do worse (or better if the analog signal is processed very well?)
#if 0 // works
    const int qrcode_version = 15;
    const int qrcode_width = 77;
    const int qrcode_max_bytes = 220;
    const QRecLevel qrcode_ecc_level = QR_ECLEVEL_H;

    const int module_height = 3;
#endif
#if 0 // doesn't work
    const int qrcode_version = 40;
    const int qrcode_width = 177;
    const int qrcode_max_bytes = 1273;
    const QRecLevel qrcode_ecc_level = QR_ECLEVEL_H;

    const int module_height = 1;
#endif
#if 0 // works (with zbar, but not with many js decoders, probably because there is not enough white padding)
    const int qrcode_version = 25;
    const int qrcode_width = 117;
    const int qrcode_max_bytes = 535;
    const QRecLevel qrcode_ecc_level = QR_ECLEVEL_H;

    const int module_height = 2;
#endif
#if 1 // works
    const int qrcode_version = 24;
    const int qrcode_width = 113;
    const int qrcode_max_bytes = 511;
    const QRecLevel qrcode_ecc_level = QR_ECLEVEL_H;

    const int module_height = 2;
#endif
#if 0 // untested
    const int qrcode_version = 15;
    const int qrcode_width = 77;
    const int qrcode_max_bytes = 520;
    const QRecLevel qrcode_ecc_level = QR_ECLEVEL_L;
#endif

    const float analog_aspect_ratio = 4.0f / 3.0f;
    const float framebuffer_aspect_ratio = (float)display_get_width() / display_get_height();

    const int module_width = (int)roundf(module_height * (framebuffer_aspect_ratio / analog_aspect_ratio));

    const int offset_x = (int)roundf(display_get_width() / 2.0f - qrcode_width * module_width / 2.0f);
    const int offset_y = (int)roundf(display_get_height() / 2.0f - qrcode_width * module_height / 2.0f);
    assert(offset_x >= 0);
    assert(offset_y >= 0);

    printf("module size: %dx%d\n", module_width, module_height);
    printf("offset: ( %d , %d )\n", offset_x, offset_y);

    uint32_t color_black = graphics_make_color(0, 0, 0, 255);
    uint32_t color_white = graphics_make_color(255, 255, 255, 255);

    const int n_header_bytes_per_frame = 5;

    // 2-align for dma (rounding down to respect qrcode_max_bytes)
    const int nrombytes_per_frame = (qrcode_max_bytes - n_header_bytes_per_frame) / 2 * 2;

    int rom_offset = 0; // must stay a multiple of 2 for dma

    uint8_t *rombuf = malloc_uncached_aligned(8, nrombytes_per_frame);
    data_cache_hit_writeback_invalidate(rombuf, nrombytes_per_frame);

    printf("nrombytes_per_frame = %d\n", nrombytes_per_frame);

    while (1)
    {
        controller_scan();

        struct controller_data keys_down = get_keys_down();

        if (keys_down.c[0].A)
        {
            show_console = !show_console;
            printf("A\n");

            surface_t *surf = display_get();
            graphics_fill_screen(surf, graphics_make_color(0, 0, 0, 255));
            display_show(surf);

            continue;
        }

        if (keys_down.c[0].B)
        {
            auto_advance = !auto_advance;
            printf("B\n");
        }

        if (keys_down.c[0].up)
        {
            rom_offset -= 4;
            if (rom_offset < 0)
                rom_offset = 0;
            printf("rom_offset = 0x%X\n", rom_offset);
        }
        if (keys_down.c[0].down)
        {
            rom_offset += 4;
            printf("rom_offset = 0x%X\n", rom_offset);
        }

        if (show_console)
        {
            console_render();
        }
        else
        {
            // dma while waiting for a framebuffer
            dma_read_async(rombuf, 0x10000000 + rom_offset, nrombytes_per_frame);
            assert(nrombytes_per_frame >= 8);

            surface_t *surf = display_get();
            graphics_fill_screen(surf, graphics_make_color(255, 255, 255, 255));

            dma_wait();

            for (int i = 0; i < nrombytes_per_frame; i++)
            {
                printf("%02X", rombuf[i]);
                if ((i % 16) == 15)
                    printf("\n");
            }
            printf("\n");

            int qrdatalen = n_header_bytes_per_frame + nrombytes_per_frame;
            uint8_t qrdata[qrdatalen];

            static uint8_t counter = 0;
            qrdata[0] = counter++;

            qrdata[1] = (rom_offset >> 24) & 0xFF;
            qrdata[2] = (rom_offset >> 16) & 0xFF;
            qrdata[3] = (rom_offset >> 8) & 0xFF;
            qrdata[4] = (rom_offset >> 0) & 0xFF;
            _Static_assert(n_header_bytes_per_frame == 5);

            memcpy(qrdata + n_header_bytes_per_frame, rombuf, nrombytes_per_frame);

            QRcode *qr_code = QRcode_encodeData(qrdatalen, qrdata, qrcode_version, qrcode_ecc_level);
            if (qr_code == NULL)
            {
                int err = errno;

                EINVAL;
                ENOMEM;
                ERANGE;
                printf("main_dumping: QRcode_encodeData failed, errno = %d\n", err);

                errno = err;
                perror("main_dumping: call QRcode_encodeData");

                show_console = true;
            }
            else
            {
                assert(qr_code->width == qrcode_width);

                for (int x = 0; x < qr_code->width; x++)
                {
                    for (int y = 0; y < qr_code->width; y++)
                    {
                        unsigned char val = qr_code->data[x * qr_code->width + y];
                        int is_black = val & 1;
                        graphics_draw_box(surf,
                                          offset_x + x * module_width, offset_y + y * module_height,
                                          module_width, module_height,
                                          is_black ? color_black : color_white);
                    }
                }

                QRcode_free(qr_code);
                qr_code = NULL;
            }

            display_show(surf);

            wait_ms(1000 / 10);

            if (auto_advance)
            {
                // TODO may want configuration to advance by nrombytes_per_frame/2
                // or such, to have even more redundancy
                rom_offset += nrombytes_per_frame;
            }
        }
    }
}

void setup_initial_boot(void)
{
    debug_init_usblog();

    console_init();
    console_set_render_mode(RENDER_AUTOMATIC);

    controller_init();

    set_cpu_watch_regs(0, false, false);

    register_RESET_handler(my_reset_handler);

    printf("main C0_STATUS() = 0x%08lX\n", C0_STATUS());
}

int main_wait_reset(void)
{
    setup_initial_boot();

    struct sprafp sprafp = get_sprafp();
    printf("main  sp %08lX  ra %08lX  fp%08lX\n", sprafp.sp, sprafp.ra, sprafp.fp);

    // The pc is stuck in this loop outside of interrupt handling,
    // until reset (NMI) where the pc is set to run IPL1 in pif rom.
    while (1)
        ;
}

int main()
{
    return main_wait_reset();
}
