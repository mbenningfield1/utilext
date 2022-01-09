/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Header file for defines and things that are common to the native entry point
 * functions.
 *
 * Contains the documentation comments for the README file that covers all of
 * the SQL extension functions.
 *
 *============================================================================*/

#pragma once

/* Notes in "utilext.c" */
#pragma warning( disable : 4820 )
#include "sqlite3ext.h"
#include "constants.h"

/* Define this symbol to add the SQLITE_DIRECT_ONLY flag to all of the user-
** defined functions, so that they can only be called from direct SQL. */
#ifdef UTILEXT_MAKE_DIRECT
#define FUNC_DIRECT SQLITE_DIRECTONLY
#else
#define FUNC_DIRECT 0
#endif

#define FUNC_FLAGS SQLITE_DETERMINISTIC | FUNC_DIRECT

/* For functions that return NULL on NULL arguments, simply return from the
** user function and the result will be set to NULL by SQLite. */
#define CHECK_ARGS_NULL(N) for (int ii = 0; ii < (N); ii++) {\
                             if (sqlite3_value_type(argv[ii]) == SQLITE_NULL) {\
                               return;\
                             }\
                           }

#ifdef _WIN64
#define PTR_TO_INT(X) (int)((i64)(X))
#else
#define PTR_TO_INT(X) (int)(X)
#endif

/* Cached mutex used to serialize access to the shared state of the like
** function case-sensitivity. If the threading mode is single or multi, then
** this is null */
extern sqlite3_mutex *LikeMutex;
extern bool Serialized;

#define NEED_MUTEX LikeMutex == nullptr && Serialized

/* Should the like function match BLOB arguments? */
extern bool MatchBlobs;

/* Native struct that represents a zero-terminated string from the database,
** encoded in UTF-8 or UTF-16.
*/
struct DbStr {
  const void *pText;  /* pointer to the string bytes       */
  int cb;             /* count of bytes in pText (less \0) */
  bool isWide;        /* true if encoding is UTF-16        */
};

/* Native struct that represents a heap-allocated array of UTF-8 strings that
** is used for the regsplit() table-valued SQL function.
*/
struct DbStrArr {
  char **pArr;  /* pointer to an array of encoded strings */
  int n;        /* number of strings in pArr              */
  bool isWide;  /* true if the strings are UTF-16         */
};

/* User data for like() and set_case_sensitive_like() functions. The shared
** pointer is a per-connection flag for whether or not like() is case-
** sensitive; the default is true (not case-sensitive). */
struct LikeState {
  bool *pNoCase;  /* shared pointer (default true) */
  bool enc16;     /* true if encoding is UTF16 */
};

/* Native struct that encapsulates a date value from SQLite, either as a Unix
** time, a Julian day, or an ISO-8601 string.
*/
struct DbDate {
  int type;        /* SQLITE_INTEGER, SQLITE_REAL, or SQLITE_TEXT */
  union {
    i64 unix;      /* Unix timestamp */
    double julian; /* Julian date */
    DbStr iso;     /* ISO-8601 date/time string */
  };
};

/* Aggregate context for the timespan total and avg functions */
struct SumCtx {
  i64 sum;
  u64 cnt;
  bool overflow;
};


/* Overflow-checked math for TimeSpans */
int util_addCheck64(i64 *lhs, i64 rhs);
int util_subCheck64(i64 *lhs, i64 rhs);

/* utilext helper prototypes */
const char *util_getAscii(sqlite3_value *value, int *pBytes);
void util_getText(sqlite3_value *value, bool isWide, DbStr *pStr);
void util_setText(sqlite3_context *pCtx, DbStr *pResult);
int util_getData(sqlite3_context *pCtx);
bool util_getEnc16(sqlite3_context *pCtx);
void util_setError(sqlite3_context *pCtx, int error);

/* Implements the util_capable() SQL function. [MISC]
** SQL Usage: util_capable(Z)
**
** Parameters -
**
**  Z - The name of a function category to check
**
** Returns true (non-zero) if the utilext library was compiled to provide the
** functions in the specified category, or false (zero) if not.
**
** Returns NULL if `Z` is NULL or not recognized.
**
** The possible values for `Z` are: 'string', 'decimal', 'bigint', 'regex',
** 'timespan', and 'like' (in any character casing).
*/
void util_option(sqlite3_context *, int, sqlite3_value**);

#if defined(UTILEXT_OMIT_STRING) && defined(UTILEXT_OMIT_DECIMAL) &&\
    defined(UTILEXT_OMIT_REGEX) && defined(UTILEXT_OMIT_BIGINT) &&\
    defined(UTILEXT_OMIT_TIME)
#error All extension functions have been omitted from build
#endif

/* If we're not doing strings, then don't do like override */
#ifdef UTILEXT_OMIT_STRING
#ifndef UTILEXT_OMIT_LIKE
#define UTILEXT_OMIT_LIKE
#endif
#endif


#ifndef UTILEXT_OMIT_BIGINT

/* Implements the bigint() SQL function
** SQL Usage: bigint(V)
**
** Parameters -
**
**  V - a valid numeric value in either TEXT, INTEGER, or REAL format
**
** Returns a hexadecimal big integer value equivalent to the input value.
**
** Returns NULL if `V` is NULL.
**
** Data in `TEXT` format is presumed to be a valid numeric string that
** represents an integer or floating-point value. Hexadecimal strings (with or
** without a leading "0x" prefix) are allowed for integer values. Note that
** integer conversion is attempted first. A valid hexadecimal string without a
** leading "0x" prefix and with no hex digits (A-F) will be converted to the
** decimal integer value represented.
**
** This is the only big integer function that permits a "0x" prefix on a
** hexadecimal string, so that the ambiguity described above can be avoided. All
** other big integer extension functions require a hexadecimal string value that
** does not include a "0x" prefix.
**
** REAL values are truncated when converting to a BigInteger. This is the only
** big integer function that permits floating-point values. All other
** big integer extension functions will fail with `SQLITE_FORMAT` if a
** floating-point argument is supplied.
**
** Data in BLOB format is not supported. If `V` is stored in BLOB format, an
** error is raised. All of the other big integer functions will attempt to
** resolve a BLOB value as a hexadecimal string.
**
** Errors -
**
**  SQLITE_FORMAT - V is a TEXT value and is not convertible to a numeric value
**  SQLITE_TOOBIG - V is a REAL value that is not representable, like NaN or ±Inf
**  SQLITE_MISUSE - V is a BLOB value
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintCtor(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_abs() SQL function
** SQL Usage: bigint_abs(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the absolute value of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintAbs(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_add() SQL function
** SQL Usage: bigint_add(V1, V2, ...)
**
** Parameters -
**
**  V1  - A big integer value
**  V2  - A big integer value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the sum of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are provided.
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid big
**                - integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintAdd(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_and() SQL function
** SQL Usage: bigint_and(V1, V2)
**
** Parameters -
**
**  V1  - A big integer value
**  V2  - A big integer value
**
** Returns the result of performing a bitwise AND of `V1` with `V2` ( V1 & V2 ).
**
** Returns NULL if either argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintAnd(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_avg() multi-argument SQL function
** SQL Usage: bigint_avg(V1, V2, ...)
**
** Parameters -
**
**  V1  - A big integer value
**  V2  - A big integer value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the average of all non-NULL arguments, rounded to the nearest
** big integer value. The rounding method used is simple rounding (away from 0
** for .5 or greater).
**
** Note that this function returns a big integer value. It does not return a
** `double` value, since any value large enough to justify the use of the
** big integer data type would significantly exceed the precision of a `double`,
** resulting in increasingly inaccurate results as the magnitude of the values
** increases.
**
** Returns '0' if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid big
**                - integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintAvgAny(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_avg() aggregate SQL function
** SQL Usage: bigint_avg(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the average of all non-NULL values in the group, rounded to the
** nearest big integer value. The rounding method used is simple rounding
** (away from 0 for .5 or greater).
**
** Note that this function returns a big integer value. It does not return a
** `double` value, since any value large enough to justify the use of the
** big integer data type would significantly exceed the precision of a `double`,
** resulting in increasingly inaccurate results as the magnitude of the values
** increases.
**
** This function will return '0' if there are only NULL values in the group.
** This behavior diverges from the SQLite `avg()` function.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid big
**                - integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintAvgStep(sqlite3_context*, int, sqlite3_value**);
void bintAvgFinal(sqlite3_context*);
void bintAvgInv(sqlite3_context*, int, sqlite3_value**);
void bintAvgValue(sqlite3_context*);

/* Implements the 'bigint' collation sequence
** SQL Usage: COLLATE BIGINT
**
** Sorts big integer data strictly by value.
**
** NULL values are sorted either first or last, depending on the sort order
** defined in the SQL query, such as `ASC NULLS LAST` or just `ASC`.
**
** Non-NULL values that are not valid big integer numbers always sort before
** valid values, so they will be first in ascending order and last in descending
** order. The sort order of invalid values is not guaranteed, but will always be
** the same for the same data. The best thing to do is to not have any invalid
** values in a 'bigint' column.
*/
int bintCollate(void*, int, const void*, int, const void*);

/* Implements the bigint_cmp() SQL function.
** SQL Usage: bigint_cmp(V1, V2)
**
** Parameters -
**
**  V1 - A big integer value
**  V2 - A big integer value
**
** Returns NULL if either argument is NULL. Otherwise, the return value is:
**
**  Comparison result - Return value
**  V1 > V2           -  1
**  V1 == V2          -  0
**  V1 < V2           - -1
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
*/
void bintCmp(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_div() SQL function.
** SQL Usage: bigint_div(V1, V2)
**
** Parameters -
**
**  V1 - A big integer value
**  V2 - A big integer value
**
** Returns the mathematical quotient of ( V1 / V2 ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_ERROR  - The result is division by zero
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintDiv(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_gcd() SQL function.
** SQL Usage: bigint_gcd(V1, V2)
**
** Parameters -
**
**  V1 - A big integer value
**  V2 - A big integer value
**
** Returns the greatest common divisor of `V1` and `V2`.
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintGcd(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_log() SQL function.
** SQL Usage: bigint_log(V)
**            bigint_log(V, B)
**
** Parameters -
**
**  V - A big integer value
**  B - A double base value
**
** Returns the base `B` logarithm of `V` as a double-precision floating-point
** value. If `B` is not specified, returns the natural (base e) logarithm of `V`.
**
** Returns NULL if any argument is NULL.
**
** Returns NULL if the logarithm result is "NaN" or "Infinity". This can happen
** for certain values of `V` and `B`. See the documentation for the
** `BigInteger.Log()` method for more information.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_RANGE  - The result exceeds the range of a double value
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintLog(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_log10() SQL function.
** SQL Usage: bigint_log10(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the base 10 (common) logarithm of `V` as a double-precision
** floating-point value.
**
** Returns NULL if `V` is NULL.
**
** Returns NULL if the logarithm result is "NaN" or "Infinity".
** This can happen if `V` is zero or negative.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_RANGE  - The result exceeds the range of a double value
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintLog10(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_lsh() SQL function.
** SQL Usage: bigint_lsh(V)
**            bigint_lsh(V, S)
**
** Parameters -
**
**  V - A big integer value
**  S - The integer number of bits to shift to the left
**
** Returns the value of `V` shifted to the left by `S` bits. If `S` is less than
** zero, `V` is right-shifted by `S` bits. ( V << S )
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintLShift(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_modpow() SQL function.
** SQL Usage: bigint_modpow(V, E, M)
**
** Parameters -
**
**  V - A big integer value
**  E - A big integer exponent
**  M - A big integer modulus
**
** Returns the remainder after dividing `V` raised to the power of `E` by
** `M`. ( (V ^ E) % M )
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - Any argument does not resolve to a valid big integer
**                - hexadecimal string
**  SQLITE_ERROR  - M is equal to zero
**  SQLITE_RANGE  - E is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintModPow(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_mult() SQL function.
** SQL Usage: bigint_mult(V1, V2, ...)
**
** Parameters -
**
**  V1  - A big integer value
**  V2  - A big integer value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the product of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid big
**                - integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintMult(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_neg() SQL function
** SQL Usage: bigint_neg(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns `V` with the sign reversed.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintNeg(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_not() SQL function
** SQL Usage: bigint_not(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the one's complement of `V` ( ~V ).
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintNot(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_or() SQL function
** SQL Usage: bigint_or(V1, V2)
**
** Parameters -
**
**  V1  - A big integer value
**  V2  - A big integer value
**
** Returns the result of performing a bitwise OR of `V1` with `V2` ( V1 | V2 ).
**
** Returns NULL if either argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintOr(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_pow() SQL function.
** SQL Usage: bigint_rsh(V, E)
**
** Parameters -
**
**  V - A big integer value
**  E - The positive integer exponent to use
**
** Returns the value of `V` raised to the power of `E`.
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_ERROR  - E is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintPow(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_rem() SQL function.
** SQL Usage: bigint_rem(V1, V2)
**
** Parameters -
**
**  V1 - A big integer value
**  V2 - A big integer value
**
** Returns the remainder after dividing `V1` by `V2` ( V1 % V2 )
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer
**                - hexadecimal string
**  SQLITE_ERROR  - V2 is equal to zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintRem(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_rsh() SQL function.
** SQL Usage: bigint_rsh(V)
**            bigint_rsh(V, S)
**
** Parameters -
**
**  V - A big integer value
**  S - The integer number of bits to shift to the right
**
** Returns the value of `V` shifted to the right by `S` bits. If `S` is less
** than zero, `V` is left-shifted by `S` bits. ( V >> S )
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintRShift(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_str() SQL function
** SQL Usage: bigint_str(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the decimal string representation of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid big integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintStr(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_sub() SQL function.
** SQL Usage: bigint_sub(V1, V2)
**
** Parameters -
**
**  V1 - A big integer value
**  V2 - A big integer value
**
** Returns the difference of ( V1 - V2 ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid big integer hexadecimal
**                - string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintSub(sqlite3_context*, int, sqlite3_value**);

/* Implements the bigint_total() aggregate SQL function
** SQL Usage: bigint_total(V)
**
** Parameters -
**
**  V - A big integer value
**
** Returns the sum of all non-NULL values in the group.
**
** This function will return '0' if there are only NULL values in the group.
** This behavior matches the SQLite `total()` function.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid big
**                - integer hexadecimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void bintTotStep(sqlite3_context*, int, sqlite3_value**);
void bintTotFinal(sqlite3_context*);
void bintTotInv(sqlite3_context*, int, sqlite3_value**);
void bintTotValue(sqlite3_context*);
#endif /* !UTILEXT_OMIT_BIGINT */

#ifndef UTILEXT_OMIT_DECIMAL

/* Implements the dec_abs() SQL function.
** SQL Usage: dec_abs(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the absolute value of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decAbsFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_add() SQL function
** SQL Usage: dec_add(V1, V2, ...)
**
** Parameters -
**
**  V1  - A decimal value
**  V2  - A decimal value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the sum of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid
**                - decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decAddFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_avg() multi-argument SQL function.
** SQL Usage: dec_avg(V1, V2, ...)
**
** Parameters -
**
**  V1  - A decimal value
**  V2  - A decimal value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the average of all non-NULL arguments.
**
** Returns '0.0' if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid
**                - decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decAvgAny(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_avg() aggregate SQL function.
** SQL Usage: dec_avg(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the average of all non-NULL values in the group.
**
** This function will return '0.0' if there are only NULL values in the group.
** This behavior diverges from the SQLite `avg()` function.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a decimal
**                - number
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid
**                - decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decAvgStep(sqlite3_context*, int, sqlite3_value**);
void decAvgFinal(sqlite3_context*); 
void decAvgInv(sqlite3_context*, int, sqlite3_value**);
void decAvgValue(sqlite3_context*);

/* Implements the dec_ceil() SQL function.
** SQL Usage: dec_ceil(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the smallest integer that is larger than or equal to `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decCeilFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_cmp() SQL function.
** SQL Usage: dec_cmp(V1, V2)
**
** Parameters -
**
**  V1 - A decimal value
**  V2 - A decimal value
**
** Returns NULL if either argument is NULL. Otherwise, the return value is:
**
**  Comparison result - Return value
**  V1 > V2           -  1
**  V1 == V2          -  0
**  V1 < V2           - -1
**
** Errors -
**
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid decimal string
*/
void decCmpFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the 'decimal' collation sequence
** SQL Usage: COLLATE DECIMAL
**
** Sorts decimal data strictly by value.
**
** NULL values are sorted either first or last, depending on the sort order
** defined in the SQL query, such as `ASC NULLS LAST` or just `ASC`.
**
** Non-NULL values that are not valid decimal numbers always sort before valid
** values, so they will be first in ascending order and last in descending
** order. The sort order of invalid values is not guaranteed, but will always be
** the same for the same data. The best thing to do is to not have any invalid
** values in a 'decimal' column.
*/
int decCollate(void*, int, const void*, int, const void*);

/* Implements the dec_div() SQL function.
** SQL Usage: dec_div(V1, V2)
**
** Parameters -
**
**  V1 - A decimal value
**  V2 - A decimal value
**
** Returns the mathematical quotient of ( V1 / V2 ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_ERROR  - The result is division by zero
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decDivFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_floor() SQL function.
** SQL Usage: dec_floor(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the smallest integer that is less than or equal to `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decFloorFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_log() SQL function.
** SQL Usage: dec_log(V)
**            dec_log(V, B)
**
** Parameters -
**
**   V - A decimal value
**   B - A double base value
**
** Returns the base `B` logarithm of `V` as a decimal value accurate to 4
** decimal places, suitable for most financial calculations. If `B` is not
** specified, returns the natural (base e) logarithm of `V`.
**
** Returns NULL if any argument is NULL.
**
** Returns NULL if the interim logarithm result is "NaN" or "Infinity". This can
** happen for certain values of `V` and `B`. See the documentation for the
** `Math.Log()` method for more information.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decLogFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_log10() SQL function.
** SQL Usage: dec_log10(V)
**
** Parameters -
**
**   V - A decimal value
**
** Returns the base 10 logarithm of `V` as a decimal value accurate to 4
** decimal places, suitable for most financial calculations.
**
** Returns NULL if `V` is NULL.
**
** Returns NULL if the interim logarithm result is "NaN" or "Infinity".
** This can happen if `V` is zero or negative.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decLog10Func(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_mult() multi-argument SQL function.
** SQL Usage: dec_mult(V1, V2, ...)
**
** Parameters -
**
**  V1  - A decimal value
**  V2  - A decimal value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the product of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_FORMAT - Any non-NULL argument does not resolve to a valid
**                - decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decMultFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_neg() SQL function.
** SQL Usage: dec_neg(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns `V` with the sign reversed.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decNegFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_pow() SQL function.
** SQL Usage: dec_pow(V, E)
**
** Parameters -
**
**  V - A decimal value
**  E - A double exponent value
**
** Returns the result of raising `V` to the power `E` as a decimal value
** accurate to 4 decimal places, suitable for most financial calculations.
**
** Returns NULL if any argument is NULL.
**
** Returns NULL if the interim result is "NaN" or "Infinity", which can happen
** for certain values of `V` and `E`. See the documentation for the `Math.Pow()`
** method for more information.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decPowFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_rem() SQL function.
** SQL Usage: dec_rem(V1, V2)
**
** Parameters -
**
**  V1 - A decimal value
**  V2 - A decimal value
**
** Returns the decimal remainder of dividing `V1` by `V2` ( V1 % V2 ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_ERROR  - The result is division by zero
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decRemFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_round() SQL function.
** SQL Usage: dec_round(V)
**            dec_round(V, N)
**            dec_round(V, N, M)
**
** Parameters -
**
**  V - A decimal value
**  N - Integer number of decimal digits to round to
**  M - The rounding mode to use, either 'even' or 'norm' (any case)
**
** Returns `V` rounded to the specified number of digits, using the specified
** rounding mode. If `N` is not specified, `V` is rounded to the appropriate
** integer value. If `M` is not specified, mode 'even' is used. That mode is the
** same as `MidpointRounding.ToEven` (also known as banker's rounding). The
** 'norm' mode is the same as `MidpointRounding.AwayFromZero` (also known as
** normal rounding).
**
** Returns NULL if `V` is NULL.
**
** If the storage format of `N` is not `INTEGER`, it is converted to an integer
** using SQLite's normal type conversion procedure. That means that if `N` is
** not a sensible integer value, it is converted to 0.
**
** Errors -
**
**  SQLITE_TOOBIG   - The result of the operation exceeded the range of a
**                  - decimal number
**  SQLITE_NOTFOUND - M is not recognized
**  SQLITE_MISUSE   - N is less than zero or greater than 28
**  SQLITE_FORMAT   - V does not resolve to a valid decimal string
**  SQLITE_NOMEM    - Memory allocation failed
*/
void decRoundFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_sub() SQL function.
** SQL Usage: dec_sub(V1, V2)
**
** Parameters -
**
**  V1 - A decimal value
**  V2 - A decimal value
**
** Returns the difference of subtracting `V2` from `V1` ( V1 - V2 ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a
**                - decimal number
**  SQLITE_FORMAT - V1 or V2 does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decSubFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the dec_total() aggregate SQL function.
** SQL Usage: dec_total(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the total of all non-NULL values in the group.
**
** This function will return '0.0' if there are only NULL values in the group.
** This behavior matches the SQLite `total()` function.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_TOOBIG - The result of the operation exceeded the range of a decimal
**                - number
**  SQLITE_FORMAT - Any non-NULL value does not resolve to a valid
**                - decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decTotStep(sqlite3_context*, int, sqlite3_value**);
void decTotFinal(sqlite3_context*);
void decTotInv(sqlite3_context*, int, sqlite3_value**);
void decTotValue(sqlite3_context*);

/* Implements the dec_trunc() SQL function.
** SQL Usage: dec_trunc(V)
**
** Parameters -
**
**  V - A decimal value
**
** Returns the integer portion of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_FORMAT - V does not resolve to a valid decimal string
**  SQLITE_NOMEM  - Memory allocation failed
*/
void decTruncFunc(sqlite3_context*, int, sqlite3_value**);
#endif /* !UTILEXT_OMIT_DECIMAL */

#ifndef UTILEXT_OMIT_REGEX

/* Implements the regsplit() table-valued SQL function
** SQL Usage: regsplit(S, P)
**            regsplit(S, P, T)
**
** Parameters -
**
**  S - The source string to split
**  P - The regular expression pattern
**  T - The timeout in milliseconds for the regular expression
**
** Returns one row for each substring that results from splitting `S` based
** on `P`:
**
** If there are no matches in `S`, then one row is returned that is identical
** to `S`.
**
** Returns no rows if any argument is NULL.
**
** If `T` is not specified, or is less than or equal to zero, the regular
** expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
** user property is set on the current managed AppDomain.
**
** This function is a wrapper for the 
** `Regex.Split(string, string, RegexOptions, TimeSpan)` static method. The
** default options are `RegexOptions.None`, but can be overridden to some extent
** with inline options.
**
** Errors -
**
**  SQLITE_ABORT - The regex operation exceeded an alloted timeout interval
**  SQLITE_ERROR - There were not enough arguments supplied to the function
**               - or there was an error parsing the regex pattern. Call
**               - <i>sqlite3_errmsg()</i> to retrieve the error message
**  SQLITE_NOMEM - Memory allocation failed
*/
extern struct sqlite3_module splitvtabModule;

/* Implements the regexp() SQL function.
** SQL Usage: regexp(P, S)  S REGEXP P
**            regexp(P, S, T)  no equivalent operator
**
** Parameters -
**
**  P - The regex pattern used for the match
**  S - The string to test for a match
**  T - The timeout interval in milliseconds for the regular expression
**
** Returns true (non-zero) if the string matches the regex; false (zero) if not.
**
** Returns NULL if any argument is NULL.
**
** If `T` is not specified, or is less than or equal to zero, the regular
** expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
** user property is set on the current managed AppDomain.
**
** This function is a wrapper for the
** `Regex.IsMatch(string, string, RegexOptions, TimeSpan)` static method. The
** default options are `RegexOptions.None`, but can be overridden to some extent
** with inline options.
**
** Errors -
**
**  SQLITE_ABORT - The regex operation exceeded an alloted timeout interval
**  SQLITE_ERROR - There were not enough arguments supplied to the function,
**               - or there was an error parsing the regex pattern. Call
**               - <i>sqlite3_errmsg()</i> to retrieve the error message
**  SQLITE_NOMEM - Memory allocation failed
*/
void regexFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the regsub() SQL function
** SQL Usage: regsub(S, P, R)
**            regsub(S, P, R, T)
**
** Parameters -
**
**  S - The string to test for a match
**  P - The regular expression pattern
**  R - The replacement text to use
**  T - The timeout in milliseconds for the regular expression
**
** Returns a string with all of the matches in `S` replaced by `R`. If there
** are no matches, then `S` is returned.
**
** Returns NULL if any argument is NULL.
**
** For those familiar with the Tcl command `regsub`, this is equivalent to
** `regsub -all $P $S $R `.
**
** If `T` is not specified, or is less than or equal to zero, the regular
** expression will run to completion, unless the `REGEX_DEFAULT_MATCH_TIMEOUT`
** user property is set on the current managed AppDomain.
**
** This function is a wrapper for the 
** `Regex.Replace(string, string, string, RegexOptions, TimeSpan)` static method.
** The default options are `RegexOptions.None`, but can be overridden to some
** extent with inline options.
**
** Errors -
**
**  SQLITE_ABORT - The regex operation exceeded an alloted timeout interval
**  SQLITE_ERROR - There were not enough arguments supplied to the function,
**               - or there was an error parsing the regex pattern. Call
**               - <i>sqlite3_errmsg()</i> to retrieve the error message
**  SQLITE_NOMEM - Memory allocation failed
*/
void regsubFunc(sqlite3_context*, int, sqlite3_value**);

#endif /* !UTILEXT_OMIT_REGEX */

#ifndef UTILEXT_OMIT_TIME

/* Implements the timespan() SQL function
** SQL Usage: timespan(V)
**            timespan(H, M, S)
**            timespan(D, H, M, S)
**            timespan(D, H, M, S, F)
**
** Parameters -
**
**  V - A valid time value in either TEXT, INTEGER, or REAL format
**  D - 32-bit signed integer number of days
**  H - 32-bit signed integer number of hours
**  M - 32-bit signed integer number of minutes
**  S - 32-bit signed integer number of seconds
**  F - 32-bit signed integer number of milliseconds
**
** Returns a 64-bit signed integer timespan.
**
** Returns NULL if any argument is NULL.
**
** Data in `TEXT` format is presumed to be a `TimeSpan` string in the format
** `[-][d.]hh:mm:ss[.fffffff]`. See the documentation for the .NET Framework
** `TimeSpan.Parse()` method for information on proper formatting.
**
** Data in `INTEGER` format is presumed to be a number of whole seconds, similar
** to a Unix time.
**
** Data in `REAL` format is presumed to be a floating-point fractional number of
** days, similar to a Julian day value.
**
** Note that data in any format, as a timespan value, is not relative to any
** particular date/time origin.
**
** Errors -
**
**  SQLITE_MISMATCH - V is a BLOB value
**  SQLITE_FORMAT   - V is a TEXT value and is not in the proper format
**  SQLITE_RANGE    - The result is out of range for a timespan value
**  SQLITE_NOMEM    - Memory allocation failed
*/
void timeCtor(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_avg() multi-argument SQL function.
** SQL Usage: timespan_avg(V1, V2, ...)
**
** Parameters -
**
**  V1  - A 64-bit signed integer timespan value
**  V2  - A 64-bit signed integer timespan value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the integer average of all non-NULL arguments.
**
** Returns 0 if all arguments are NULL, or if no arguments are specified.
**
** Note that the return value of this function is an integer. A timespan value
** is defined as an integral number of ticks, so a fractional result would be
** meaningless. Besides, the resolution of a .NET `TimeSpan` is in
** 100-nanosecond ticks, so integer division will result in a fairly precise
** value.
**
** Errors -
**
**  SQLITE_TOOBIG - The operation resulted in signed integer overflow
**  SQLITE_NOMEM  - Memory allocation failed
*/
void timeAvgAny(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_add() SQL function
** SQL Usage: timespan_add(V1, V2, ...)
**
** Parameters -
**
**  V1  - A 64-bit signed integer timespan value
**  V2  - A 64-bit signed integer timespan value
**  ... - Any number of additional values, up to the per-connection limit
**      - for function arguments
**
** Returns the sum of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
**  SQLITE_TOOBIG - The operation resulted in signed integer overflow
**  SQLITE_NOMEM  - Memory allocation failed
*/
void timeAddFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_sub() SQL function.
** SQL Usage: timespan_sub(V1, V2)
**
** Parameters -
**
**  V1 - A 64-bit signed integer timespan value
**  V2 - A 64-bit signed integer timespan value
**
** Returns the difference of `V1` and `V2` ( 'V1 - V2` ).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
**  SQLITE_TOOBIG - The operation resulted in signed integer overflow
**  SQLITE_NOMEM  - Memory allocation failed
*/
void timeSubFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_str() SQL function.
** SQL Usage: timespan_str(V)
**
** Parameters -
**
**  V - A 64-bit signed integer timespan value
**
** Returns a string in the format `[-][d.]hh:mm:ss[.fffffff]`
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void timeStrFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_neg() SQL function.
** SQL Usage: timespan_neg(V)
**
** Parameters -
**
**  V - A 64-bit signed integer timespan value
**
** Returns `V` with the sign reversed.
**
** Returns NULL if `V` is NULL.
**
** If `V` is equal to `TimeSpan.MinValue`, then `TimeSpan.MaxValue` is returned.
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void timeNegFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_cmp() SQL function.
** SQL Usage: timespan_cmp(V1, V2)
**
** Parameters -
**
**  V1 - A 64-bit signed integer timespan value
**  V2 - A 64-bit signed integer timespan value
**
** Returns NULL if either argument is NULL. Otherwise, the return value is:
**
**  Comparison result - Return value
**  V1 > V2           -  1
**  V1 == V2          -  0
**  V1 < V2           - -1
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void timeCmpFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_addto() SQL function.
** SQL Usage: timespan_addto(D, V)
**
** Parameters -
**
**  D - A valid date/time value in either TEXT, INTEGER, or REAL format
**  V - A 64-bit signed integer timespan value
**
** Returns a date/time value with the timespan `V` added to `D`, in the same
** format as `D`.
**
** Returns NULL if any argument is NULL.
**
** If `D` is in TEXT format, it is presumed to be a string representation of
** a valid .NET Framework `DateTime` value, parsable according to the current
** culture on the host machine. ISO-8601 format is recommended for date/times
** stored as TEXT.
**
** If `D` is in INTEGER format, it is presumed to be a Unix timestamp -- in
** whole seconds -- that represents a valid .NET Framework `DateTime` value.
**
** If `D` is in REAL format, it is presumed to be a Julian day value that
** represents a valid .NET Framework `DateTime` value.
**
** Errors -
**
**  SQLITE_MISMATCH - D is a BLOB value
**  SQLITE_FORMAT   - D is a TEXT value and is not in the proper format
**  SQLITE_RANGE    - The result is out of range for a timespan value
**  SQLITE_ERROR    - D is an invalid Unix time or Julian day value
*/
void timeAddToFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the timespan_diff() SQL function.
** SQL Usage: timespan_diff(D1, D2)
**
** Parameters -
**
**  D1 - A valid date/time value in either TEXT, INTEGER, or REAL format
**  D2 - A valid date/time value in either TEXT, INTEGER, or REAL format
**
** Returns a 64-bit signed integer timespan value that is the result of
** subtracting `D2` from `D1`.
**
** Returns NULL if any argument is NULL.
**
** The result will be negative if `D1` is earlier in time than `D2`.
**
** If data is in TEXT format, it is presumed to be a string representation of
** a valid .NET Framework `DateTime` value, parsable according to the current
** culture on the host machine. ISO-8601 format is recommended for date/times
** stored as TEXT.
**
** If both `D1` and `D2` are TEXT, the text encoding is presumed to be the same
** for both values. If not, the result is undefined.
**
** If data is in INTEGER format, it is presumed to be a Unix timestamp -- in
** whole seconds -- that represents a valid .NET Framework `DateTime` value.
**
** If data is in REAL format, it is presumed to be a Julian day value that
** represents a valid .NET Framework `DateTime` value.
**
** Errors -
**
**  SQLITE_MISMATCH - D1 or D2 is a BLOB value
**  SQLITE_FORMAT   - D1 or D2 is a TEXT value and is not in the proper format
**  SQLITE_ERROR    - D1 or D2 is an invalid Unix time or Julian day value
*/
void timeDiffFunc(sqlite3_context*, int, sqlite3_value ** argv);

/* Implements the timespan_total() aggregate SQL function.
** SQL Usage: timespan_total(V)
**
** Parameters -
**
**  V - A 64-bit signed integer timespan value
**
** Returns the total of all non-NULL values in the group.
**
** This function will return 0 if the group contains only NULL values. This
** behavior matches the SQLite `total()` function.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_TOOBIG - The operation resulted in signed integer overflow
**  SQLITE_NOMEM  - Memory allocation failed
*/
void timeTotStep(sqlite3_context*, int, sqlite3_value**);
void timeTotFinal(sqlite3_context*);
void timeTotInv(sqlite3_context*, int, sqlite3_value**);
void timeTotVal(sqlite3_context*);

/* Implements the timespan_avg() aggregate SQL function.
** SQL Usage: timespan_avg(V)
**
** Parameters -
**
**  V - A 64-bit signed integer timespan value
**
** Returns the average timespan of all non-NULL values in the group.
**
** This function will return 0 if there are only NULL values in the group. This
** behavior diverges from the SQLite `avg()` function.
**
** Note that the return value of this function is an integer. A timespan value
** is defined as an integral number of ticks, so a fractional result would be
** meaningless. Besides, the resolution of a .NET `TimeSpan` is in
** 100-nanosecond ticks, so integer division will result in a fairly precise
** value.
**
** [Aggregate]
**
** Errors -
**
**  SQLITE_TOOBIG - The operation resulted in signed integer overflow
**  SQLITE_NOMEM  - Memory allocation failed
*/
void timeAvgStep(sqlite3_context*, int, sqlite3_value**);
void timeAvgFinal(sqlite3_context*);
void timeAvgInv(sqlite3_context*, int, sqlite3_value**);
void timeAvgVal(sqlite3_context*);
#endif /* !UTILEXT_OMIT_TIME */

#ifndef UTILEXT_OMIT_LIKE
int allocLikeStates(LikeState **aStates);
void destroyLikeState(void *ps);

/* Implements the like() override.
** SQL Usage: like(P,S)  S LIKE P
**            like(P,S,E)  S LIKE P ESCAPE E
**
** Parameters -
**
**  P - The like pattern to match
**  S - The string to test against `P`
**  E - An optional escape character for `P`
**
** Returns true(1) or false(0) for the result of the match.
**
** Returns NULL if any argument is NULL.
**
** This function disables the SQLite like optimization, since it overrides the
** built-in `like()` function. Because the `SQLITE_LIKE_DOESNT_MATCH_BLOBS`
** symbol is intended to keep from hindering the like optimization, there is no
** point in not allowing `like()` to compare BLOBs (assuming the BLOBs actually
** represent sensible text). However, this function honors the pre-processor
** symbol, so it will return false if the `SQLITE_LIKE_DOESNT_MATCH_BLOBS`
** symbol is defined and either argument is a BLOB.
**
** Errors -
**
**  SQLITE_TOOBIG - The length in bytes of P exceeded the like pattern limit for
**                - the current connection
**  SQLITE_MISUSE - E resolves to more than 1 Unicode character in length
**  SQLITE_NOMEM  - Memory allocation failed
*/
void likeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv);

/* Implements the set_case_sensitive_like() SQL function. [MISC]
** SQL Usage: set_case_sensitive_like(B)
**
** Parameters -
**
**  B - A boolean option to enable case-sensitive like
**
** Returns the previous setting of the case-sensitivity of the `like()`
** function as a boolean integer.
**
** `B` can be 'true'['false'], 'on'['off'], 'yes'['no'], '1'['0'], or 1[0].
**
** Returns NULL if `B` is not recognized.
**
** Returns the current setting if `B` is NULL.
*/
void setLikeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv);
#endif

#ifndef UTILEXT_OMIT_STRING

/* Implements the charindex[_i]() SQL function.
** SQL Usage: charindex[_i](S, P)
**            charindex[_i](S, P, I)
**
** Parameters -
**
**  S - The string to search
**  P - The pattern to find in `S`
**  I - The integer 1-based index in `S` to start searching from
**
** Returns the 1-based index in `S` where the match occurred, or zero
** if no match was found.
**
** Returns NULL if `S` or `P` is NULL.
**
** [comparison] - case sensitive or not, depending on version called
**
** If `I` is NULL, it is considered absent and the match proceeds from the start
** of `S` (index 1).
**
** The 1-based index is used for compatibility with the SQLite `substr()` and
** `instr()` functions, which use the same indexing.
**
** Errors -
**
**  SQLITE_RANGE - I evaluates to less than 1 or greater than the length of S
**  SQLITE_NOMEM - Memory allocation failed
*/
void charindexFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the exfilter[_i]() SQL function
** SQL Usage: exfilter[_i](S, M)
**
** Parameters -
**
**  S - The source string to filter the characters from
**  M - A string containing the matching characters to remove from `S`
**
** Returns a string that contains only the characters in `S` that are not
** contained in `M`. Any characters in `S` that are also in `M` are
** removed from `S`.
**
** Returns NULL if any argument is NULL.
**
** Returns `S` if `M` or `S` is an empty string.
**
** [comparison] - case sensitive or not, depending on version called
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void exfilterFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the infilter[_i]() SQL function.
** SQL Usage: infilter[_i](S, M)
**
** Parameters -
**
**  S - The source string to filter the characters from
**  M - A string containing the matching characters to retain in `S`
**
** Returns a string that contains only the characters in `S` that are also
** contained in `M`. Any characters in `S` that are not contained in
** `M` are removed from `S`.
**
** Returns NULL if any argument is NULL.
**
** Returns an empty string if `S` or `M` is an empty string.
**
** [comparison] - case sensitive or not, depending on version called
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void infilterFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the leftstr() SQL function.
** SQL Usage: leftstr(S, N)
**
** Parameters -
**
**  S - The source string to modify
**  N - The number of characters to retain from `S`
**
** Returns a string containing the leftmost `N` characters from `S`.
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void leftFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the lower() SQL function override.
** SQL Usage: lower(S)
**
** Parameters -
**
**  S - The string to convert
**
** Returns `S` converted to lower case, using the casing conventions of the
** currently defined .NET Framework culture.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void lowerFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the padcenter() SQL function.
** SQL Usage: padcenter(S, N)
**
** Parameters -
**
**  S - The string to pad
**  N - The desired total length of the padded string
**
** Returns a string containing `S` padded with spaces on the left and right
** to equal `N`. If the difference in lengths results in an odd number of
** spaces required, the remaining space is added to the end of the string.
** Whether this is the left or right side of the string depends on whether the
** current .NET Framework culture uses a RTL writing system.
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void padcFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the padleft() SQL function.
** SQL Usage: padleft(S, N)
**
** Parameters -
**
**  S - The string to pad
**  N - The desired total length of the padded string
**
** Returns a string containing `S` padded with spaces at the beginning to equal
** `N`. If the current .NET Framework culture uses a RTL writing system, the
** spaces are added on the right (the beginning of the string).
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void padlFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the padright() SQL function.
** SQL Usage: padright(S, N)
**
** Parameters -
**
**  S - The string to pad
**  N - The desired total length of the padded string
**
** Returns a string containing `S` padded with spaces at the end to equal `N`.
** If the current .NET Framework culture uses a RTL writing system, the spaces
** are added on the left (the end of the string).
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is less than the length of `S`.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void padrFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the replicate() SQL function.
** SQL Usage: replicate(S, N)
**
** Parameters -
**
**  S - The string to replicate
**  N - The number of times to repeat `S`
**
** Returns a string that contains `S` repeated `N` times.
**
** Returns NULL if any argument is NULL.
**
** Returns an empty string if `N` is equal to zero, or if `S` is an empty string.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void replicateFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the reverse() SQL function.
** SQL Usage: reverse(S)
**
** Parameters -
**
**  S - The string to reverse
**
** Returns a string containing the characters in `S` in reverse order.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void reverseFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the rightstr() SQL function.
** SQL Usage: rightstr(S, N)
**
** Parameters -
**
**  S - The string to modify
**  N - The number of characters to retain from `S`
**
** Returns a string containing the rightmost `N` characters from `S`.
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.
**
** Errors -
**
**  SQLITE_MISUSE - N is less than zero
**  SQLITE_NOMEM  - Memory allocation failed
*/
void rightFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the set_culture() SQL function. [MISC]
** SQL Usage: set_culture(L)
**
** Parameters -
**
**  L - A recognized NLS locale name or integer LCID
**
** Assigns the specified culture to use and returns the integer LCID of the
** previous culture.
**
** `L` can be an NLS locale name in the form "ln-RG" where "ln" is the language
** tag, and "RG" is the region or location tag. Note that locale names are
** preferred over LCID's for identifying specific locales.
**
** If `L` is an integer constant or a string representation of an integer
** constant, it can be in hexadecimal (0x prefixed) or decimal format.
**
** If `L` is NULL, the integer LCID of the current culture is returned and no
** changes are made.
**
** If `L` is an empty string, the `CultureInfo.InvariantCulture` is used and the
** LCID of the previous culture is returned.
**
** The question of whether the specified culture provides the proper behavior
** with respect to the data in the database is up to the application developer.
** If there is a broad mismatch between the data in the database and the OS
** configuration of the target machine, some trial-and-error will undoubtedly
** be required.
**
** NOTE: The `set_culture()` function is NOT thread safe. This function is
** intended to be called once at application start. If this function is called
** on one connection while there are other open connections, the effects on the
** other connections are undefined, and will probably be chaotic.
**
** Errors -
**
**  SQLITE_NOTFOUND - L is not a recognized NLS locale name or identifier
**  SQLITE_NOMEM    - Memory allocation failed
*/
void cultureFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the str_concat() SQL function
** SQL Usage: str_concat(S, ...)
**
** Parameters -
**
**  S   - The separator string to use
**  ... - Two or more values (interpreted as text) to be concatenated, up to
**      - the per-connection limit for function arguments.
**
** Returns a string containing all non-NULL values separated by `S`. If `S` is
** NULL, an error is returned. To use an empty string as the separator, specify
** an empty string.
**
** Returns NULL if all supplied values are NULL.
**
** Errors -
**
**  SQLITE_MISUSE - S is NULL, or less than 3 arguments are supplied
**  SQLITE_NOMEM  - Memory allocation failed
*/
void strcatFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the upper() SQL function override.
** SQL Usage: upper(S)
**
** Parameters -
**
**  S - The string to convert
**
** Returns `S` converted to upper case, using the casing conventions of the
** currently defined .NET Framework culture.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
**  SQLITE_NOMEM - Memory allocation failed
*/
void upperFunc(sqlite3_context*, int, sqlite3_value**);

/* Implements the 'utf[_i]' collation sequence
** SQL Usage: COLLATE UTF[_I]
**
** Performs a sort-order comparison according to the currently-defined culture.
**
** [comparison] - case sensitive or not, depending on version called
*/
int utfCollate(void*, int, const void*, int, const void*);
#endif /* !UTILEXT_OMIT_STRING */
