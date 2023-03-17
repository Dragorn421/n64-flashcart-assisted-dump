from pathlib import Path
import ctypes

ROOT = Path(__file__).parent


#
# generate
#

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
    ctypes.create_string_buffer(b"hello world"),
    buf,
    len(buf),
    ctypes.byref(width),
    ctypes.byref(height),
)
print("generate_res =", generate_res)

print(width.value)
print(height.value)
print(buf[:100])

for y in range(height.value):
    for x in range(width.value):
        print("##" if buf[y * width.value + x] else "  ", end="")
    print("\n", end="")

#
# dump
#

# "true" is black
image_buf_bytes = bytes(0 if b else 0xFF for b in buf[: width.value * height.value])

# convert to png:
# ffmpeg -f rawvideo -pix_fmt gray -video_size 29x29 -i qr.bin -y yo.png
(ROOT / "qr.bin").write_bytes(image_buf_bytes)


#
# scan
#

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
    (ctypes.c_uint8 * len(image_buf_bytes))(*image_buf_bytes),
    width.value,
    height.value,
    out_payload_buf,
    len(out_payload_buf),
)
print("scan_res =", scan_res)

print(out_payload_buf[: out_payload_buf[:].index(0)])
