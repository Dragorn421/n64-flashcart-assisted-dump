#ifndef HEAVYLIFTING_H
#define HEAVYLIFTING_H

#define HL_VERBOSE 0

#define HL_SUCCESS 64
#define HL_FAIL -1

struct image_data_header
{
    int width;
    int height;
};

struct image_data
{
    struct image_data_header hdr;
    unsigned char rgba_data[1]; // length 4*width*height
};

struct imgdec_response_header
{
    int result_code;
};

struct imgdec_response_rom_fragment
{
    uint32_t offset;
    int n_bytes;
    unsigned char bytes[1]; // length n_bytes
};

struct imgdec_response
{
    struct imgdec_response_header hdr;
    struct imgdec_response_rom_fragment rom_fragment;
};

#endif
