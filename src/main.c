/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f10x.h"
#include <stdio.h>
#include "image.h"

			
extern void debug_init(void);
extern void jump2addr(uint32_t addr);


#define FIRMWARE_1_START  0x08010000UL
#define FIRMWARE_1_SIZE   0x10000UL
#define FIRMWARE_2_START  0x08020000UL
#define FIRMWARE_2_SIZE   0x10000UL

#define FIRMWARE_B_START  0x08030000UL
#define FIRMWARE_B_SIZE   0x10000UL



struct firmware_region
{
	uint32_t start;
	uint32_t size;
} firmware_array[] =
{
		{FIRMWARE_1_START, FIRMWARE_1_SIZE},
		{FIRMWARE_2_START, FIRMWARE_2_SIZE},
		{FIRMWARE_B_START, FIRMWARE_B_SIZE},
};


int main(void)
{
	int i;

	uint32_t addr_bak  = firmware_array[2].start;
	uint32_t addr_user = firmware_array[1].start;

	debug_init();

	printf("\nSTM32 Bootloader ...\n\n");

	if (firmware_valid(addr_bak)) {

		image_header_t *hdr = addr_bak - sizeof(image_header_t);

		uint32_t addr = ntohl(hdr->ih_load);

		if (addr != addr_user)
		{
//			printf("BAK region header's loadaddr not match USER region\n");
			firmware_erase(addr_bak, 0);
			goto load_user;
		}


#if 0
		int i;
		for (i = 0; i < cnt - 1; i++) {
			if (addr_user == firmware_array[i].start)
				break;
		}
		if (i >= cnt - 1) {
			goto jump1;
		}
#endif

		printf("Updating the USER region...\n");
	    FLASH_Unlock();
		if (!firmware_erase(addr_user, ntohl(hdr->ih_size))) {
			printf("flash_erase error1\n");
		}
		if (!firmware_write(addr_user, addr_bak)) {
			printf("flash_write error1\n");
		}
		if (!firmware_erase(addr_bak, 0)) {
			printf("flash_erase error2\n");
		}
		FLASH_Lock();
		printf("Update Done\n");
	} else {
//		printf("BAK region not available, goto USER region\n");
	}

load_user:

	if (!firmware_valid(addr_user)) {

//		printf("USER region not available, goto Original region\n");

		addr_user = firmware_array[0].start;

		if (!firmware_valid(addr_user)) {
			printf("No Firmware available, Please Download uImage to 0x%08\n", FIRMWARE_1_START - sizeof(image_header_t));
			printf("System halting...\n");
			while (1);
		}
	}

	printf("\n");

	print_header(addr_user - sizeof(image_header_t));

	printf("\nJumping...\n\n");

	jump2addr(addr_user);

	for (;;);
}


/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);
pFunction Jump_To_Application;

void jump2addr(uint32_t addr)
{
	uint32_t JumpAddress;
    /* Check if valid stack address (RAM address) then jump to user application */
    if (((*(__IO uint32_t*)addr) & 0x2FFE0000 ) == 0x20000000)
    {
      /* Jump to user application */
      JumpAddress = *(__IO uint32_t*) (addr + 4);
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) addr);

//      NVIC_SetVectorTable(NVIC_VectTab_RAM, addr);
      NVIC_SetVectorTable(NVIC_VectTab_FLASH, addr & 0x000FFFFF);

//      NVIC_SystemReset();

      Jump_To_Application();
    }
}
