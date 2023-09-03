#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include <emscripten/emscripten.h>

#include "heavylifting.h"

bool is_initialized = false;

#define N_IMGDEC_WORKERS 4

worker_handle imgdec_workers[N_IMGDEC_WORKERS];

#define ROM_BYTE_UNKNOWN -1

int max_rom_size;
int16_t *rom_data;
// all rom_data[i] is ROM_BYTE_UNKNOWN outside rom_data_not_unknown_min <= i < rom_data_not_unknown_max
int rom_data_not_unknown_min;
int rom_data_not_unknown_max;

EMSCRIPTEN_KEEPALIVE
int heavylifting_init(int max_rom_size_)
{
    assert(!is_initialized);
    is_initialized = true;

    max_rom_size = max_rom_size_;
    rom_data = malloc(sizeof(int16_t[max_rom_size]));
    assert(rom_data != NULL);

    rom_data_not_unknown_min = max_rom_size;
    rom_data_not_unknown_max = 0;

    for (int i = 0; i < max_rom_size; i++)
    {
        rom_data[i] = ROM_BYTE_UNKNOWN;
    }

    for (int i = 0; i < N_IMGDEC_WORKERS; i++)
    {
        imgdec_workers[i] = emscripten_create_worker("./heavylifting_imgdec.js");
        emscripten_call_worker(imgdec_workers[i],
                               "set_id",
                               (char *)&i, sizeof(int),
                               NULL, NULL);
    }

    return HL_SUCCESS;
}

void store_rom_fragment(struct imgdec_response_rom_fragment *rom_frag)
{
    assert(rom_frag->n_bytes > 0);
    assert(rom_frag->offset + rom_frag->n_bytes <= max_rom_size);

    if (HL_VERBOSE)
        printf("store_rom_fragment: rom_frag->offset = %d\n", rom_frag->offset);

    if (rom_frag->offset < rom_data_not_unknown_min)
    {
        rom_data_not_unknown_min = rom_frag->offset;
    }
    if (rom_data_not_unknown_max < rom_frag->offset + rom_frag->n_bytes)
    {
        rom_data_not_unknown_max = rom_frag->offset + rom_frag->n_bytes;
    }

    for (int i = 0; i < rom_frag->n_bytes; i++)
    {
        if (rom_data[rom_frag->offset + i] != ROM_BYTE_UNKNOWN)
        {
            if (rom_data[rom_frag->offset + i] != rom_frag->bytes[i])
            {
                // TODO
                fprintf(stderr, "mismatching byte at %08X : current %02X != incoming %02X\n",
                        rom_frag->offset + i,
                        (uint8_t)rom_data[rom_frag->offset + i],
                        rom_frag->bytes[i]);
            }
        }
        rom_data[rom_frag->offset + i] = rom_frag->bytes[i];
    }
}

void decode_image_callback(char *data, int size, void *arg)
{
    assert(size >= sizeof(struct imgdec_response_header));
    struct imgdec_response *idr = (struct imgdec_response *)data;

    if (idr->hdr.result_code != HL_SUCCESS)
    {
        // TODO
        if (HL_VERBOSE)
            fprintf(stderr, "idr->hdr.result_code != HL_SUCCESS\n");
        return;
    }

    store_rom_fragment(&idr->rom_fragment);
}

EMSCRIPTEN_KEEPALIVE
int heavylifting_process_frame(int width, int height, void *image_data_rgba)
{
    assert(is_initialized);

    int imgd_memsize = offsetof(struct image_data, rgba_data) + 4 * width * height;
    struct image_data *imgd = malloc(imgd_memsize);
    assert(imgd != NULL);

    imgd->hdr.width = width;
    imgd->hdr.height = height;
    memcpy(&imgd->rgba_data, image_data_rgba, 4 * width * height);

    // Find worker with smallest backlog
    int i_worker_least_busy = -1;
    int queue_size_least_busy;
    for (int i = 0; i < N_IMGDEC_WORKERS; i++)
    {
        worker_handle worker = imgdec_workers[i];
        int queue_size = emscripten_get_worker_queue_size(worker);

        if (i_worker_least_busy == -1 || queue_size < queue_size_least_busy)
        {
            i_worker_least_busy = i;
            queue_size_least_busy = queue_size;
        }
    }

    if (HL_VERBOSE)
        printf("heavylifting_process_frame: i_worker_least_busy = %d\n", i_worker_least_busy);

    void *callback_arg = NULL; // not sure what to use this for yet
    emscripten_call_worker(imgdec_workers[i_worker_least_busy],
                           "heavylifting_decode_image",
                           (char *)imgd, imgd_memsize,
                           decode_image_callback, callback_arg);

    free(imgd);

    return HL_SUCCESS;
}

EMSCRIPTEN_KEEPALIVE
int16_t *heavylifting_get_rom_data(void)
{
    return rom_data;
}

EMSCRIPTEN_KEEPALIVE
int heavylifting_get_rom_data_not_unknown_min(void)
{
    return rom_data_not_unknown_min;
}

EMSCRIPTEN_KEEPALIVE
int heavylifting_get_rom_data_not_unknown_max(void)
{
    return rom_data_not_unknown_max;
}
