"""
find "minimal" (lexical order, could be better) n_pad and int_scale that allow
"reliably" (tested by doing a roundtrip on random data) encode/decode of 64 bytes of data,
encoded as the ascii encoding of the string representation in uppercase hex of the 64 bytes

with Ntests=1000, got: (n_pad, int_scale)
    4, 4
    1, 5

with Ntests = 10000:
    with:
        for n_pad in range(0, 10):
            for int_scale in range(10):
    nothing!
"""
Ntests = 10000


import random

import matplotlib.pyplot as plt

import test

n_pad_ok_x = []
int_scale_ok_y = []

for n_pad in range(0, 10):
    for int_scale in range(10):
        try:
            for _ in range(Ntests):
                payload = "".join(f"{b:02X}" for b in random.randbytes(64)).encode(
                    "ascii"
                )
                print(payload)
                assert len(payload) == 128
                test.test_roundtrip_payload(
                    payload, version=6, n_pad=n_pad, int_scale=int_scale
                )
        except AssertionError:
            success = False
        else:
            success = True
        if success:
            n_pad_ok_x.append(n_pad)
            int_scale_ok_y.append(int_scale)
            break
    if success:
        break

plt.switch_backend("WebAgg")
plt.figure()
plt.scatter(n_pad_ok_x, int_scale_ok_y)
plt.show()
