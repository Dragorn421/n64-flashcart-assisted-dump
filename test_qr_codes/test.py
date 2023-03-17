from pathlib import Path
import ctypes

ROOT = Path(__file__).parent


PRINT_CODE = False


def do_generate(payload: bytes, version: int):

    generate = ctypes.CDLL(ROOT / "generate.so")
    # int32_t generate(char *data, uint8_t version, uint8_t *buf, size_t buf_size, int32_t *width, int32_t *height)
    generate.generate.restype = ctypes.c_int32
    generate.generate.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint8,
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
        version,
        buf,
        len(buf),
        ctypes.byref(width),
        ctypes.byref(height),
    )
    print("generate_res =", generate_res)
    assert generate_res == 0

    return bytes(buf[: width.value * height.value]), width.value, height.value


def enlarge(image_buf, image_width, image_height, *, n_pad=0, int_scale=1):
    lines = [
        image_buf[i : i + image_width] for i in range(0, len(image_buf), image_width)
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

    if PRINT_CODE:
        print("enlarged_image_buf =")
        for y in range(enlarged_image_height):
            for x in range(enlarged_image_width):
                print(
                    "  " if enlarged_image_buf[y * enlarged_image_width + x] else "##",
                    end="",
                )
            print("\n", end="")

    return enlarged_image_buf, enlarged_image_width, enlarged_image_height


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


def test_roundtrip_payload(initial_payload, *, version=3, n_pad=0, int_scale=1):

    print(
        ">test_roundtrip_payload",
        "initial_payload =",
        initial_payload,
        "version =",
        version,
    )

    #
    # generate
    #

    gen_buf, gen_width, gen_height = do_generate(initial_payload, version)
    print(gen_width)
    print(gen_height)
    print(gen_buf[:100])

    if PRINT_CODE:
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

    image_buf, image_width, image_height = enlarge(
        image_buf,
        image_width,
        image_height,
        n_pad=n_pad,
        int_scale=int_scale,
    )

    # convert to png:
    # ffmpeg -f rawvideo -pix_fmt gray -video_size 31x31 -i qr.bin -y yo.png
    (ROOT / "qr.bin").write_bytes(image_buf)
    print("qr.bin", image_width, "x", image_height)

    #
    # scan
    #

    scanned_payload = do_scan(image_buf, image_width, image_height)
    print(scanned_payload)

    assert initial_payload == scanned_payload


def main():
    max_sizes_by_version = dict()
    for version in [6]:  # range(1, 40 + 1):
        for i in range(100):
            try:
                test_roundtrip_payload(
                    b"a" * i,
                    version=version,
                    n_pad=20,
                    int_scale=3,
                )
            except:
                max_sizes_by_version[version] = i - 1
                break
    print(max_sizes_by_version)


if __name__ == "__main__":
    main()
