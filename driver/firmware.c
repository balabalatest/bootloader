/*
 * firmware.c
 *
 */
#include "image.h"

#include <stdio.h>
#include "stm32f10x_flash.h"

#define FLASH_PAGE_SIZE   0x800


int firmware_erase(uint32_t addr, uint32_t size)
{

	uint32_t FlashAddress = addr - FLASH_PAGE_SIZE;
	uint32_t LastAddress  = addr + size;

	while (FlashAddress <= (uint32_t) LastAddress) {
		if (FLASH_ErasePage(FlashAddress) == FLASH_COMPLETE) {
			FlashAddress += FLASH_PAGE_SIZE;
		} else {
			return 0;
		}
	}

	return 1;
}

int firmware_write(uint32_t dest, uint32_t src)
{
	image_header_t *hdr = src - sizeof(image_header_t);

	uint32_t src_addr  = src  - FLASH_PAGE_SIZE;
	uint32_t dist_addr = dest - FLASH_PAGE_SIZE;

	uint16_t count = (ntohl(hdr->ih_size) + FLASH_PAGE_SIZE) / 4;
	if ((ntohl(hdr->ih_size) + FLASH_PAGE_SIZE) % 4)
		count++;

	for (int i = 0; i < count; i++) {
		if ( FLASH_ProgramWord(dist_addr, *((uint32_t *)src_addr)) != FLASH_COMPLETE ) {
			return 0;
		}
		dist_addr += 4;
		src_addr  += 4;
	}

	return 1;
}

image_header_t header;

int firmware_valid(uint32_t addr)
{
	int len;
	char *data;
	uint32_t checksum;

	uint8_t *ptr = (uint8_t *) (addr - sizeof(image_header_t));
	image_header_t *hdr = &header;

	memset(hdr, 0, sizeof(image_header_t));
	memcpy(hdr, ptr, sizeof(image_header_t));

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
		return 0;
	}

	data = (char *) hdr;
	len  = sizeof(image_header_t);

	checksum = ntohl(hdr->ih_hcrc);

	hdr->ih_hcrc = htonl(0);	/* clear for re-calculation */

	if (crc32 (0, data, len) != checksum) {
//		printf ("ADDR: 0x%08 has bad header checksum!\n", addr);
		return 0;
	}

	data = (char *) addr;
	len  = ntohl(hdr->ih_size);

	checksum = ntohl(hdr->ih_dcrc);

	if (crc32 (0, data, len) != checksum) {
//		printf ("ADDR: 0x%08 has corrupted data!\n", addr);
		return 0;
	}

#if 0
	/* for multi-file images we need the data part, too */
	print_header ((image_header_t *)ptr);
#endif

	return 1;
}
