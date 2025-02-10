/* ============================================================================================================================================================= *\
   Pico-Flash-Module.c
   St-Louys Andre - April 2024
   astlouys@gmail.com
   Revision 09-FEB-2025
   Version 2.00
   Langage: C

   From an original version by Raspberry Pi (Trading) Ltd.

   NOTE:
   THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
   WITH CODIN INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
   TIME. AS A RESULT, THE AUTHOR SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
   INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM
   THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
   INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCT.

   31-AUG-2024 - 1.00 - Initial release from an original by Raspberry Pi (Trading) Ltd.
   09-FEB-2025 - 2.00 - Adapted to become a module that could be easily integrated into an existing C-language program.
\* ============================================================================================================================================================= */

/* ============================================================================================================================================================= *\
                                                                    Include files.
\* ============================================================================================================================================================= */
#include "Pico-Flash-Module.h"





/* ============================================================================================================================================================= *\
                                                                    Global variables.
\* ============================================================================================================================================================= */





/* ============================================================================================================================================================= *\
                                                          Function prototypes for local functions.
\* ============================================================================================================================================================= */
/* Read a string from stdin. */
void static input_string(UCHAR *String);

/* Pause for specified number of msec. */
void static wait_ms(UINT16 WaitMSec);





/* $PAGE */
/* $TITLE=flash_display() */
/* ============================================================================================================================================================= *\
                                                      Display flash content through external monitor.
\* ============================================================================================================================================================= */
void flash_display(UINT32 Offset, UINT32 Length)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UCHAR String[256];

  UINT8 *FlashBaseAddress;

  UINT32 Loop1UInt32;
  UINT32 Loop2UInt32;


  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Entering flash_display()\r");
    uart_send(__LINE__, __func__, "Offset: 0x%8.8lX     Length: 0x%8.8lX  (%lu)\r\r\r", Offset, Length, Length);
  }

  /* Compute target flash memory address.
     NOTE: XIP_BASE ("eXecute-In-Place") is the base address of the flash memory in Pico's address space (memory map). */
  FlashBaseAddress = (UINT8 *)(XIP_BASE);

  uart_send(__LINE__, __func__, " XIP_BASE: 0x%p   Offset: 0x%6.6X   Length: 0x%X (%u)\r", XIP_BASE, Offset, Length, Length);
  uart_send(__LINE__, __func__, " ================================================================================\r");


  /* Display target address and 16 bytes in hex. */
  for (Loop1UInt32 = Offset; Loop1UInt32 < (Offset + Length); Loop1UInt32 += 16)
  {
    sprintf(String, " [%p] ", XIP_BASE + Loop1UInt32);

    for (Loop2UInt32 = 0; Loop2UInt32 < 16; ++Loop2UInt32)
    {
      sprintf(&String[strlen(String)], "%2.2X ", FlashBaseAddress[Loop1UInt32 + Loop2UInt32]);
    }
    uart_send(__LINE__, __func__, String);


    /* Display separator. */
    sprintf(String, "| ");


    /* Display same bytes in ASCII or a dot <.> if not printable. */
    for (Loop2UInt32 = 0; Loop2UInt32 < 16; ++Loop2UInt32)
    {
      if ((FlashBaseAddress[Loop1UInt32 + Loop2UInt32] >= 0x20) && (FlashBaseAddress[Loop1UInt32 + Loop2UInt32] <= 0x7E)  && (FlashBaseAddress[Loop1UInt32 + Loop2UInt32] != 0x25))
      {
        sprintf(&String[Loop2UInt32 + 2], "%c", FlashBaseAddress[Loop1UInt32 + Loop2UInt32]);
      }
      else
      {
        sprintf(&String[Loop2UInt32 + 2], ".");
      }
    }
    uart_send(__LINE__, __func__, String);
    uart_send(__LINE__, __func__, "\r");
  }

  if (FlagLocalDebug) uart_send(__LINE__, __func__, "Exiting flash_display()\r");

  return;
}





/* PAGE */
/* $TITLE=flash_erase() */
/* ============================================================================================================================================================= *\
             Erase data in Pico's flash memory. The way the Pico's electronics is done, one sector of the flash (4096 bytes) must be erased at a time.
\* ============================================================================================================================================================= */
UINT8 flash_erase(UINT32 DataOffset)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT32 InterruptMask;


  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Entering flash_erase()\r");
    uart_send(__LINE__, __func__, "DataOffset: 0x%8.8lX\r\r\r", DataOffset);

    /* Wait for interrupts to clear from uart_send() above before disable them. */
    wait_ms(200);
  }


  if (DataOffset % FLASH_SECTOR_SIZE)
  {
    /* Data offset specified is not aligned on a flash sector boundary. */
    uart_send(__LINE__, __func__, "*** FATAL *** The data offset specified (0x%8.8X) is not aligned on a flash sector boundary (multiple of 0x1000)\r", DataOffset);
    uart_send(__LINE__, __func__, "Phased out by 0x%X (%u) bytes.\r", DataOffset % FLASH_SECTOR_SIZE, DataOffset % FLASH_SECTOR_SIZE);
    uart_send(__LINE__, __func__, "Three last hex digits of DataOffset must be 0x000.\r");

    return 1;
  }


  /* Erase an area of the Pico's flash memory. Keep track of interrupt mask on entry. */
  InterruptMask = save_and_disable_interrupts();

  /* Erase flash area to be reprogrammed. */
  flash_range_erase(DataOffset, FLASH_SECTOR_SIZE);

  /* Restore original interrupt mask when done. */
  restore_interrupts(InterruptMask);

  if (FlagLocalDebug) uart_send(__LINE__, __func__, "Exiting flash_erase()\r");

  return 0;
}





/* $PAGE */
/* $TITLE=flash_extract_crc() */
/* ============================================================================================================================================================= *\
                                                                   Extract CRC16 from a data packet.
             NOTES: It is assumed that the CRC16 is an unsigned integer 16 and that it is the last 2 bytes (last 16 bits) of the packet passed as an argument.
                    DateSize represents the full size of the packet, including the CRC.
\* ============================================================================================================================================================= */
UINT16 flash_extract_crc(UINT8 *Data, UINT16 DataSize)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT16 Crc16;


  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Entering flash_extract()\r");
    uart_send(__LINE__, __func__, "Data: 0x%8.8lX     DataSize: 0x%4.4X  (%u)\r\r\r", Data, DataSize, DataSize);
  }

  Crc16 = *(UINT16 *)(Data + DataSize - 2);

  /* Extract CRC16 from packet passed as an argument (last packet 16 bits). */
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "RAM base address:         0x%X\r",       RAM_BASE_ADDRESS);
    uart_send(__LINE__, __func__, "Data pointer:             0x%X\r",       Data);
    uart_send(__LINE__, __func__, "Data size:                0x%X  (%u)\r", DataSize, DataSize);
    uart_send(__LINE__, __func__, "Pointer to CRC16:         0x%X\r",       Data + DataSize - 2);
    uart_send(__LINE__, __func__, "Value of CRC found:       0x%X\r\r\r",   Crc16);
  }

  return Crc16;
}





/* $PAGE */
/* $TITLE=flash_read_data() */
/* ============================================================================================================================================================= *\
                                                                   Read data from flash memory.
\* ============================================================================================================================================================= */
UINT8 flash_read_data(UINT32 DataOffset, UINT8 *Data, UINT16 DataSize)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT8 *FlashBaseAddress;

  UINT16 Crc16Computed;
  UINT16 Crc16Extracted;
  UINT16 Loop1UInt16;


  /* Check if an external terminal is connected to the Pico. */
  if (!stdio_usb_connected()) FlagLocalDebug = FLAG_OFF;

  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, " =======================================================================================================================\r");
    uart_send(__LINE__, __func__, "      Entering flash_read_data()\r");
    uart_send(__LINE__, __func__, "      Read current data from Pico's flash\r");
    uart_send(__LINE__, __func__, "      XIP_BASE (flash base address):              0x%8.8X\r", XIP_BASE);
    uart_send(__LINE__, __func__, "      Data offset in flash:                       0x%8.8X\r", DataOffset);
    uart_send(__LINE__, __func__, "      Pointer to variable that will contain data: 0x%8.8X\r", Data);
    uart_send(__LINE__, __func__, "      Size of data to be read from flash:             0x%4.4X  (%u)\r", DataSize, DataSize);
    uart_send(__LINE__, __func__, "      Displaying data retrieved from flash memory...\r");
    uart_send(__LINE__, __func__, " =======================================================================================================================\r");
  }

  /* Read configuration data from Pico's flash memory (as an array of UINT8). */
  FlashBaseAddress = (UINT8 *)(XIP_BASE);
  for (Loop1UInt16 = 0; Loop1UInt16 < DataSize; ++Loop1UInt16)
    Data[Loop1UInt16] = FlashBaseAddress[DataOffset + Loop1UInt16];

  Crc16Extracted = flash_extract_crc(Data, DataSize);  // CRC16 extracted from data retrieved from flash memory.
  Crc16Computed  = util_crc16(Data, DataSize - 2);     // CRC16 computed from data retrieved from flash (excluding the CRC16 itself).


  /* Optionally display configuration data retrieved from flash memory and CRC16. */
  if (FlagLocalDebug)
  {
    util_display_data(Data, DataSize);
    uart_send(__LINE__, __func__, "CRC16 extracted from packet:   0x%4.4X\r",     Crc16Extracted);
    uart_send(__LINE__, __func__, "CRC16 computed from data read: 0x%4.4X\r\r\r", Crc16Computed);
  }


  if (Crc16Extracted != Crc16Computed)
  {
    /* CRC16 is not valid. */
    if (stdio_usb_connected()) uart_send(__LINE__, __func__, "Flash configuration is invalid.\r");

    return 1;
  }

  return 0;  // CRC is valid.
}





/* $PAGE */
/* $TITLE=flash_save_data() */
/* ============================================================================================================================================================= *\
                                                                    Save current data to flash.
\* ============================================================================================================================================================= */
UINT8 flash_save_data(UINT32 DataOffset, UINT8 *Data, UINT16 DataSize)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_ON;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT16 Crc16;

  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Entering flash_save_data()\r");
    uart_send(__LINE__, __func__, "=========================================================================================================\r");
    uart_send(__LINE__, __func__, "     FLASH_SECTOR_SIZE:                      %8u\r",        FLASH_SECTOR_SIZE);
    uart_send(__LINE__, __func__, "     XIP_BASE (flash base address):        0x%8.8X\r",      XIP_BASE);
    uart_send(__LINE__, __func__, "     Data offset:                          0x%8.8X\r",      DataOffset);
    uart_send(__LINE__, __func__, "     Pointer to beginning of Data:         0x%8.8X\r",      Data);
    uart_send(__LINE__, __func__, "     Data size:                            0x%8.8X (%u)\r", DataSize, DataSize);
    uart_send(__LINE__, __func__, "     Data size for CRC16 calculation:      0x%8.8X (%u)\r", (((Data - RAM_BASE_ADDRESS + DataSize) - (Data - RAM_BASE_ADDRESS)) - 2), (((Data - RAM_BASE_ADDRESS + DataSize) - (Data - RAM_BASE_ADDRESS)) - 2));
    uart_send(__LINE__, __func__, "     Pointer to CRC16:                     0x%8.8X\r",      Data + DataSize - 2);
    uart_send(__LINE__, __func__, "=========================================================================================================\r");
    
    /* Display data being saved. */
    uart_send(__LINE__, __func__, "     Current data before computing CRC16:\r");
    util_display_data(Data, DataSize);

    /* Wait for interrupts to clear in uart_send() above before disable them. */
    wait_ms(500);
  }

  /* Validate size of data. */
  if (DataSize > 0x1000)
  {
    printf("\r\r\r\r\r");
    uart_send(__LINE__, __func__, "*** FATAL *** Data size to save to flash is too big (0x%4.4X)\r", DataSize);
    uart_send(__LINE__, __func__, "Must be 0x1000 maximum. Fix this problem and rebuild the Firmware...\r\r");

    return 1;
  }


  if (DataOffset % FLASH_SECTOR_SIZE)
  {
    /* Data offset specified is not aligned on a flash sector boundary. */
    uart_send(__LINE__, __func__, "*** FATAL *** The data offset specified (0x%8.8X) is not aligned on a flash sector boundary (multiple of 0x1000)\r", DataOffset);
    uart_send(__LINE__, __func__, "Phased out by 0x%X (%u) bytes.\r", DataOffset % FLASH_SECTOR_SIZE, DataOffset % FLASH_SECTOR_SIZE);
    uart_send(__LINE__, __func__, "Three last hex digits of DataOffset must be 0x000.\r");

    return 1;
  }


  /* Compute CRC16 of packet to be saved. */
  Crc16 = util_crc16(Data, DataSize - 2);
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Pointer to data to be saved to flash: 0x%p\r", Data);
    uart_send(__LINE__, __func__, "Data size:                                  %4u (0x%X)\r", DataSize,     DataSize);
    uart_send(__LINE__, __func__, "Data size for CRC calculation:              %4u (0x%X)\r", DataSize - 2, DataSize - 2);
    uart_send(__LINE__, __func__, "CRC computed:                             0x%4.4X\r", Crc16);
  }

  /* Insert CRC16 as last 16 bits of the packet. */
  *(UINT16 *)(Data + DataSize - 2) = Crc16;

  /* Save data to flash. */
  flash_write(DataOffset, Data, DataSize);

  /* Display flash data as saved. NOTE: Will crash the firmware if done inside a callback. */
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Display flash data as saved:\r");
    util_display_data(Data, DataSize);
    uart_send(__LINE__, __func__, "Exiting flash_save_data())\r");
  }

  return 0;
}





/* $PAGE */
/* $TITLE=flash_write() */
/* ============================================================================================================================================================= *\
                                                               Write data to Pico's flash memory.
\* ============================================================================================================================================================= */
UINT8 flash_write(UINT32 DataOffset, UINT8 *NewData, UINT16 NewDataSize)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UCHAR String[32];

  UINT8 *FlashBaseAddress;
  UINT8 *FlashSector;

  UINT16 Loop1UInt16;

  UINT32 InterruptMask;


  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Entering flash_write()\r");
    uart_send(__LINE__, __func__, "=========================================================================================================\r");
    uart_send(__LINE__, __func__, "     FLASH_SECTOR_SIZE:                    0x%8.8X (%u)\r", FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    uart_send(__LINE__, __func__, "     XIP_BASE (flash base address):        0x%8.8X\r",      XIP_BASE);
    uart_send(__LINE__, __func__, "     Data offset:                          0x%8.8X\r",      DataOffset);
    uart_send(__LINE__, __func__, "     Pointer to beginning of data:         0x%8.8X\r",      NewData);
    uart_send(__LINE__, __func__, "     Data size for CRC16 calculation:      0x%8.8X (%u)\r", (((NewData - RAM_BASE_ADDRESS + NewDataSize) - (NewData - RAM_BASE_ADDRESS)) - 2), (((NewData - RAM_BASE_ADDRESS + NewDataSize) - (NewData - RAM_BASE_ADDRESS)) - 2));
    uart_send(__LINE__, __func__, "     Pointer to CRC16:                     0x%8.8X\r",      NewData + NewDataSize - 2);
    uart_send(__LINE__, __func__, "     Data size:                            0x%8.8X (%u)\r", NewDataSize, NewDataSize);
    uart_send(__LINE__, __func__, "=========================================================================================================\r");
    uart_send(__LINE__, __func__, "     Displaying data to be written to flash.\r");
    util_display_data(NewData, NewDataSize);

    /* Wait for interrupts to clear from uart_send() above before disabling them. */
    wait_ms(100);
  }


  if (DataOffset % FLASH_SECTOR_SIZE)
  {
    /* Data offset specified is not aligned on a flash sector boundary. */
    uart_send(__LINE__, __func__, "*** FATAL *** The data offset specified (0x%8.8X) is not aligned on a flash sector boundary (multiple of 0x1000)\r", DataOffset);
    uart_send(__LINE__, __func__, "Phased out by 0x%X (%u) bytes.\r", DataOffset % FLASH_SECTOR_SIZE, DataOffset % FLASH_SECTOR_SIZE);
    uart_send(__LINE__, __func__, "Three last hex digits of DataOffset must be 0x000.\r");

    return 1;
  }


  /* NOTE: A wear leveling algorithm has not been implemented since the flash usage for saving configuration data will usually not require it.
     However, flash write should not be used for intensive data logging without adding a wear leveling algorithm. */
  FlashBaseAddress = (UINT8 *)(XIP_BASE);
  FlashSector      = malloc(FLASH_SECTOR_SIZE);
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "FlashSector address: 0x%p\r", FlashSector);
    uart_send(__LINE__, __func__, "malloc() returned 0x%p for FlashSector\r", FlashSector);
    sleep_ms(100);    ///
  }


  /* Take a copy of current flash sector content. */
  for (Loop1UInt16 = 0; Loop1UInt16 < FLASH_SECTOR_SIZE; ++Loop1UInt16) FlashSector[Loop1UInt16] = FlashBaseAddress[DataOffset + Loop1UInt16];
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "FlashSector: 0x%p   FlashBaseAddress: 0x%p   Data offset: 0x%6.6X\r", FlashSector, FlashBaseAddress, DataOffset);
    uart_send(__LINE__, __func__, "Displaying original data retrieved from flash\r");
    util_display_data(FlashSector, FLASH_SECTOR_SIZE);
  }


  /* Overwrite the sector area that we want to save. */
  memcpy(FlashSector, NewData, NewDataSize);
  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Display data to be written back to flash at offset %X:\r", DataOffset);
    util_display_data(FlashSector, FLASH_SECTOR_SIZE);

    /* Wait for interrupts to clear from uart_send() above before disable them. */
    wait_ms(1000);
  }


  /* Erase flash before reprogramming. */
  if (flash_erase(DataOffset)) return 1;  // return in case of error while trying to erase.

  /* Keep track of interrupt mask and disable interrupts during flash writing. */
  InterruptMask = save_and_disable_interrupts();

  /* Save data to flash memory. */
  flash_range_program(DataOffset, FlashSector, FLASH_SECTOR_SIZE);

  /* Restore original interrupt mask when done. */
  restore_interrupts(InterruptMask);

  /* Release memory when done. */
  free(FlashSector);

  if (FlagLocalDebug) uart_send(__LINE__, __func__, "Exiting flash_write()\r");

  return 0;
}





/* $PAGE */
/* $TITLE=input_string() */
/* ============================================================================================================================================================= *\
                                                                            Read a string from stdin.
\* ============================================================================================================================================================= */
void static input_string(UCHAR *String)
{
  INT8 DataInput;

  UINT8 Loop1UInt8;


  Loop1UInt8 = 0;
  do
  {
    DataInput = getchar_timeout_us(50000);

    switch (DataInput)
    {
      case (PICO_ERROR_TIMEOUT):
      case (0):
        continue;
      break;

      case (8):
        /* <Backspace> */
        if (Loop1UInt8 > 0)
        {
          --Loop1UInt8;
          String[Loop1UInt8] = 0x00;
          printf("%c %c", 0x08, 0x08);  // erase character under the cursor.
        }
      break;

      case (27):
        /* <ESC> */
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      case (0x0D):
        /* <Enter> */
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      default:
        printf("%c", (UCHAR)DataInput);
        String[Loop1UInt8] = (UCHAR)DataInput;
        // printf("Loop1UInt8: %3u   %2.2X - %c\r", Loop1UInt8, DataInput, DataInput);  /// for debugging purposes.
        ++Loop1UInt8;
      break;
    }
  } while((Loop1UInt8 < 128) && (DataInput != 0x0D));

  String[Loop1UInt8] = '\0';  // end-of-string
  /// printf("\r\r\r");

  /* Optionally display each character entered. */
  /***
  for (Loop1UInt8 = 0; Loop1UInt8 < 10; ++Loop1UInt8)
    printf("%2u:[%2.2X]   ", Loop1UInt8, String[Loop1UInt8]);
  printf("\r");
  ***/

  /// if (DebugBitMask & DEBUG_PROGRAM_FLOW) printf("Exiting input_string()\r");

  return;
}





/* $PAGE */
/* $TITLE=util_crc16() */
/* ============================================================================================================================================================= *\
                                                     Find the cyclic redundancy check of the specified data.
\* ============================================================================================================================================================= */
UINT16 util_crc16(UINT8 *Data, UINT16 DataSize)
{
#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

  UINT8 Loop1UInt8;

  UINT16 CrcValue;


  /* Validate data pointer. */
  if (Data == NULL) return 0;

  if (FlagLocalDebug)
  {
    uart_send(__LINE__, __func__, "Calculating CRC16 of this packet (Data pointer: 0x%8.8X   size: %u):\r", Data, DataSize);
    util_display_data(Data, DataSize);
  }

  CrcValue = 0;

  while (DataSize-- > 0)
  {
    CrcValue = CrcValue ^ (UINT8)*Data++ << 8;

    for (Loop1UInt8 = 0; Loop1UInt8 < 8; ++Loop1UInt8)
    {
      if (CrcValue & 0x8000)
        CrcValue = CrcValue << 1 ^ CRC16_POLYNOM;
      else
        CrcValue = CrcValue << 1;
    }
  }

  if (FlagLocalDebug)
    uart_send(__LINE__, __func__, "CRC16 computed: 0x%4.4X\r\r\r", CrcValue & 0xFFFF);

  return (CrcValue & 0xFFFF);
}





/* $PAGE */
/* $TITLE=util_display_data() */
/* ============================================================================================================================================================= *\
                                            Display data whose pointer is sent in argument to an external monitor.
\* ============================================================================================================================================================= */
void util_display_data(UCHAR *Data, UINT32 DataSize)
{
  UCHAR String[256];

  UINT32 Loop1UInt32;
  UINT32 Loop2UInt32;


  uart_send(__LINE__, __func__, " ===========================================================================================\r");
  uart_send(__LINE__, __func__, "      Entering util_display_data() - Data pointer: 0x%X   DataSize: 0x%4.4lX (%u)\r", Data, DataSize, DataSize);
  uart_send(__LINE__, __func__, " ===========================================================================================\r");
  uart_send(__LINE__, __func__, "                                                                             Printable\r");
  uart_send(__LINE__, __func__, "   Address     Offset                       Hex data                         characters\r");
  uart_send(__LINE__, __func__, " -------------------------------------------------------------------------------------------\r");


  for (Loop1UInt32 = 0; Loop1UInt32 < DataSize; Loop1UInt32 += 16)
  {
    /* First, display memory address, offset and hex part. */
    sprintf(String, "[0x%8.8X] [0x%4.4X] - ", &Data[Loop1UInt32], Loop1UInt32);

    for (Loop2UInt32 = 0; Loop2UInt32 < 16; ++Loop2UInt32)
    {
      if ((Loop1UInt32 + Loop2UInt32) >= DataSize)
        strcat(String, "   ");
      else
        sprintf(&String[strlen(String)], "%2.2X ", Data[Loop1UInt32 + Loop2UInt32]);
    }


    uart_send(__LINE__, __func__, String);


    /* Add separator. */
    sprintf(String, "| ");


    /* Then, display ASCII character if it is displayable ASCII (or <.> if it is not). */
    for (Loop2UInt32 = 0; Loop2UInt32 < 16; ++Loop2UInt32)
    {
      if ((Loop1UInt32 + Loop2UInt32) >= DataSize)
        break; // do not count up to 16 if we already reached end of data to display.

      /* Display character if it is displayable ASCII. */
      if ((Data[Loop1UInt32 + Loop2UInt32] >= 0x20) && (Data[Loop1UInt32 + Loop2UInt32] <= 0x7E) && (Data[Loop1UInt32 + Loop2UInt32] != 0x25))
        sprintf(&String[strlen(String)], "%c", Data[Loop1UInt32 + Loop2UInt32]);
      else
        strcat(String, ".");
    }
    strcat(String, "\r");
    uart_send(__LINE__, __func__, String);
    sleep_ms(10);  // prevent communication override.
  }
  uart_send(__LINE__, __func__, "===========================================================================================\r\r");

  return;
}





/* $PAGE */
/* $TITLE=wait_ms() */
/* ============================================================================================================================================================= *\
                                                                  Pause for specified number of msec.
                       NOTE: This function does not use a sleep_xx(), so it can be used inside a hardware or software interrupt (callback).
\* ============================================================================================================================================================= */
void static wait_ms(UINT16 WaitMSec)
{
  UINT64 TimeStamp;


  TimeStamp = time_us_64();
  while (time_us_64() < (TimeStamp + (WaitMSec * 1000ll)));

  return;
}
