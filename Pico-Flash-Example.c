/* ================================================================================================================================================================= *\
   Pico-Flash-Example.c
   St-Louys Andre - August 2024
   Revision 02-SEP-2024
   Langage: Linux gcc
   Version 1.00

   REVISION HISTORY:
   =================
   31-AUG-2024 1.00 - Initial release.

\* ================================================================================================================================================================= */


/* ================================================================================================================================================================= *\
      This program does not perform any useful action. It has been created only to show how to use the Pico-Flash-Driver library with one of your own program.

                                                                            HOW TO USE
                                                                         ================
       Create a structure containing all data that we want to save to Pico's flash memory. One flash sector (0x1000 or 4096 bytes) must be written at a time.
      So, it is a good practice to allocate a full sector (0x1000 bytes) as the structure size with placeholders (dummies) for unused variables. If more data
     is required later, some placeholders are simply replaced with actual new variables. If you want to save more than 4096 bytes, the easier way is to create
    another structure (also 4096 bytes) that will be saved in flash at another offset (location). Since the **programs** are written to the beginning of flash,
   the offset to save data to flash (in Pico-Flash-Driver) are selected beginning at the complete end of flash with an offset going backward if we need to save 
      more than one sector. This way, the program grows in size from the beginning to then end, while the data saved grows from the end toward the beginning.
                                                  Obviously, there will be a problem if ever both overlap !

   The very last UINT16 of the structure must be reserved to contain the checksum (CRC16) of the whole structure (excluding the CRC itself), This way, a program
      may recalculate the checksum from time to time and if the checksum has changed (some data in the structure has been updated), then the new data is saved.

   Be aware that there is no "wear levelling" algorithm in the Pico-Flash-Driver. Pico's flash is said to be able to withstand 100,000 flash write cycles. It may
   be more than enough for must uses, but do not use algorithms that are performing "a few writes every seconds" or you may rapidly reach the end-of-life of your
                                                                                 Pico.

   I used the word "driver" in the name of the program "Pico-Flash-Driver". Of course, this programs does not correspond to the usual definition of what a "driver"
   is. I could have used the word "Utility", "Support", "Helper", or whatever, but I decide to use "Driver". It simply means that it represents code that could be
   used with your "main" program to help you implement some kind of side support. I'm also using other such "drivers" that I could eventually publich on GitHub.

   Function uart_send() must be copied to your own program, or all calls to uart_send() must be replaced by printf() statements.
\* ================================================================================================================================================================= */



/* $TITLE=Included files. */
/* $PAGE */
/* ================================================================================================================================================================= *\
                                                                           Include files.
\* ================================================================================================================================================================= */
#include "baseline.h"
#include "pico/bootrom.h"
#include "Pico-Flash-Driver.h"



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
#define RELEASE_VERSION



#ifdef RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // must remain OFF at all times.
#else   // RELEASE_VERSION
  UINT8 FlagLocalDebug = FLAG_OFF;  // may be modified for debug purposes.
#endif  // RELEASE_VERSION

/* Structure containing data to be saved to Pico's flash (non volatile) memory. This is only to give an idea of what can be saved. */
struct flash_data
{
  UCHAR  Version[10];              // for example, Firmware Version number.
  UCHAR  NetworkName[40];          // SSID or network name.
  UCHAR  NetworkPassword[72];      // network password.
  UINT8  Variable8[100];           // variables to be saved ( 8 bits each).
  UINT16 Variable16[100];          // variables to be saved (16 bits each).
  UINT32 Variable32[100];          // variables to be saved (32 bits each).
  UINT64 Variable64[100];          // variables to be saved (64 bits each).
  UINT8  Reserved[2464];           // placeholders (dummy) variables, only to make the whole structure size "one sector" (0x1000 or 4096 bytes).
  UINT16 Crc16;                    // checksum computed on all structure content, except the CRC itself.
} FlashData;



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

  UINT8 *Dum1Ptr8;

  UINT16 Crc16;
  UINT16 Loop1UInt16;

  UINT32 Length;
  UINT32 Offset;


  stdio_init_all();


  /* Wipe FlashData structure on entry. */
  Dum1Ptr8 = (UINT8 *)&FlashData;
  for (Loop1UInt16 = 0; Loop1UInt16 < sizeof (struct flash_data); ++Loop1UInt16)
    Dum1Ptr8[Loop1UInt16] = 0xFF;


  /* Give some time for user to start terminal emulator program after powering up Pico. */
  sleep_ms(5000);


  /* Wait for CDC USB connection. */
  while (!stdio_usb_connected())
  {
    sleep_ms(200);
    ++Delay;
    if (Delay == 50) break;  // wait for CDC USB connection for 10 seconds.
  }



  /* Check if a CDC USB connection has been established. If not, user won't see any message from the application. */
  if (stdio_usb_connected())
    printf("         CDC USB connection has been established\r");



  while (1)
  {
    printf(" ========================================================\r");
    printf("                    Pico-Flash-Example\r");
    printf("             Size of FlashData: %u (0x%4.4X)\r", sizeof(struct flash_data), sizeof(struct flash_data));
    printf("    Address in RAM of structure FlashData: 0x%p\r", &FlashData);
    printf(" ========================================================\r\r");
    printf("           1) Display flash data.\r");
    printf("           2) Read variables from flash memory.\r");
    printf("           3) Display value of variables (in RAM).\r");
    printf("           4) Modify variables.\r");
    printf("           5) Save new variables to flash.\r");
    printf("           6) Wipe working flash memory area.\r");
    printf("           7) Toggle Pico into upload mode.\r\r");
    printf("\r");
    printf("              Enter your choice: ");
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
        printf("                   ===================\r\r");
        printf("In the Raspberry Pi Pico and PicoW, the 2MB flash memory goes from\r");
        printf("0x10000000 up to 0x101FFFFF. Those specifications are different for\r");
        printf("the new Raspberry Pi Pico 2 that has been announced in August 2024.\r\r");

        printf("With this simple example program, we will write by default only to\r");
        printf("flash memory offset 0x1FF000 up to the end of flash, offset 0x1FFFFF\r\r");

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
        printf("          ========================================\r\r");
        printf("This will read the variables that are currently in flash memory.\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          printf("Reading variables from flash memory\r");
          flash_read_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Variables have been read from flash...\r\r\r");
          sleep_ms(1000);
        }
        printf("\r\r");
      break;



      case (3):
        /* Display current values of variables in RAM. */
        printf("\r\r");
        printf("                        Display current values of variables in RAM.\r\r");
        printf("                        ==========================================\r\r");
        printf("                                          Version:\r");
        util_display_data(FlashData.Version, sizeof(FlashData.Version));
        printf("\r\r");

        printf("                                          NetworkName:\r");
        util_display_data(FlashData.NetworkName, sizeof(FlashData.NetworkName));
        printf("\r\r");

        printf("                                          NetworkPassword:\r");
        util_display_data(FlashData.NetworkPassword, sizeof(FlashData.NetworkPassword));
        printf("\r\r");
      break;



      case (4):
        /* Modify variables. */
        printf("\r\r");
        printf("                               Modify variables\r");
        printf("                               ================\r\r");
        printf("This is just an example to show how variables can be changed, than saved to flash.\r");

        printf("Current value for string variable <Version> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1), sizeof(FlashData.Version));
        printf("Enter new string value for variable <Version>: ");
        input_string(FlashData.Version);
        /* Since we can display all bytes making up variabel size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.Version); Loop1UInt16 < sizeof(FlashData.Version); ++Loop1UInt16)
          FlashData.Version[Loop1UInt16] = 0x00;


        printf("\r\r\r");
        printf("Current value for string variable <NetworkName> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1 + sizeof(FlashData.Version)), sizeof(FlashData.NetworkName));
        printf("Enter new string value for variable <NetworkName>: ");
        input_string(FlashData.NetworkName);
        /* Since we can display all bytes making up variabel size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.NetworkName); Loop1UInt16 < sizeof(FlashData.NetworkName); ++Loop1UInt16)
          FlashData.NetworkName[Loop1UInt16] = 0x00;


        printf("\r\r\r");
        printf("Current value for string variable <NetworkPassword> is:\r\r");
        util_display_data((UINT8 *)(XIP_BASE + FLASH_DATA_OFFSET1 + sizeof(FlashData.Version) + sizeof(FlashData.NetworkName)), sizeof(FlashData.NetworkName));
        printf("Enter new string value for variable <NetworkPassword>: ");
        input_string(FlashData.NetworkPassword);
        /* Since we can display all bytes making up variabel size, clean all bytes passed end-of-string. */
        for (Loop1UInt16 = strlen(FlashData.NetworkPassword); Loop1UInt16 < sizeof(FlashData.NetworkPassword); ++Loop1UInt16)
          FlashData.NetworkPassword[Loop1UInt16] = 0x00;

        printf("\r\r\r");
      break;



      case (5):
        /* Save variables to flash. */
        printf("\r\r");
        printf("            Save variables to Pico's flash memory.\r");
        printf("            ======================================\r");
        printf("This will save the variables currently in RAM to flash memory.\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          printf("Saving variables to flash memory\r");
          flash_save_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Data has been saved to flash...\r");
          printf("See the CRC16 in the last 2 bytes ??\r\r\r");
          sleep_ms(1000);
        }
        printf("\r\r");
      break;



      case (6):
        /* Wipe working flash memory area. */
        printf("\r\r");
        printf("                      Wipe working flash memory area.\r");
        printf("                      ===============================\r");
        printf("We've been working in flash memory area from offset 0x1FF000 up to 0x1FFFFF.\r");
        printf("This will write back 0xFF all over this flash memory area.\r");
        printf("Press <G> to proceed: ");
        input_string(String);
        if ((String[0] == 'G') || (String[0] == 'g'))
        {
          /***
          printf("FLASH_PAGE_SIZE:           %u\r",   FLASH_PAGE_SIZE);
          printf("FLASH_SECTOR_SIZE:         %u\r",   FLASH_SECTOR_SIZE);
          printf("sizeof(struct flash_data): %u\r",   sizeof(struct flash_data));
          printf("Dum1Ptr8:                0x%p\r",   &FlashData);
          printf("FLASH_DATA_OFFSET1:      0x%X\r\r", FLASH_DATA_OFFSET1);
          ***/
          /* NOTE: flash_erase(FLASH_DATA_OFFSET1) may also be used to more easily erase one page of flash. */
          Dum1Ptr8 = (UINT8 *)&FlashData;
          for (Loop1UInt16 = 0; Loop1UInt16 < FLASH_SECTOR_SIZE; ++Loop1UInt16)
            Dum1Ptr8[Loop1UInt16] = 0xFF;
          printf("Erasing flash memory area from offset 0x1FF000 up to offset 0x1FFFFF\r");
          flash_save_data(FLASH_DATA_OFFSET1, (UINT8 *)&FlashData, sizeof(struct flash_data));
          printf("Working flash memory area has been returned to 0xFF...\r\r\r");
          sleep_ms(1000);
        }
        printf("\r\r");
      break;



      case (7):
        /* Toggle Pico into upload mode. */
        printf("\r\r");
        printf("Toggle Pico into upload mode.\r");
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
