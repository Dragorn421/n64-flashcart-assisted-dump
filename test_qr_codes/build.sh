gcc -shared generate.c ../QRCode/src/qrcode.c -o generate.so

# cd quirc && make
gcc -shared scan.c ../quirc/libquirc.a -o scan.so -lm
