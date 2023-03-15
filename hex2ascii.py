# decodes a string of hex nibbles into an ascii string

chars_hex = "5A454C4441204D414A4F52412753204D41534B20"  # -> "ZELDA MAJORA'S MASK "

chars = []
for i in range(0, len(chars_hex), 2):
    c_hex = chars_hex[i : i + 2]
    c = int(c_hex, 16)
    chars.append(c)

ascii = bytes(chars).decode("ascii")

print(repr(ascii))
