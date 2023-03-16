#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <strings.h>

#include "../quirc/lib/quirc.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define N_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)

int main()
{
    FILE *debug_buf_out = fopen("debug_buf_out.bin", "wb");
    if (debug_buf_out == NULL)
    {
        perror("Failed to open debug_buf_out.bin");
        abort();
    }

    struct quirc *qr;

    qr = quirc_new();
    if (!qr)
    {
        perror("Failed to allocate memory");
        abort();
    }

    if (quirc_resize(qr, SCREEN_WIDTH, SCREEN_HEIGHT) < 0)
    {
        perror("Failed to allocate video memory");
        abort();
    }

    while (true)
    {

        {
            uint8_t *image;
            int w, h;

            image = quirc_begin(qr, &w, &h);

            bzero(image, N_PIXELS);

            // ffmpeg -pix_fmt gray
            // so 1 byte per pixel
            size_t n_read = fread(image, 1, N_PIXELS, stdin);

            fseek(debug_buf_out, 0, SEEK_SET);
            fwrite(image, 1, N_PIXELS, debug_buf_out);

            if (n_read != N_PIXELS)
            {
                fprintf(stderr, "Read only n_read=%ld / N_PIXELS=%d bytes, stopping\n", n_read, N_PIXELS);

                quirc_destroy(qr);

                return EXIT_FAILURE;
            }

            quirc_end(qr);
        }

        {
            int num_codes;
            int i;

            num_codes = quirc_count(qr);

            if (num_codes > 1)
            {
                fprintf(stderr, "Found num_codes=%d (> 1) codes!\n", num_codes);

                quirc_destroy(qr);

                return EXIT_FAILURE;
            }

            fprintf(stderr, "Found num_codes=%d codes\n", num_codes);

            for (i = 0; i < num_codes; i++)
            {
                struct quirc_code code;
                struct quirc_data data;
                quirc_decode_error_t err;

                quirc_extract(qr, i, &code);

                /* Decoding stage */
                err = quirc_decode(&code, &data);
                if (err)
                    fprintf(stderr, "DECODE FAILED: %s\n", quirc_strerror(err));
                else
                {
                    fprintf(stderr, "Data: %s\n", data.payload);
                }
            }
        }
    }
}
