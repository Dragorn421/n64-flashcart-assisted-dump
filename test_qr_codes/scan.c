#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "../quirc/lib/quirc.h"

enum
{
    ERR_NONE = 0,
    ERR_MEM1,
    ERR_MEM2,
    ERR_BUF_TOO_SMALL,
    ERR_NO_CODE,
    ERR_SEVERAL_CODES,
    ERR_DECODE_FAILED
};

int32_t scan(const uint8_t *image_buf, int32_t width, int32_t height, char *out_payload_buf, size_t out_payload_buf_size)
{
#define PRINT_CODE 0
    if (PRINT_CODE)
    {
        printf("> scan.c scan\n");
        for (int y = 0; y < width; y++)
        {
            for (int x = 0; x < height; x++)
            {
                if (image_buf[x + width * y])
                {
                    printf("  ");
                }
                else
                {
                    printf("##");
                }
            }
            printf("\n");
        }
    }

    struct quirc *qr;

    qr = quirc_new();
    if (!qr)
    {
        perror("Failed to allocate memory");
        return ERR_MEM1;
    }

    if (quirc_resize(qr, width, height) < 0)
    {
        perror("Failed to allocate video memory");
        return ERR_MEM2;
    }

    uint8_t *image;
    int w, h;

    image = quirc_begin(qr, &w, &h);

    /* Fill out the image buffer here.
     * image is a pointer to a w*h bytes.
     * One byte per pixel, w pixels per line, h lines in the buffer.
     */
    memcpy(image, image_buf, w * h);

    quirc_end(qr);

    int num_codes;
    int i;

    num_codes = quirc_count(qr);

    if (num_codes == 0)
    {
        return ERR_NO_CODE;
    }
    else if (num_codes >= 2)
    {
        return ERR_SEVERAL_CODES;
    }

    for (i = 0; i < num_codes; i++)
    {
        struct quirc_code code;
        struct quirc_data data;
        quirc_decode_error_t err;

        quirc_extract(qr, i, &code);

        /* Decoding stage */
        err = quirc_decode(&code, &data);
        if (err)
        {
            printf("DECODE FAILED: %s\n", quirc_strerror(err));
            return ERR_DECODE_FAILED;
        }
        else
        {
            printf("Data: %s\n", data.payload);

            if (strlen(data.payload) + 1 <= out_payload_buf_size)
            {
                strcpy(out_payload_buf, data.payload);
            }
            else
            {
                return ERR_BUF_TOO_SMALL;
            }
        }
    }

    quirc_destroy(qr);

    return ERR_NONE;
}