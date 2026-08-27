# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2022-2024 Spacemit, Inc

ifeq ($(CONFIG_RSA_VERIFY),)
its := $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/uboot_fdt.its
else
its := $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/uboot_fdt_sign.its
ifdef KEY_DIR
key_para := -k $(KEY_DIR)
else
key_para := -k $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/key
endif
endif

bootinfo_%.bin:
	$(Q)$(srctree)/tools/build_binary_file.py -o $@ -i $(obj) \
	-c $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/bootinfo_$*.json

u-boot-env-default.txt:
	$(Q)$(srctree)/scripts/get_default_envs.sh $(obj) > $@

u-boot.itb: u-boot-nodtb.bin u-boot-dtb.bin u-boot.dtb FORCE
	$(Q)tools/mkimage -D "-Idts -O dtb -p 500 -i $(objtree) -i $(objtree)/arch/$(ARCH)/dts" -f $(its) $(key_para) -r $@

ifneq ($(CONFIG_SPL_BUILD),)
INPUTS-y += FSBL.bin bootinfo_spinor.bin bootinfo_spinand.bin bootinfo_block.bin

# Add 4KB header for u-boot-spl.bin
FSBL.bin: $(obj)/u-boot-spl.bin FORCE
	$(Q)$(srctree)/tools/build_binary_file.py -o $@ -i $(obj) $(if $(KEY_DIR),--key-dir $(KEY_DIR)) \
	-c $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/fsbl.json
else
INPUTS-y += u-boot-env-default.bin
u-boot-env-default.bin: u-boot-env-default.txt
	$(Q)tools/mkenvimage -o $@ -s $(CONFIG_ENV_SIZE) $(obj)/u-boot-env-default.txt
endif
