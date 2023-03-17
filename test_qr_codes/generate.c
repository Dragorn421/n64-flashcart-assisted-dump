#include <stdlib.h>
#include <stdint.h>

#include "../QRCode/src/qrcode.h"

enum
{
    ERR_NONE = 0,
    ERR_BUF_TOO_SMALL
};

int32_t generate(char *data, uint8_t *buf, size_t buf_size, int32_t *width, int32_t *height)
{
    // The structure to manage the QR code
    QRCode qrcode;

#define VERSION 3

    // Allocate a chunk of memory to store the QR code
    uint8_t qrcodeBytes[qrcode_getBufferSize(VERSION)];

    qrcode_initText(&qrcode, qrcodeBytes, VERSION, ECC_LOW, data);

    if (buf_size < qrcode.size * qrcode.size)
    {
        return ERR_BUF_TOO_SMALL;
    }

    uint8_t *bufp = buf;

    for (int y = 0; y < qrcode.size; y++)
    {
        for (int x = 0; x < qrcode.size; x++)
        {
            if (qrcode_getModule(&qrcode, x, y))
            {
                *bufp = 1;
            }
            else
            {
                *bufp = 0;
            }
            bufp++;
        }
    }

    *width = *height = qrcode.size;

    return ERR_NONE;
}
