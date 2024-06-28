# ppp-sdz
Pretty Poor Privacy - Means of trusted boot at boot record level of class 6 (weakest) protection level defined by FSTEC (which almost all requirements are satisfied).

Kinda useless for actual purposes of boot protection, has no proper installer yet, but could be a good example of an UEFI driver with POSIX-UEFI lib with minimalistic GUI, based on GOP from UEFI

# Build
```make ./uefi/ && make```

Uses https://github.com/queso-fuego/UEFI-GPT-image-creator for creating drive image with proper GPT boot sector. write_gpt binary from there should be at root directory

# Usage
QEMU with OVMF should be installed for running testing script, which runs after make
For using on real hardware, write resulting hdd image to desirable drive with dd (e.g. removable USB drive)

```sudo dd if=test.hdd of=/dev/sdb1/ bs=1M```
