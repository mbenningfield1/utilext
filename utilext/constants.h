/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Simple header file for constants. Only lawyers could imagine that someone
 * could copyright a list of numbers in the first place.
 *
 *============================================================================*/
#pragma once

#define MIN_WINDOW_VERSION 3025000

#define	RESULT_OK           0
#define	RESULT_NULL        -1
#define	ERR_NOMEM		        SQLITE_NOMEM
#define	ERR_LIKE_PATTERN	  SQLITE_TOOBIG
#define	ERR_INDEX		        SQLITE_RANGE
#define	ERR_REGEX_PARSE		  SQLITE_ERROR
#define	ERR_REGEX_TIMEOUT   SQLITE_ABORT
#define	ERR_DECIMAL_PARSE	  SQLITE_FORMAT
#define	ERR_DECIMAL_OVFLOW  SQLITE_TOOBIG
#define	ERR_DECIMAL_DIVZ    SQLITE_ERROR
#define	ERR_DECIMAL_MODE    SQLITE_NOTFOUND
#define	ERR_DECIMAL_PREC    SQLITE_MISUSE
#define ERR_DECIMAL_NAN     SQLITE_ABORT
#define	ERR_AGGREGATE      -2 /* internal */
#define	ERR_ESC_LENGTH      SQLITE_MISUSE
#define	ERR_TIME_OVFLOW     SQLITE_TOOBIG
#define	ERR_TIME_JD_RANGE   SQLITE_ERROR
#define	ERR_TIME_PARSE      SQLITE_FORMAT
#define	ERR_TIME_UNIX_RANGE SQLITE_ERROR
#define ERR_TIME_INVALID    SQLITE_RANGE
#define ERR_CULTURE         SQLITE_NOTFOUND
#define ERR_BIGINT_PARSE    SQLITE_FORMAT
#define ERR_BIGINT_OVFLOW   SQLITE_TOOBIG
#define ERR_BIGINT_DIVZ     SQLITE_ERROR
#define ERR_BIGINT_RANGE    SQLITE_RANGE

/* Rounding modes for decimal functions */
#define ROUND_EVEN     0
#define ROUND_NORMAL   1

/* Limits used in our overflow-checked addition and subtraction of 64-bit
** integers; should never be true on any Windows system after Windows98, but
** just because you're paranoid doesn't mean someone isn't out to get you :)
*/
#ifndef LLONG_MAX
#include <limits.h>
#endif
#if LLONG_MAX == INT_MAX
#define LLONG_MAX 0X7FFFFFFFFFFFFFFF
#define LLONG_MIN 0X8000000000000000
#endif

/* User data for encoding-dependent functions; cast to void* for function
** context; for user data that needs to be encoding and case dependent, we
** combine with a flag to signal case-insensitive.
*/
#define UTF8_ENC       0
#define UTF16_ENC      1
#define NOCASE         2

/* We have 4 like overloads, and 2 set_like_case overloads */
#define LIKE_STATE_CNT 6

/* include our "babyints" header */
typedef char               i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;