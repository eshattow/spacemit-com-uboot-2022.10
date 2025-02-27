// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <asm/io.h>
#include <stdlib.h>
#include <tlv_eeprom.h>
#include <u-boot/crc.h>
#include <net.h>

#define EEPROM_SIZE       (256)
#define EEPROM_SIZE_MAX_TLV_LEN (EEPROM_SIZE - sizeof(struct tlvinfo_header))

extern int k1x_eeprom_init(void);
extern int _read_from_i2c(int chip, u32 addr, u32 size, uchar *buf);

/* File scope function prototypes */
static bool is_checksum_valid(u8 *eeprom);

/**
 *  _is_valid_tlvinfo_header
 *
 *  Perform sanity checks on the first 11 bytes of the TlvInfo EEPROM
 *  data pointed to by the parameter:
 *      1. First 8 bytes contain null-terminated ASCII string "TlvInfo"
 *      2. Version byte is 1
 *      3. Total length bytes contain value which is less than or equal
 *         to the allowed maximum (2048-11)
 *
 */
bool _is_valid_tlvinfo_header(struct tlvinfo_header *hdr)
{
	return ((strcmp(hdr->signature, TLV_INFO_ID_STRING) == 0) &&
		(hdr->version == TLV_INFO_VERSION) &&
		(be16_to_cpu(hdr->totallen) <= TLV_TOTAL_LEN_MAX));
}

static inline bool is_valid_tlv(struct tlvinfo_tlv *tlv)
{
	return((tlv->type != 0x00) && (tlv->type != 0xFF));
}

/**
 *  is_checksum_valid
 *
 *  Validate the checksum in the provided TlvInfo EEPROM data. First,
 *  verify that the TlvInfo header is valid, then make sure the last
 *  TLV is a CRC-32 TLV. Then calculate the CRC over the EEPROM data
 *  and compare it to the value stored in the EEPROM CRC-32 TLV.
 */
static bool is_checksum_valid(u8 *eeprom)
{
	struct tlvinfo_header *eeprom_hdr = (struct tlvinfo_header *)eeprom;
	struct tlvinfo_tlv    *eeprom_crc;
	unsigned int       calc_crc;
	unsigned int       stored_crc;

	// Is the eeprom header valid?
	if (!_is_valid_tlvinfo_header(eeprom_hdr)){
		pr_err("%s, not valid tlv info header\n", __func__);
		return false;
	}

	// Is the last TLV a CRC?
	eeprom_crc = (struct tlvinfo_tlv *)(&eeprom[sizeof(struct tlvinfo_header) +
		be16_to_cpu(eeprom_hdr->totallen) - (sizeof(struct tlvinfo_tlv) + 4)]);
	if (eeprom_crc->type != TLV_CODE_CRC_32 || eeprom_crc->length != 4)
		return false;

	// Calculate the checksum
	calc_crc = crc32(0, (void *)eeprom,
			 sizeof(struct tlvinfo_header) + be16_to_cpu(eeprom_hdr->totallen) - 4);
	stored_crc = (eeprom_crc->value[0] << 24) |
		(eeprom_crc->value[1] << 16) |
		(eeprom_crc->value[2] <<  8) |
		eeprom_crc->value[3];

	return calc_crc == stored_crc;
}

/**
 *  tlvinfo_find_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM.
 *  An offset from the beginning of the EEPROM is returned in the
 *  eeprom_index parameter if the TLV is found.
 */
bool tlvinfo_find_tlv(u8 *eeprom, u8 tcode, int *eeprom_index)
{
	struct tlvinfo_header *eeprom_hdr = (struct tlvinfo_header *)eeprom;
	struct tlvinfo_tlv    *eeprom_tlv;
	int eeprom_end;

	// Search through the TLVs, looking for the first one which matches the
	// supplied type code.
	*eeprom_index = sizeof(struct tlvinfo_header);
	eeprom_end = sizeof(struct tlvinfo_header) + be16_to_cpu(eeprom_hdr->totallen);
	while (*eeprom_index < eeprom_end) {
		eeprom_tlv = (struct tlvinfo_tlv *)(&eeprom[*eeprom_index]);
		if (!is_valid_tlv(eeprom_tlv))
			return false;
		if (eeprom_tlv->type == tcode)
			return true;
		*eeprom_index += sizeof(struct tlvinfo_tlv) + eeprom_tlv->length;
	}
	return(false);
}

static int read_tlvinfo_from_eeprom(u8 *eeprom)
{
	struct tlvinfo_header *eeprom_hdr = (struct tlvinfo_header *)eeprom;
	struct tlvinfo_tlv *eeprom_tlv = (struct tlvinfo_tlv *)(&eeprom[sizeof(struct tlvinfo_header)]);
	int chip;

	chip = k1x_eeprom_init();
	if (chip < 0){
		pr_err("can not get i2c bus addr\n");
		return -1;
	}

	/*read tlv head info*/
	if (_read_from_i2c(chip, 0, sizeof(struct tlvinfo_header), (uchar *)eeprom_hdr)){
		pr_err("read tlvinfo_header from i2c fail\n");
		return -1;
	}

	if (_is_valid_tlvinfo_header(eeprom_hdr)){
		if (_read_from_i2c(chip, sizeof(struct tlvinfo_header), be16_to_cpu(eeprom_hdr->totallen),
							(uchar *)eeprom_tlv)){
			pr_err("read tlvinvo_tlv from i2c fail\n");
			return -1;
		}
	}

	// If the contents are invalid, start over with default contents
	if (!_is_valid_tlvinfo_header(eeprom_hdr) ||
	    !is_checksum_valid(eeprom)) {
		strcpy(eeprom_hdr->signature, TLV_INFO_ID_STRING);
		eeprom_hdr->version = TLV_INFO_VERSION;
		eeprom_hdr->totallen = cpu_to_be16(0);
		pr_info("update new tlv info\n");
	}
	return 0;
}

int get_tlvinfo_from_eeprom(int tcode, char *buf)
{
	int tlv_end;
	int curr_tlv;
	u8 eeprom[EEPROM_SIZE];
	memset(eeprom, 0, EEPROM_SIZE);

	if (read_tlvinfo_from_eeprom(eeprom)){
		pr_err("read tlv info fail\n");
		return -1;
	}

	struct tlvinfo_header *eeprom_hdr = (struct tlvinfo_header *)eeprom;
	struct tlvinfo_tlv    *eeprom_tlv;

	curr_tlv = sizeof(struct tlvinfo_header);
	tlv_end  = sizeof(struct tlvinfo_header) + be16_to_cpu(eeprom_hdr->totallen);
	while (curr_tlv < tlv_end) {
		eeprom_tlv = (struct tlvinfo_tlv *)(&eeprom[curr_tlv]);
		if (!is_valid_tlv(eeprom_tlv)) {
			pr_err("Invalid TLV field starting at EEPROM offset %d\n",
			       curr_tlv);
			return -1;
		}

		if (eeprom_tlv->type == tcode){
			memcpy(buf, eeprom_tlv->value, eeprom_tlv->length);
			pr_info("get tlvinfo value:%x,%s\n", tcode, buf);
			return 0;
		}
		curr_tlv += sizeof(struct tlvinfo_tlv) + eeprom_tlv->length;
	}
	pr_info("can not get tlvinfo index:%x\n", tcode);
	return -1;
}
