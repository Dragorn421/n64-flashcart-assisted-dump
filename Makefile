all: romfile.z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/QRCode/src/qrcode.o

romfile.z64: N64_ROM_TITLE = "Flashcart Assisted Dump"

$(BUILD_DIR)/romfile.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
