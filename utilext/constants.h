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

#define	RESULT_OK            0
#define	RESULT_NULL          1
#define	ERR_NOMEM		         2 /* SQLITE_NOMEM    */
#define	ERR_LIKE_PATTERN	   3 /* SQLITE_TOOBIG   */
#define	ERR_INDEX		         4 /* SQLITE_RANGE    */
#define	ERR_REGEX_PARSE		   5 /* SQLITE_ERROR    */
#define	ERR_REGEX_TIMEOUT    6 /* SQLITE_ABORT    */
#define	ERR_DECIMAL_PARSE	   7 /* SQLITE_FORMAT   */
#define	ERR_DECIMAL_OVFLOW   8 /* SQLITE_TOOBIG   */
#define	ERR_DECIMAL_DIVZ     9 /* SQLITE_ERROR    */
#define	ERR_DECIMAL_MODE    10 /* SQLITE_NOTFOUND */
#define	ERR_DECIMAL_PREC    11 /* SQLITE_MISUSE   */
#define	ERR_AGGREGATE       12 /* internal        */
#define	ERR_ESC_LENGTH      13 /* SQLITE_MISUSE   */
#define	ERR_TIME_OVFLOW     14 /* SQLITE_TOOBIG   */
#define	ERR_TIME_JD_RANGE   15 /* SQLITE_ERROR    */
#define	ERR_TIME_PARSE      16 /* SQLITE_FORMAT   */
#define	ERR_TIME_UNIX_RANGE 17 /* SQLITE_ERROR    */
#define ERR_TIME_INVALID    18 /* SQLITE_RANGE    */
#define ERR_CULTURE         19 /* SQLITE_NOTFOUND */
#define ERR_BIGINT_PARSE    20 /* SQLITE_FORMAT   */
#define ERR_BIGINT_OVFLOW   21 /* SQLITE_TOOBIG   */
#define ERR_BIGINT_DIVZ     22 /* SQLITE_ERROR    */
#define ERR_BIGINT_RANGE    23 /* SQLITE_RANGE    */

#define ROUND_EVEN     0
#define ROUND_NORMAL   1

#define TIMETYPE_UNIX   1 /* SQLITE_INTEGER */
#define TIMETYPE_JULIAN 2 /* SQLITE_FLOAT   */
#define TIMETYPE_ISO    3 /* SQLITE_TEXT    */

/* include our "babyints" header */
typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;