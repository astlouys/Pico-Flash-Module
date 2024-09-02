#ifndef _BASELINE_H_
#define _BASELINE_H_

#include "stdint.h"

/* --------------------------------------------------------------------------------------------------------------------------- *\
                                                      General definitions.
\* --------------------------------------------------------------------------------------------------------------------------- */
typedef char          CHAR;
typedef unsigned char UCHAR;
typedef int           INT;    // processor-optimized.
typedef int8_t        INT8;
typedef int16_t       INT16;
typedef int32_t       INT32;
typedef int64_t       INT64;
typedef unsigned int  UINT;    // processor-optimized.
typedef uint8_t       UINT8;
typedef uint16_t      UINT16;
typedef uint32_t      UINT32;
typedef uint64_t      UINT64;

#define FLAG_OFF     0x00
#define FLAG_ON      0x01

#define FALSE        0
#define TRUE         1

#endif  // _BASELINE_H_

