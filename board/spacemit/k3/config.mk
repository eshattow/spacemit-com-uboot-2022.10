# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2022-2024 Spacemit, Inc


quiet_cmd_build_itb = BUILD   $2
cmd_build_itb = \
	mkdir -p $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/dtb && \
	cp $(srctree)/arch/$(ARCH)/dts/*.dtb $(srctree)/ && \
	cp $(srctree)/arch/$(ARCH)/dts/*.dtb \
		$(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/dtb/ && \
	cp $(srctree)/u-boot-nodtb.bin \
		$(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/ && \
	if test -z "$(CONFIG_RSA_VERIFY)"; then \
		test "$(CONFIG_SPL_LZO)" = "y" || { \
			echo "K3 compressed FIT requires CONFIG_SPL_LZO=y" >&2; exit 1; }; \
		for dtb in $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/dtb/*.dtb; do \
			lzop -9 -c < "$$dtb" > "$$dtb.lzo" || exit $$?; \
		done; \
		lzop -9 -c < $(srctree)/u-boot-nodtb.bin > \
			$(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/u-boot-nodtb.bin.lzo || exit $$?; \
	fi && \
	$(srctree)/tools/mkimage -f $3 $4 \
		-r $(srctree)/$2 && \
	rm -rf $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/dtb && \
	rm -f $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/u-boot-nodtb.bin \
		$(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/u-boot-nodtb.bin.lzo

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
	$(call if_changed,build_itb,$@,$(its),$(key_para))

ifneq ($(CONFIG_SPL_BUILD),)
INPUTS-y += FSBL.bin bootinfo_spinor.bin bootinfo_spinand.bin bootinfo_block.bin

FSBL.bin: spl/u-boot-spl.bin FORCE
	$(call if_changed,build_spl_platform,$@,$<)
FSBL.bin: $(obj)/u-boot-spl.bin FORCE
	$(Q)$(srctree)/tools/build_binary_file.py -o $@ -i $(obj) $(if $(KEY_DIR),--key-dir $(KEY_DIR)) \
	-c $(srctree)/board/$(CONFIG_SYS_VENDOR)/$(CONFIG_SYS_BOARD)/configs/fsbl.json
else
INPUTS-y += u-boot-env-default.bin
u-boot-env-default.bin: u-boot-env-default.txt
	$(Q)tools/mkenvimage -o $@ -s $(CONFIG_ENV_SIZE) $(obj)/u-boot-env-default.txt
endif
