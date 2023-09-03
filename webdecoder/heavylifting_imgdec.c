#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <emscripten/emscripten.h>

#include "zbar/include/zbar.h"

#include "heavylifting.h"

/**
 * @param image_data_gray gray image, one byte per pixel, row major (data[height][width])
 * @param[out] decoded_data where to store the decoded data
 * @param decoded_data_buf_len how many bytes can be stored into decoded_data
 * @param[out] decoded_data_len where to store the length of the decoded data to
 * @return a result code
 */
enum decode_qr_code_image_result
{
    DEC_QR_OK = 0,
    DEC_QR_ERR_NO_CODE,
    DEC_QR_ERR_SEVERAL_CODES,
    DEC_QR_ERR_OUT_BUF_TOO_SHORT
} decode_qr_code_image(int width, int height, uint8_t *image_data_gray,
                       uint8_t *decoded_data, size_t decoded_data_buf_len, int *decoded_data_len)
{
    enum decode_qr_code_image_result result;
    int ret;

    // Initialize processor

    zbar_processor_t *processor = zbar_processor_create(0);
    assert(processor != NULL);

    // Only enable QR codes
    ret = zbar_processor_set_config(processor, 0, ZBAR_CFG_ENABLE, 0);
    assert(ret == 0);
    ret = zbar_processor_set_config(processor, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
    assert(ret == 0);
    // Keep binary data as binary, don't decode/encode as text
    ret = zbar_processor_set_config(processor, ZBAR_QRCODE, ZBAR_CFG_BINARY, 1);
    assert(ret == 0);

    // Initialize the zbar image

    zbar_image_t *image = zbar_image_create();
    assert(image != NULL);

    // One byte per pixel, gray scale
    zbar_image_set_format(image, zbar_fourcc('Y', '8', '0', '0'));

    zbar_image_set_size(image, width, height);

    int n_pixels = width * height;

    zbar_image_set_data(image, image_data_gray, n_pixels, NULL);

    // Find QR codes in the image

    ret = zbar_process_image(processor, image);
    assert(ret >= 0);

    const zbar_symbol_set_t *symbols = zbar_image_get_symbols(image);
    int nsyms = zbar_symbol_set_get_size(symbols);

    if (nsyms == 1)
    {
        // Found exactly one QR code

        const zbar_symbol_t *sym = zbar_symbol_set_first_symbol(symbols);
        assert(sym != NULL);

        zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
        assert(typ == ZBAR_QRCODE);

        unsigned len = zbar_symbol_get_data_length(sym);
        const char *data = zbar_symbol_get_data(sym);

        // Copy data to output buffer
        if (len > decoded_data_buf_len)
        {
            result = DEC_QR_ERR_OUT_BUF_TOO_SHORT;
        }
        else
        {
            result = DEC_QR_OK;

            memcpy(decoded_data, data, len);
            *decoded_data_len = len;
        }
    }
    else
    {
        if (nsyms == 0)
        {
            result = DEC_QR_ERR_NO_CODE;
        }
        else
        {
            result = DEC_QR_ERR_SEVERAL_CODES;
        }
    }

    // Cleanup

    zbar_image_destroy(image);

    zbar_processor_destroy(processor);

    return result;
}

uint8_t decoded_data_buf[10000];

int id = -1;

EMSCRIPTEN_KEEPALIVE
void set_id(char *data, int size)
{
    assert(size == sizeof(int));
    id = *(int *)data;
}

EMSCRIPTEN_KEEPALIVE
void heavylifting_decode_image(char *data, int size)
{
    if (HL_VERBOSE)
        printf("> heavylifting_decode_image %d\n", id);

    assert(size >= sizeof(struct image_data_header));

    struct image_data *imgd = (struct image_data *)data;
    int width = imgd->hdr.width;
    int height = imgd->hdr.height;

    assert(data + size >= (char *)imgd->rgba_data + 4 * width * height);

    uint8_t *image_data_gray = malloc(width * height);
    assert(image_data_gray != NULL);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int r = imgd->rgba_data[4 * (y * width + x) + 0];
            int g = imgd->rgba_data[4 * (y * width + x) + 1];
            int b = imgd->rgba_data[4 * (y * width + x) + 2];
            int gray = (r + g + b) / 3; // bad conversion but whatever
            assert(gray >= 0);
            assert(gray <= 255);
            image_data_gray[y * width + x] = gray;
        }
    }

    int decoded_data_len;

    enum decode_qr_code_image_result result = decode_qr_code_image(
        width, height, image_data_gray,
        decoded_data_buf, sizeof(decoded_data_buf), &decoded_data_len);

    free(image_data_gray);

    if (result != DEC_QR_OK)
    {
        emscripten_worker_respond((char *)&(struct imgdec_response_header){HL_FAIL}, 4);

        if (HL_VERBOSE)
            printf("< heavylifting_decode_image %d (no data)\n", id);
        return;
    }

    assert(decoded_data_len >= 5);

    uint8_t counter = decoded_data_buf[0];
    uint32_t offset =
        ((decoded_data_buf[1] << 24) |
         (decoded_data_buf[2] << 16) |
         (decoded_data_buf[3] << 8) |
         (decoded_data_buf[4] << 0));
    uint8_t *rom_bytes = decoded_data_buf + 5;
    int n_rom_bytes = decoded_data_len - 5;

    assert(12 == offsetof(struct imgdec_response, rom_fragment.bytes)); // TODO remove when sure
    int response_size = offsetof(struct imgdec_response, rom_fragment.bytes) + n_rom_bytes;
    struct imgdec_response *response = malloc(response_size);

    response->hdr.result_code = HL_SUCCESS;
    response->rom_fragment.offset = offset;
    response->rom_fragment.n_bytes = n_rom_bytes;
    memcpy(&response->rom_fragment.bytes, rom_bytes, n_rom_bytes);

    char *response_data = (char *)response;
    emscripten_worker_respond(response_data, response_size);

    free(response);

    if (HL_VERBOSE)
        printf("< heavylifting_decode_image %d (success)\n", id);
}
