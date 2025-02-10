/* ================================================================================================================================================================= *\
   Pico-Flash-Example.c
   St-Louys Andre - August 2024
   Revision 09-FEB-2025
   Langage: Linux gcc
   Version 2.00

   REVISION HISTORY:
   =================
   31-AUG-2024 1.00 - Initial release.
   09-FEB-2025 2.00 - Some cleanup and optimizations.
\* ================================================================================================================================================================= */


/* ================================================================================================================================================================= *\
            This program does not perform any useful action. It has been created only to show how to use the Pico-Flash-Module with one of your own program.
                Basically, refer to Pico-Flash-Module.c to see how you can integrate flash reading and writing in your own C-language program / project.
                    You should also refer to the User Guide for more details and the discussion group in the GitHub repository for Pico-Flash-Module.

                                                                            HOW TO USE
                                                                         ================
      Create a structure containing all data that you want to save to Pico's flash memory. One flash sector (0x1000 or 4096 bytes) must be written at a time.
      So, it is a good practice to allocate a full sector (0x1000 bytes) as the structure size with placeholders (dummies) for unused variables. If more data
     is required later, some placeholders are simply replaced with actual new variables. If you want to save more than 4096 bytes, the easier way is to create
    another structure (also 4096 bytes) that will be saved in flash at another offset (location). Since the **programs** are written to the beginning of flash,
   (and growing in size toward the end of flash memory as the program is updated), the offset to save data to flash (in Pico-Flash-Module) is selected beginning
   at the complete end of flash with an offset going backward if we need to save more than one sector. This way, the program grows in size from the beginning to
     then end, while the data saved grows from the end toward the beginning. Obviously, there will be a problem if ever both (program and saved data) overlap !

   The very last UINT16 of the structure must be reserved to contain the checksum (CRC16) of the whole structure (excluding the CRC itself), This way, a program
      may recalculate the checksum from time to time and if the checksum has changed (some data in the structure has been updated), then the new data is saved.

   Be aware that there is no "wear levelling" algorithm in the Pico-Flash-Module. Pico's flash is said to be able to withstand 100,000 flash write cycles. It may
   be more than enough for must uses, but do not use algorithms that are performing "a few writes every seconds" or you may rapidly reach the end-of-life of your
                                                                                 Pico.

   Function uart_send() must be copied to your own program, or all calls to uart_send() must be replaced by printf() statements.
\* ================================================================================================================================================================= */



/* $TITLE=Included files. */
/* $PAGE */
/* ================================================================================================================================================================= *\
                                                                           Include files.
\* ================================================================================================================================================================= */
#include "baseline.h"
#include "pico/bootrom.h"
#include "Pico-Flash-Module.h"
#include "stdarg.h"



/* $TITLE=Function definitions. */
/* $PAGE */
/* ================================================================================================================================================================= *\
                                                                       Function definitions.
\* ================================================================================================================================================================= */
/* Display first variables from flash memory. */
void display_variables(void);

/* Read a string from stdin. */
void input_string(UCHAR *String);

/* Send a string to VT101 monitor through Pico's UART or CDC USB. */
void uart_send(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...);



/* $TITLE=Global variables and definitions. */
/* $PAGE */
/* ================================================================================================================================================================= *\
                                                                     Global variables and defines.
\* ================================================================================================================================================================= */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------- *\
                   Structure containing data to be saved to Pico's flash (non volatile) memory. This is only to give an idea of what can be saved.
        The size of this structure must not be bigger than 4096 bytes (0X1000 HEX). The electronic of the Pico requires that one full flash sector (4096 bytes)
      be erased and reprogrammed at a time (no more, no less). Pico-Flash-Module algorithm will read a full sector and partly overwrite it if the data to save is
    smaller than 4096 bytes. However, an error message will be displayed (if an external terminal is connected to the Pico) if the host program tries to save data
    larger than 4096 bytes. If more data need to be saved, the host program must create another structure (smaller or equal to 4096 bytes) and use a flash offset
                                 below the first sector. There are already 10 such offset defined in the Pico-Flash-Module.h include file.
                                                           They go from FLASH_DATA_OFFSET1 up to FLASH_DATA_OFFSET10
\* ----------------------------------------------------------------------------------------------------------------------------------------------------------------- */
struct flash_data
{
  UCHAR  Version[12];              // for example, Firmware Version number.
  UCHAR  NetworkName[40];          // SSID or network name.
  UCHAR  NetworkPassword[72];      // network password.
  /// UINT8  Variable8[120];           // variables to be saved ( 8 bits each).
  /// UINT16 Variable16[100];          // variables to be saved (16 bits each).
  /// UINT32 Variable32[100];          // variables to be saved (32 bits each).
  /*****
   * UINT16 Crc16 MUST ALWAYS BE THE LAST MEMBER OF THE STRUCTURE. 
   * The CRC is computed on all structure content, except the CRC itself.
   * Make sure that byte alignment has no impact on Crc16's address.
  *****/
  UINT16 Crc16;
} FlashData;



#define RELEASE_VERSION

#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#warning ===============> Pico-Flash-Example built as RELEASE VERSION.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#warning ===============> Pico-Flash-Example built as DEVELOPER VERSION.
#endif  // RELEASE_VERSION

/* $PAGE */
/* $TITLE=Main program entry point. */
/* ============================================================================================================================================================= *\
                                                                          Main program entry point.
\* ============================================================================================================================================================= */
INT main()
{
  UCHAR String[256];

  UINT8 Delay;
  UINT8 FlashResult;
  UINT8 Menu;

  UINT8 *FlashDataPtr;

  UINT16 Crc16;
  UINT16 Loop1UInt16;

  UINT32 Length;
  UINT32 Offset;

  stdio_init_all();


  /* Wipe FlashData structure on entry. */
  FlashDataPtr = (UINT8 *)&FlashData;
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof (struct flash_data); ++Loop1UInt16)
    FlashDataPtr[Loop1UInt16] = 0xFF;


  /* 
  Give some time for user to start terminal emulator program after powering up Pico. Wait for USB CDC connection.
  Note: Since the purpose of this program is to show the internal workings of Pico-Flash-Module, we wait until a terminal has been connected,
        so that we can navigate through the menu and see how things are going.
  */
  while (!stdio_usb_connected())
  {
    sleep_ms(200);
    ++Delay;
    /// if (Delay == 150) break;  // wait for USB CDC connection for 30 seconds.
  }



  /* Check if a USB CDC connection has been established. If not, user won't see any message from the application. */
  if (stdio_usb_connected()) printf("            CDC USB connection has been established\r");



  /* Start Firmware's endless loop. */
  while (1)
  {
    printf("==================================================================\r");
    printf("                        Pico-Flash-Example\r");
    printf("   Pico's RAM memory area goes from 0x20000000 up to 0x2003FFFF\r");
    printf("              (plus a 2Kb stack area for each core).\r");
    printf("  Pico's flash memory area goes from 0x10000000 up to 0x101FFFFF\r");
    printf("      Size of flash sector used for this demo: %u (0x%4.4X)\r", sizeof(struct flash_data), sizeof(struct flash_data));
    printf("       Address in RAM of structure <FlashData>: 0x%p\r", &FlashData);
    printf("==================================================================\r\r");
    printf("          1) Display a specific area of flash memory.\r");
    printf("          2) Read variables from flash memory to RAM.\r");
    printf("          3) Display current value of RAM variables.\r");
    printf("          4) Modify RAM variables.\r");
    printf("          5) Save current RAM variables to flash.\r");
    printf("          6) Wipe target sector of flash memory area.\r");
    printf("          7) Display technical information.\r");
    printf("          8) Toggle Pico into upload mode.\r\r\r");
    printf("                  Enter your choice: ");
    input_string(String);

    /* If user pressed <Enter> only or <ESC>, loop back displaying menu. */
    if ((String[0] == 0x0D) || (String[0] == 0x1B)) continue;

    /* User pressed a menu option, execute it. */
    Menu = atoi(String);


    switch(Menu)
    {
      case (1):
        /* Display flash data. */
        printf("\r\r");
        printf("                   Display flash data.\r");
        printf("                  =====================\r\r");
        printf("In the Raspberry Pi Pico and PicoW, the 2MB flash memory goes from\r");
        printf("0x10000000 up to 0x101FFFFF. Those specifications are different for\r");
        printf("the new Raspberry Pi Pico 2 that has been announced in August 2024.\r\r");

        printf("With this simple example program, we will write by default only to\r");
        printf("the last sector of the flash memory area, at offset 0x1FF000\r");

        printf("However, this menu selection allows you to display any part of the\r");
        printf("flash memory, not only from offset 0x1FF000 to 0x1FFFFF\r\r");

        printf("Enter the offset in Pico's flash memory from which you want to display.\r");
        printf("Offset must be in the range of the 2MB flash (from 0x000000 to 0x1FFFFF)\r\r");

        printf("Enter offset to start flash memory display in hex (ex: 0x1FF000): ");
        input_string(String);
        Offset = strtol(String, NULL, 16);

        printf("\r");
        printf("Enter the length of the flash memory that you want to display.\r");
        printf("Length must be speficied in hex (ex: 0x1000): ");
        input_string(String);
        Length = strtol(String, NULL, 16);
        
        printf("\r\r\r");
        util_display_data((UINT8 *)(XIP_BASE + Offset), Length);
        printf("\r\r");
      break;



      case (2):
        /* Read variables from flash memory. */
        printf("\r\r");
        printf("          Read variables from Pico's flash memory.\r");
        printf("         ==========================================\r\r");
        printf("This will read the variables defined in the code with their content currently saved in flash memory at offset 0x%lX.\r", FLASH_DATA_OFFSET1);
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          printf("Reading variables from flash memory\r");
          flash_read_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Variables have been read from flash...\r\r\r");
        }
        else
        {
          printf("Operation aborted...\r\r");
        }
        printf("\r\r");
      break;



      case (3):
        /* Display current values of variables in RAM. */
        printf("\r\r");
        printf("                        Display current values of variables in RAM.\r\r");
        printf("                       ============================================\r\r");
        printf("                                          Variable: <Version> (in hex, then in ASCII):\r");
        printf("                                          Address in RAM: 0x%p\r", &FlashData.Version);
        util_display_data(FlashData.Version, sizeof(FlashData.Version));
        printf("\r");

        printf("                                          Variable <NetworkName> (in hex, then in ASCII):\r");
        printf("                                          Address in RAM: 0x%p\r", FlashData.NetworkName);
        util_display_data(FlashData.NetworkName, sizeof(FlashData.NetworkName));
        printf("\r");

        printf("                                          Variable <NetworkPassword> (in hex, then in ASCII):\r");
        printf("                                          Address in RAM: 0x%p\r", FlashData.NetworkPassword);
        util_display_data(FlashData.NetworkPassword, sizeof(FlashData.NetworkPassword));
        printf("\r");

        printf("                                          Variable <Crc16> (in hex, then in ASCII):\r");
        printf("                                          Address in RAM: 0x%p\r", &FlashData.Crc16);
        util_display_data((UCHAR *)&FlashData.Crc16, sizeof(FlashData.Crc16));
        printf("\r");
        break;



      case (4):
        /* Modify variables. */
        printf("\r\r");
        printf("                               Modify variables\r");
        printf("                              ==================\r\r");
        printf("This is just an example to show how variables can be changed, than saved to flash.\r");

        printf("Current value for string variable <Version> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1), sizeof(FlashData.Version));
        printf("Enter new string value for variable <Version> (max %u characters)\r", sizeof(FlashData.Version));
        printf("or simply <Enter> for no change: ");
        input_string(String);
        if (String[0] != 0x0D) strcpy(FlashData.Version, String);
        /* Since we can display all bytes making up variabel size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.Version); Loop1UInt16 < sizeof(FlashData.Version); ++Loop1UInt16)
          FlashData.Version[Loop1UInt16] = 0x00;


        printf("\r\r\r");
        printf("Current value for string variable <NetworkName> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1 + sizeof(FlashData.Version)), sizeof(FlashData.NetworkName));
        printf("Enter new string value for variable <NetworkName> (max %u characters)\r", sizeof(FlashData.NetworkName));
        printf("or simply <Enter> for no change: ");
        input_string(String);
        if (String[0] != 0x0D) strcpy(FlashData.NetworkName, String);
        /* Since we can display all bytes making up variable size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.NetworkName); Loop1UInt16 < sizeof(FlashData.NetworkName); ++Loop1UInt16)
          FlashData.NetworkName[Loop1UInt16] = 0x00;


        printf("\r\r\r");
        printf("Current value for string variable <NetworkPassword> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1 + sizeof(FlashData.Version) + sizeof(FlashData.NetworkName)), sizeof(FlashData.NetworkName));
        printf("Enter new string value for variable <NetworkPassword> (max %u characters)\r", sizeof(FlashData.NetworkPassword));
        printf("or simply <Enter> for no change: ");
        input_string(String);
        if (String[0] != 0x0D) strcpy(FlashData.NetworkPassword, String);
        /* Since we can display all bytes making up variable size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.NetworkPassword); Loop1UInt16 < sizeof(FlashData.NetworkPassword); ++Loop1UInt16)
          FlashData.NetworkPassword[Loop1UInt16] = 0x00;

        printf("\r\r\r");
      break;



      case (5):
        /* Save variables to flash. */
        printf("\r\r");
        printf("            Save variables to Pico's flash memory.\r");
        printf("           ========================================\r\r");
        printf("This will save the variables currently in RAM to flash memory.\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          printf("Saving variables to flash memory\r");
          flash_save_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Data has been saved to flash...\r");
        }
        else
        {
          printf("Operation aborted...\r\r");
        }
        printf("\r\r");
      break;



      case (6):
        /* Wipe working flash memory area. */
        printf("\r\r");
        printf("                Wipe target flash memory area sector.\r");
        printf("               =======================================\r\r");
        printf("We've been working in flash memory area from offset 0x1FF000 up to 0x1FFFFF.\r");
        printf("This will write back 0xFF all over this flash sector.\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          /* NOTE: flash_range_erase() may also be used to more easily erase flash memory instead of "writing" 0xFF's. */
          for (Loop1UInt16 = 0; Loop1UInt16 < sizeof(FlashData); ++Loop1UInt16)
            FlashDataPtr[Loop1UInt16] = 0xFF;
          printf("Erasing flash memory area from offset 0x1FF000 up to offset 0x1FFFFF\r");
          sleep_ms(100);
          flash_save_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Working flash memory area has been returned to 0xFF...\r\r\r");
          sleep_ms(1000);
        }
        else
        {
          printf("Operation aborted...\r\r");
        }
        printf("\r\r");
      break;



      case (7):
        /* Display technical information. */
        printf("\r\r");
        printf("Size of structure FlashData):               %4u (0x%X)\r",  sizeof(FlashData), sizeof(FlashData));
        printf("Address of structure FlashData:       0x%p                     [%X]\r", &FlashData, (XIP_BASE + FLASH_DATA_OFFSET1));
        printf("Address of FlashData.Version:         0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.Version,         ((UINT32)FlashData.Version -         (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.Version -         (UINT32)&FlashData)));
        printf("Address of FlashData.NetworkName:     0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.NetworkName,     ((UINT32)FlashData.NetworkName -     (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.NetworkName -     (UINT32)&FlashData)));
        printf("Address of FlashData.NetworkPassword: 0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.NetworkPassword, ((UINT32)FlashData.NetworkPassword - (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.NetworkPassword - (UINT32)&FlashData)));
        /// printf("Address of FlashData.Variable8:       0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.Variable8,       ((UINT32)FlashData.Variable8 -       (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.Variable8 -       (UINT32)&FlashData)));
        /// printf("Address of FlashData.Variable16:      0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.Variable16,      ((UINT32)FlashData.Variable16 -      (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.Variable16 -      (UINT32)&FlashData)));
        /// printf("Address of FlashData.Variable32:      0x%p (offset: 0x%4.4X)    [%X]\r",  FlashData.Variable32,      ((UINT32)FlashData.Variable32 -      (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)FlashData.Variable32 -      (UINT32)&FlashData)));
        printf("Address of FlashData.Crc16:           0x%p (offset: 0x%4.4X)    [%X]\r", &FlashData.Crc16,           ((UINT32)&FlashData.Crc16 -          (UINT32)&FlashData), (((UINT32)XIP_BASE + (UINT32)FLASH_DATA_OFFSET1) + ((UINT32)&FlashData.Crc16 -          (UINT32)&FlashData)));
        printf("Flash memory base address:            0x%X\r",        XIP_BASE);
        printf("FLASH_DATA_OFFSET1:                     0x%X\r",      FLASH_DATA_OFFSET1);
        printf("FLASH_DATA_OFFSET2:                     0x%X\r",      FLASH_DATA_OFFSET2);
        printf("FLASH_DATA_OFFSET3:                     0x%X\r",      FLASH_DATA_OFFSET3);
        printf("FLASH_DATA_OFFSET4:                     0x%X\r",      FLASH_DATA_OFFSET4);
        printf("FLASH_DATA_OFFSET5:                     0x%X\r",      FLASH_DATA_OFFSET5);
        printf("FLASH_PAGE_SIZE:                            %4u\r",   FLASH_PAGE_SIZE);
        printf("FLASH_SECTOR_SIZE:                          %4u\r\r", FLASH_SECTOR_SIZE);
    break;



      case (8):
        /* Toggle Pico into upload mode. */
        printf("\r\r");
        printf("          Toggle Pico into upload mode.\r");
        printf("         ===============================\r\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g')) reset_usb_boot(0l, 0l);
        printf("\r\r");
      break;



      default:
        printf("\r\r");
        printf("                    Invalid choice... please re-enter [%s]  [%u]\r\r\r\r\r", String, Menu);
        printf("\r\r");
      break;
    }
  }

  return 0;
}





/* $PAGE */
/* $TITLE=input_string() */
/* ============================================================================================================================================================= *\
                                                                         Read a string from stdin.
\* ============================================================================================================================================================= */
void input_string(UCHAR *String)
{
  INT8 DataInput;

  UINT8 Loop1UInt8;

  UINT32 IdleTimer;


  if (FlagLocalDebug) printf("Entering input_string()\r");

  Loop1UInt8 = 0;
  IdleTimer  = time_us_32();  // initialize time-out timer with current system timer.
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
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 > 0)
        {
          --Loop1UInt8;
          String[Loop1UInt8] = 0x00;
          printf("%c %c", 0x08, 0x08);  // erase character under the cursor.
        }
      break;

      case (27):
        /* <ESC> */
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      case (0x0D):
        /* <Enter> */
        IdleTimer = time_us_32();  // restart time-out timer.
        if (Loop1UInt8 == 0)
        {
          String[Loop1UInt8++] = (UCHAR)DataInput;
          String[Loop1UInt8++] = 0x00;
        }
        printf("\r");
      break;

      default:
        IdleTimer = time_us_32();  // restart time-out timer.
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

  if (FlagLocalDebug) printf("Exiting input_string()\r");

  return;
}





/* $PAGE */
/* $TITLE=uart_send() */
/* ============================================================================================================================================================= *\
                                                     Send a string to VT101 monitor through Pico's UART or CDC USB.
\* ============================================================================================================================================================= */
void uart_send(UINT LineNumber, const UCHAR *FunctionName, UCHAR *Format, ...)
{
  UCHAR Dum1Str[256];
  UCHAR TimeStamp[128];

  UINT Loop1UInt;
  UINT StartChar;

  va_list argp;


  /* Transfer the text to print to variable Dum1Str */
  va_start(argp, Format);
  vsnprintf(Dum1Str, sizeof(Dum1Str), Format, argp);
  va_end(argp);

  /* Trap special control code for <HOME>. Replace "home" by appropriate control characters for "Home" on a VT101. */
  if (strcmp(Dum1Str, "home") == 0)
  {
    Dum1Str[0] = 0x1B; // ESC code
    Dum1Str[1] = '[';
    Dum1Str[2] = 'H';
    Dum1Str[3] = 0x00;
  }

  /* Trap special control code for <CLS>. Replace "cls" by appropriate control characters for "Clear screen" on a VT101. */
  if (strcmp(Dum1Str, "cls") == 0)
  {
    Dum1Str[0] = 0x1B; // ESC code
    Dum1Str[1] = '[';
    Dum1Str[2] = '2';
    Dum1Str[3] = 'J';
    Dum1Str[4] = 0x00;
  }

  /* Time stamp will not be printed if first character is a '-' (for title line when starting debug, for example),
     or if first character is a line feed '\r' when we simply want add line spacing in the debug log,
     or if first character is the beginning of a control stream (for example 'Home' or "Clear screen'). */
  if ((Dum1Str[0] != '-') && (Dum1Str[0] != '\r') && (Dum1Str[0] != 0x1B) && (Dum1Str[0] != '|'))
  {
    /* Send line number through UART. */
    printf("[%7u] - ", LineNumber);

    /* Display function name. */
    printf("[%s]", FunctionName);
    for (Loop1UInt = strlen(FunctionName); Loop1UInt < 25; ++Loop1UInt)
    {
      printf(" ");
    }
    printf("- ");
  }

  /* Send string through UART. */
  printf(Dum1Str);

  return;
}
