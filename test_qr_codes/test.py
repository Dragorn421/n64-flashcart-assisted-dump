from pathlib import Path
import ctypes

ROOT = Path(__file__).parent


def do_generate(payload: bytes):

    generate = ctypes.CDLL(ROOT / "generate.so")
    # int32_t generate(char *data, uint8_t *buf, size_t buf_size, int32_t *width, int32_t *height)
    generate.generate.restype = ctypes.c_int32
    generate.generate.argtypes = [
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_int32),
        ctypes.POINTER(ctypes.c_int32),
    ]

    buf = (ctypes.c_uint8 * (1024**2))()
    width = ctypes.c_int32()
    height = ctypes.c_int32()
    generate_res = generate.generate(
        ctypes.create_string_buffer(payload),
        buf,
        len(buf),
        ctypes.byref(width),
        ctypes.byref(height),
    )
    print("generate_res =", generate_res)
    assert generate_res == 0

    return bytes(buf[: width.value * height.value]), width.value, height.value


def do_scan(
    image_buf: bytes,  # grayscale, one byte per pixel
    width: int,
    height: int,
):
    scan = ctypes.CDLL(ROOT / "scan.so")
    # int32_t scan(const uint8_t *image_buf, int32_t width, int32_t height, char *out_payload_buf, size_t out_payload_buf_size)
    scan.scan.restype = ctypes.c_int32
    scan.scan.argtypes = [
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_char_p,
        ctypes.c_size_t,
    ]

    out_payload_buf = ctypes.create_string_buffer(1024**2)
    scan_res = scan.scan(
        (ctypes.c_uint8 * len(image_buf))(*image_buf),
        width,
        height,
        out_payload_buf,
        len(out_payload_buf),
    )
    print("scan_res =", scan_res)
    assert scan_res == 0

    return bytes(out_payload_buf[: out_payload_buf[:].index(0)])


def main():

    #
    # generate
    #

    gen_buf, gen_width, gen_height = do_generate(b"hello world")
    print(gen_width)
    print(gen_height)
    print(gen_buf[:100])

    for y in range(gen_height):
        for x in range(gen_width):
            print("##" if gen_buf[y * gen_width + x] else "  ", end="")
        print("\n", end="")

    #
    # process/dump
    #

    # "true" is black
    image_buf = bytes(0 if b else 0xFF for b in gen_buf[: gen_width * gen_height])
    image_width = gen_width
    image_height = gen_height

    def enlarge(image_buf, image_width, image_height, *, n_pad=0, int_scale=1):
        lines = [
            image_buf[i : i + gen_width] for i in range(0, len(image_buf), gen_width)
        ]
        enlarged_image_width = int_scale * image_width + 2 * n_pad
        enlarged_image_height = int_scale * image_height + 2 * n_pad

        pad_updown = b"\xFF" * enlarged_image_width
        pad_leftright = b"\xFF" * n_pad

        enlarged_lines = []
        enlarged_lines += [pad_updown] * n_pad
        for line in lines:
            enlarged_line = b""
            enlarged_line += pad_leftright
            for b in line:
                enlarged_line += bytes([b] * int_scale)
            enlarged_line += pad_leftright
            enlarged_lines.extend([enlarged_line] * int_scale)
        enlarged_lines += [pad_updown] * n_pad

        enlarged_image_buf = b"".join(enlarged_lines)

        if False:
            print("enlarged_lines =")
            print(enlarged_lines)
            for i, line in enumerate(enlarged_lines):
                print(f"{i:3} ", end="")
                for b in line:
                    print("  " if b else "##", end="")
                print()

        print("enlarged_image_buf =")
        for y in range(enlarged_image_height):
            for x in range(enlarged_image_width):
                print(
                    "  " if enlarged_image_buf[y * enlarged_image_width + x] else "##",
                    end="",
                )
            print("\n", end="")

        return enlarged_image_buf, enlarged_image_width, enlarged_image_height

    image_buf, image_width, image_height = enlarge(
        image_buf,
        image_width,
        image_height,
        n_pad=1,
        int_scale=1,
    )

    # convert to png:
    # ffmpeg -f rawvideo -pix_fmt gray -video_size 31x31 -i qr.bin -y yo.png
    (ROOT / "qr.bin").write_bytes(image_buf)
    print("qr.bin", image_width, "x", image_height)

    #
    # scan
    #

    payload = do_scan(image_buf, image_width, image_height)
    print(payload)


if __name__ == "__main__":
    main()
