/* ============================================================================================================================================================= *\
   Pico-Flash-Driver.h
   St-Louys Andre - April 2024
   astlouys@gmail.com
   Revision 31-AUG-2024
   Langage: C with arm-none-eabi

   From an original version by Raspberry Pi (Trading) Ltd.

   NOTE:
   THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
   WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
   TIME. AS A RESULT, THE AUTHOR SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
   INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM
   THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
   INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCT.
\* ============================================================================================================================================================= */

#ifndef __PICO_FLASH_DRIVER_H
#define __PICO_FLASH_DRIVER_H

/* $PAGE */
/* $TITLE=Include files. */
/* ============================================================================================================================================================= *\
                                                                        Include files.
\* ============================================================================================================================================================= */
#include "baseline.h"
#include "hardware/flash.h"
/// #include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"





/* $PAGE */
/* $TITLE=Definitions */
/* ============================================================================================================================================================= *\
                                                                        Definitions.
\* ============================================================================================================================================================= */
/* Define if developer or release version. */
/// #define RELEASE_VERSION
#ifdef RELEASE_VERSION
#warning ===============> Firmware Pico-Flash_Driver built as RELEASE VERSION.
#else  // RELEASE_VERSION
#warning ===============> Firmware Pico-Flash_Driver built as DEVELOPER VERSION.
#endif  // RELEASE_VERSION


/* Polynom used for CRC16 calculation. Different authorities use different polynoms:
   0x8005, 0x1021, 0x1DCF, 0x755B, 0x5935, 0x3D65, 0x8BB7, 0x0589, 0xC867, 0xA02B, 0x2F15, 0x6815, 0xC599, 0x202D, 0x0805, 0x1CF5 */
#define CRC16_POLYNOM           0x1021

/* Offsets in Pico's 2 MB flash where to save configuration data. Starting at 2 MB flash highest limit minus 4096 bytes (0x1000) - At the very end of flash.
   More offsets are added in case we want to save more than 0x1000 (4096) bytes. Data saved must not override program's bytes. */
#define FLASH_DATA_OFFSET1  0x1FF000  // very last sector of flash in Pico's flash.
#define FLASH_DATA_OFFSET2  0x1FE000  // one sector before FLASH_DATA_OFFSET1
#define FLASH_DATA_OFFSET3  0x1FD000  // one sector before FLASH_DATA_OFFSET2
#define FLASH_DATA_OFFSET4  0x1FC000  // one sector before FLASH_DATA_OFFSET3
#define FLASH_DATA_OFFSET5  0x1FB000  // one sector before FLASH_DATA_OFFSET4
#define FLASH_DATA_OFFSET6  0x1FA000  // one sector before FLASH_DATA_OFFSET5
#define FLASH_DATA_OFFSET7  0x1F9000  // one sector before FLASH_DATA_OFFSET6
#define FLASH_DATA_OFFSET8  0x1F8000  // one sector before FLASH_DATA_OFFSET7
#define FLASH_DATA_OFFSET9  0x1F7000  // one sector before FLASH_DATA_OFFSET8
#define FLASH_DATA_OFFSET10 0x1F6000  // one sector before FLASH_DATA_OFFSET9

/* RAM base address. */
#define RAM_BASE_ADDRESS  0x20000000





/* $PAGE */
/* $TITLE=Functions prototype. */
/* ============================================================================================================================================================= *\
                                                                     Functions prototype.
\* ============================================================================================================================================================= */
/* Display flash content through external monitor. */
void flash_display(UINT32 Offset, UINT32 Length);

/* Erase data in Pico's flash memory. One sector of the flash (4096 bytes or 0x1000) must be erased at a time. */
static void flash_erase(UINT32 DataOffset);

/* Extract the CRC16 from the packet passed as an argument (it is the last 16 bits of the packet). */
UINT16 flash_extract_crc(UINT8 *Data, UINT16 DataSize);

/* Read data from flash memory at the specified offset. */
UINT8 flash_read_data(UINT32 DataOffset, UINT8 *Data, UINT16 DataSize);

/* Save current data to flash. */
UINT8 flash_save_data(UINT32 DataOffset, UINT8 *Data,  UINT16 DataSize);

/* Write data to Pico's flash memory. */
static UINT8 flash_write(UINT32 DataOffset, UINT8 *NewData, UINT16 NewDataSize);

/* Send a string to external monitor through Pico's UART or CDC USB. */
extern void uart_send(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...);

/* Find the cyclic redundancy check of the specified data. */
UINT16 util_crc16(UINT8 *Data, UINT16 DataSize);

/* Display binary data - whose pointer is passed has an argument - to an external monitor. */
void util_display_data(UCHAR *Data, UINT32 DataSize);

#endif  // __PICO_FLASH_DRIVER_H
