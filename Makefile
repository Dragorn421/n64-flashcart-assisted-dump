all: romfile.z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

OBJS_DEPS = qrencode/.libs/libqrencode.a

SRC_FILES = main.c cpu_utils.c
OBJS = $(addprefix $(BUILD_DIR)/,$(SRC_FILES:.c=.o)) $(OBJS_DEPS)

romfile.z64: N64_ROM_TITLE = "Flashcart Assisted Dump"

$(BUILD_DIR)/romfile.elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
