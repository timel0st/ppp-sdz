# Output file
TARGET = BOOTX64.EFI
# ./uefi - POSIX UEFI files
include uefi/Makefile

DISK_IMG_FOLDER = .
DISK_IMG_PGM = write_gpt -ae / font.sfn
QEMU_SCRIPT = qemu.sh

all: $(TARGET)
	cd $(DISK_IMG_FOLDER); \
	./$(DISK_IMG_PGM); \
	./$(QEMU_SCRIPT)