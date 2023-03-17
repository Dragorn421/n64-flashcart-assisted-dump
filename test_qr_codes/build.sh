# Note: I had crashes at one point, and added -g to debug it, and then it no longer crashed.
# so the C code may have a flaw somewhere :)

gcc -g -shared generate.c ../QRCode/src/qrcode.c -o generate.so

# cd quirc && make
gcc -g -shared scan.c ../quirc/libquirc.a -o scan.so -lm
