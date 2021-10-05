/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions -- compiled as C++ (a pox be upon it) for
 * mixed-mode -- that are registered with the SQLite library as extension
 * functions.
 *
 * Most of these functions are just native entry points for the SQLite library;
 * each one just fixes up the data for consumption by managed code and calls its
 * managed counterpart for the actual processing, and sends the result back to
 * SQLite.
 *
 * A lot of these functions are virtual twins of one another, since they are
 * really nothing more than wrappers for the managed functions.
 *
 * None of these functions are marked as SQLITE_INNOCUOUS. Do so at your own
 * risk.
 *
 * All of these functions are marked as SQLITE_DETERMINISTIC, with the exception
 * of the "set_case_sensitive_like()" function.
 *
 * If necessary, the UTILEXT_MAKE_DIRECT preprocessor symbol can be defined to
 * add the SQLITE_DIRECTONLY flag to all functions.
 *
 *============================================================================*/

#pragma warning( disable : 4339 ) /* use of undefined types with /clr =>

** Also ignored LNK4248 on the linker command line; this is the linker version
** of the same warning. We are using the opaque pointers 'sqlite3',
** 'sqlite3_context', and 'sqlite3_value', which are not defined anywhere in
** managed code. The linker does the right thing, so no worries.
*/

#pragma warning( disable : 4514 ) /* unreferenced inline function removed =>

** Seems that the MS CRT headers define a lot of inline functions, and if they
** don't get referenced, Visual Studio will let you know that it went ahead and
** removed them, since you weren't using them. Also suppressed on the compiler
** command line, since this pragma doesn't seem to catch all of them.
*/

#ifdef NDEBUG
#pragma warning( disable: 4100 ) /* unreferenced formal parameter =>

** if an assert() statement makes the only reference to a parameter, such as
** argc, then ignore the warning on release builds. We also disable warnings for
** function inlining (or not inlining) for release builds on the command line.
*/
#endif /* !NDEBUG */

#pragma warning( push )
#pragma warning( disable : 4820 ) /* struct padding added =>

** We suppress the warning for struct padding when and where it's needed for
** our own structs, after we have had a chance to make sure that there isn't
** a problem with the padding; we suppress it here because we are definitely not
** interested in the warnings for the MS CRT header files. */

#include <assert.h>
#include <stdlib.h>
#include "sqlite3ext.h"

#pragma warning( pop ) /* C4820 */

#ifndef UTILEXT_OMIT_DECIMAL
#include "DecimalExt.h"
#endif

#ifndef UTILEXT_OMIT_STRING
#include "StringExt.h"
#endif

#ifndef UTILEXT_OMIT_REGEX
#include "RegexExt.h"
#include "splitvtab.h"
#endif

#ifndef UTILEXT_OMIT_TIME
#include "TimeExt.h"
#endif

/* Structure that holds a length-annotated string pointer */
typedef UtilityExtensions::DbStr DbStr;

SQLITE_EXTENSION_INIT1

/* Define this symbol to add the SQLITE_DIRECT_ONLY flag to all of the user-
** defined functions, so that they can only be called from direct SQL. */
#ifdef UTILEXT_MAKE_DIRECT
#define FUNC_DIRECT SQLITE_DIRECTONLY
#else
#define FUNC_DIRECT 0
#endif /* UTILEXT_MAKE_DIRECT */

#define FUNC_FLAGS SQLITE_DETERMINISTIC | FUNC_DIRECT

/* User data for encoding-dependent functions */
#define USE_UTF8      (void*)(0)
#define USE_UTF16     (void*)(1)

/* For functions that are both encoding-dependent and case-dependent, we use
** a combination of flags for the user data. */
#define NOCASE        2

#define UTF8_CASE     0
#define UTF16_CASE    1
#define UTF8_NOCASE   2
#define UTF16_NOCASE  3

/* For functions that return NULL on NULL arguments, simply return from the
** user function and the result will be set to NULL by SQLite. */
#define CHECK_NULL_COLUMN(V) if (sqlite3_value_type(V) == SQLITE_NULL) \
                               return

#define CHECK_NULL_COLUMNS(V1,V2) if (        \
    (sqlite3_value_type(V1) == SQLITE_NULL) ||  \
    (sqlite3_value_type(V2) == SQLITE_NULL))    \
      return

#ifdef _WIN64
#define PTR_TO_INT(X) (int)((i64)(X))
#else
#define PTR_TO_INT(X) (int)(X)
#endif


/* Gets the complex user data for encoding and case */
static int getData(sqlite3_context *pCtx) {
  return PTR_TO_INT(sqlite3_user_data(pCtx));
}

/* Return true if using UTF16 (simple user data) */
static bool getEnc16(sqlite3_context *pCtx) {
  return PTR_TO_INT(sqlite3_user_data(pCtx)) != 0;
}

/* Per the SQLite docs, the order of the *_text and *_bytes calls is significant;
** don't change it. */
static const void *getText(sqlite3_value *value, bool isWide, int *pBytes) {
  const void *pResult;
  if (isWide) {
    pResult = sqlite3_value_text16(value);
    *pBytes = sqlite3_value_bytes16(value);
  }
  else {
    pResult = sqlite3_value_text(value);
    *pBytes = sqlite3_value_bytes(value);
  }
  return pResult;
}

/* The 'zResult' pointer has been allocated by managed code, so we have to
** be sure to free it after we set the result for SQLite. If this function is
** called with a null pointer for the result, then managed code has returned
** 'OK' but the pointer is not valid, which indicates a very serious problem
** somewhere. */
static void setText(sqlite3_context *pCtx, const void *zResult, bool isWide) {
  assert(zResult);
  if (isWide) {
    sqlite3_result_text16(pCtx, zResult, -1, SQLITE_TRANSIENT);
  }
  else {
    sqlite3_result_text(pCtx, (char*)zResult, -1, SQLITE_TRANSIENT);
  }
  free((void*)zResult);
}


#ifndef UTILEXT_OMIT_TIME

#pragma warning( push )
#pragma warning( disable: 4820 ) /* struct padding added */

/* Aggregate context for the timespan total and avg functions */
typedef struct SumCtx SumCtx;
struct SumCtx {
  i64 sum;
  i64 cnt;
  bool overflow;
};

#pragma warning( pop )

typedef UtilityExtensions::TimeExt TimeEx;
typedef UtilityExtensions::DbDate DbDate;

/* Should never be true on any Windows system after Windows98, but just because
** you're paranoid doesn't mean someone isn't out to get you :) */
#if LLONG_MAX == INT_MAX
#define LLONG_MAX 0X7FFFFFFFFFFFFFFF
#define LLONG_MIN 0X8000000000000000
#endif

/* Overflow checked addition of signed long integers. */
static int addCheck64(i64 *lhs, i64 rhs) {
  i64 i = *lhs;
  if (i >= 0) {
    if (LLONG_MAX - i < rhs) return 1;
  }
  else if (rhs < LLONG_MIN - i) return 1;
  *lhs += rhs;
  return 0;
}

/* Overflow checked subtraction of signed long integers. This is basically 
** addition of the negated value, with the special case of subtracting the
** minimum value. */
static int subCheck64(i64 *lhs, i64 rhs) {
  if (rhs == LLONG_MIN) {
    if (*lhs >= 0) return 1;
    *lhs -= rhs;
    return 0;
  }
  return addCheck64(lhs, -rhs);
}

/* Implements the timespan() SQL function
** SQL Usage: timespan(V)
**            timespan(H, M, S)
**            timespan(D, H, M, S)
**            timespan(D, H, M, S, F)
**
**    V - a valid time value in either TEXT, INTEGER, or REAL format
**    D - 32-bit signed integer number of days
**    H - 32-bit signed integer number of hours
**    M - 32-bit signed integer number of minutes
**    S - 32-bit signed integer number of seconds
**    F - 32-bit signed integer number of milliseconds
**
** Returns a 64-bit signed integer TimeSpan.
**
** Returns NULL if any argument is NULL.
**
** Data in `TEXT` format is presumed to be a `TimeSpan` string in the format
** `[-][d.]hh:mm:ss[.fffffff]`; data in `INTEGER` format is presumed to be a
** number of whole seconds, similar to a Unix time; data in `REAL` format is
** presumed to be a floating-point fractional number of days, similar to a
** Julian day value.
**
** Note that data in any format, as a `TimeSpan` value, is not relative to any
** particular date/time origin.
**
** If the data is in `TEXT` format, the data must properly formatted according to
** the documentation for the .NET Framework `TimeSpan.Parse()` method.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_MISMATCH</td><td>V is a BLOB value</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V is a TEXT value and is not in the proper format</td></tr>
**  <tr><td>SQLITE_RANGE</td><td>The result would be out of range for a TimeSpan value</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeCtor(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int *pArgs;
  i64 result;
  DbDate date;
  int rc;
  bool isWide = false;

  assert((argc == 1) || (argc < 6 && argc > 2));
  if (argc == 1) {
    CHECK_NULL_COLUMN(argv[0]);
    date.type = sqlite3_value_type(argv[0]);
    switch (date.type) {
      case SQLITE_INTEGER:
        date.unix = sqlite3_value_int64(argv[0]);
        break;
      case SQLITE_FLOAT:
        date.julian = sqlite3_value_double(argv[0]);
        break;
      case SQLITE_TEXT:
        isWide = getEnc16(pCtx);
        date.iso.pText = getText(argv[0], isWide, &date.iso.cb);
        break;
      default:
        sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
        return;
    }
    rc = TimeEx::TimespanCreate(&date, isWide, &result);
    switch (rc) {
      case RESULT_OK:
        sqlite3_result_int64(pCtx, result);
        break;
      case ERR_TIME_PARSE:
        sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
        break;
      case ERR_TIME_INVALID:
        sqlite3_result_error_code(pCtx, SQLITE_RANGE);
        break;
      default:
        assert(0);
    }
  }
  else {
    pArgs = (int*)malloc(argc * sizeof(*pArgs));
    if (pArgs) {
      for (int i = 0; i < argc; i++) {
        if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
          free(pArgs);
          sqlite3_result_null(pCtx);
          return;
        }
        pArgs[i] = sqlite3_value_int(argv[i]);
      }
      rc = TimeEx::TimespanCreate(argc, pArgs, &result);
      if (rc == RESULT_OK) {
        sqlite3_result_int64(pCtx, result);
      }
      else {
        sqlite3_result_error_code(pCtx, SQLITE_RANGE);
      }
      free(pArgs);
    }
    else {
      sqlite3_result_error_nomem(pCtx);
    }
  }
}

/* Implements the timespan_add() SQL function
** SQL Usage: timespan_add(V1, V2, ...)
**
**    V1  - a 64-bit signed integer TimeSpan value.
**    V2  - a 64-bit signed integer TimeSpan value.
**    ... - any number of 64-bit signed integer TimeSpan values, up to the
**        - per-connection limit for function arguments.
**
** Returns the sum of all non-NULL arguments as a 64-bit signed integer `TimeSpan`.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeAddFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 *aValues;
  int n = 0;
  i64 sum = 0;

  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  aValues = (i64*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n] = sqlite3_value_int64(argv[i]);
      n++;
    }
  }
  if (n == 0) {
    free(aValues);
    sqlite3_result_null(pCtx);
    return;
  }
  for (int i = 0; i < n; i++) {
    if (addCheck64(&sum, aValues[i])) {
      free(aValues);
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      return;
    }
  }
  free(aValues);
  sqlite3_result_int64(pCtx, sum);
}

/* Implements the timespan_sub() SQL function.
** SQL Usage: timespan_sub(V1, V2)
**
**    V1 - a 64-bit signed integer TimeSpan value.
**    V2 - a 64-bit signed integer TimeSpan value.
**
** Returns the difference of `V1` and `V2` as a 64-bit signed integer TimeSpan.
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeSubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 lhs;
  i64 rhs;

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  lhs = sqlite3_value_int64(argv[0]);
  rhs = sqlite3_value_int64(argv[1]);
  if (subCheck64(&lhs, rhs)) {
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  sqlite3_result_int64(pCtx, lhs);
}

/* Implements the timespan_str() SQL function.
** SQL Usage: timespan_str(V)
**
**    V - a 64-bit signed integer TimeSpan value.
**
** Returns a string in the format `[-][d.]hh:mm:ss[.fffffff]`
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeStrFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int rc;
  i64 time;
  bool isWide;
  void *zResult;

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  time = sqlite3_value_int64(argv[0]);
  rc = TimeEx::TimespanStr(time, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the timespan_neg() SQL function.
** SQL Usage: timespan_neg(V)
**
**    V - a 64-bit signed integer TimeSpan value.
**
** Returns `V` with the sign reversed.
**
** Returns NULL if `V` is NULL. If `V` is equal to `TimeSpan.MinValue`, then
** `TimeSpan.MaxValue` is returned.
*/
static void timeNegFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 result;

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  result = sqlite3_value_int64(argv[0]);
  if (result == LLONG_MIN) {
    sqlite3_result_int64(pCtx, LLONG_MAX);
  }
  else {
    sqlite3_result_int64(pCtx, -result);
  }
}

/* Implements the timespan_cmp() SQL function.
** SQL Usage: timespan_cmp(V1, V2)
**
**    V1 - a 64-bit signed integer TimeSpan value.
**    V2 - a 64-bit signed integer TimeSpan value.
**
** Returns:
**
** <table style="font-size:smaller">
** <th>Condition</th><th>Return</th>
** <tr><td>V1 > V2</td><td>1</td></tr>
** <tr><td>V1 == V2</td><td>0</td></tr>
** <tr><td>V1 < V2</td><td>-1</td></tr>
** <tr><td>V1 == NULL or V2 == NULL</td><td>NULL</td></tr>
** </table>
*/
static void timeCmpFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 lhs;
  i64 rhs;
  int result = 0;

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  lhs = sqlite3_value_int64(argv[0]);
  rhs = sqlite3_value_int64(argv[1]);
  if (lhs > rhs) {
    result = 1;
  }
  else if (lhs < rhs) {
    result = -1;
  }
  sqlite3_result_int(pCtx, result);
}

/* Implements the timespan_addto() SQL function.
** SQL Usage: timespan_addto(D, V)
**
**    D - a valid date/time value in either TEXT, INTEGER, or REAL format
**    V - a 64-bit signed integer TimeSpan value.
**
** Returns a date/time value with the time span `V` added to `D`, in the same
** format as `D`.
**
** Returns NULL if any argument is NULL.
**
** If `D` is in `TEXT` format, it is presumed to be a string representation of
** a valid .NET Framework `DateTime` value, parsable according to the current
** culture on the host machine. ISO-8601 format is recommended for date/times
** stored as `TEXT`.
**
** If `D` is in `INTEGER` format, it is presumed to be a Unix timestamp -- in
** whole seconds -- that represents a valid .NET Framework `DateTime` value.
**
** If `D` is in `REAL` format, it is presumed to be a Julian day value that
** represents a valid .NET Framework `DateTime` value.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_MISMATCH</td><td>D is a BLOB value</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>D is a TEXT value and is not in the proper format</td></tr>
**  <tr><td>SQLITE_RANGE</td><td>The result would be out of range for a TimeSpan value</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>D is an invalid Unix time or Julian day value</td></tr>
** </table>
*/
static void timeAddToFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  DbDate rd = { 0 };
  int rc = 0;

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  int t = sqlite3_value_type(argv[0]);
  isWide = getEnc16(pCtx);
  switch (t) {
    case SQLITE_INTEGER:
      rd.unix = sqlite3_value_int64(argv[0]);
      break;
    case SQLITE_FLOAT:
      rd.julian = sqlite3_value_double(argv[0]);
      break;
    case SQLITE_TEXT:
      rd.iso.pText = getText(argv[0], isWide, &rd.iso.cb);
      break;
    default:
      sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
      return;
  }
  rd.type = t;
  rc = TimeEx::TimespanAddTo(&rd, isWide, sqlite3_value_int64(argv[1]));
  switch (rc) {
    case RESULT_OK:
      switch (t) {
        case SQLITE_INTEGER:
          sqlite3_result_int64(pCtx, rd.unix);
          break;
        case SQLITE_FLOAT:
          sqlite3_result_double(pCtx, rd.julian);
          break;
        case SQLITE_TEXT:
          setText(pCtx, rd.iso.pText, isWide);
          break;
      }
      break;
    case ERR_TIME_JD_RANGE:
    case ERR_TIME_UNIX_RANGE:
      sqlite3_result_error_code(pCtx, SQLITE_ERROR);
      break;
    case ERR_TIME_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the timespan_diff() SQL function.
** SQL Usage: timespan_diff(D1, D2)
**
**    D1 - a valid date/time value in either TEXT, INTEGER, or REAL format
**    D2 - a valid date/time value in either TEXT, INTEGER, or REAL format
**
** Returns a 64-bit signed integer TimeSpan value that is the result of
** subtracting `D2` from `D1`.
**
** Returns NULL if any argument is NULL.
**
** The result will be negative if `D1` is earlier in time than `D2`.
**
** If data is in `TEXT` format, it is presumed to be a string representation of
** a valid .NET Framework `DateTime` value, parsable according to the current
** culture on the host machine. ISO-8601 format is recommended for date/times
** stored as `TEXT`.
**
** If data is in `INTEGER` format, it is presumed to be a Unix timestamp -- in
** whole seconds -- that represents a valid .NET Framework `DateTime` value.
**
** If data is in `REAL` format, it is presumed to be a Julian day value that
** represents a valid .NET Framework `DateTime` value.
**
** If both `D1` and `D2` are `TEXT`, the text encoding is presumed to be the same
** for both values. If not, the result is undefined.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_MISMATCH</td><td>D1 or D2 is a BLOB</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>D1 or D2 is an invalid Unix time or Julian day value</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>D1 or D2 is a TEXT value and is not in the proper format</td></tr>
** </table>
*/
static void timeDiffFunc(sqlite3_context *pCtx, int argc, sqlite3_value ** argv) {
  int rc;
  bool isWide;
  i64 result;
  int t;
  DbDate d1 = { 0 };
  DbDate d2 = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  if ((t = sqlite3_value_type(argv[0])) != SQLITE_BLOB) {
    d1.type = t;
  }
  else {
    sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
    return;
  }
  if ((t = sqlite3_value_type(argv[1])) != SQLITE_BLOB) {
    d2.type = t;
  }
  else {
    sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
    return;
  }
  isWide = getEnc16(pCtx);
  switch (d1.type) {
    case SQLITE_INTEGER:
      d1.unix = sqlite3_value_int64(argv[0]);
      break;
    case SQLITE_FLOAT:
      d1.julian = sqlite3_value_double(argv[0]);
      break;
    case SQLITE_TEXT:
      d1.iso.pText = getText(argv[0], isWide, &d1.iso.cb);
      break;
  }
  switch (d2.type) {
    case SQLITE_INTEGER:
      d2.unix = sqlite3_value_int64(argv[1]);
      break;
    case SQLITE_FLOAT:
      d2.julian = sqlite3_value_double(argv[1]);
      break;
    case SQLITE_TEXT:
      d2.iso.pText = getText(argv[1], isWide, &d2.iso.cb);
      break;
  }
  rc = TimeEx::TimespanDiff(&d1, &d2, isWide, &result);
  switch (rc) {
    case RESULT_OK:
      sqlite3_result_int64(pCtx, result);
      break;
    case ERR_TIME_JD_RANGE:
    case ERR_TIME_UNIX_RANGE:
      sqlite3_result_error_code(pCtx, SQLITE_ERROR);
      break;
    case ERR_TIME_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the timespan_total() aggregate SQL function.
** SQL Usage: timespan_total(C)
**
**    C - an INTEGER column that contains 64-bit signed integer TimeSpan values.
**
** Aggregate Window Function: returns the sum of all non-NULL values in the
** column.
**
** NOTE: Window functions require SQLite version 3.25.0 or greater. If the
** SQLite version in use is less than 3.25.0, this function is a normal aggregate
** function.
**
** This function will return 0 if the column contains only NULL values. This
** behavior matches the SQLite `total()` function.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeTotalStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 sum;
  SumCtx *pAgg;

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  sum = pAgg->sum;
  if (addCheck64(&sum, sqlite3_value_int64(argv[0]))) {
    pAgg->overflow = true;
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
  }
  pAgg->sum = sum;
}

/* xFinal() function for the timespan_total() aggregate SQL window function */
static void timeTotalFinal(sqlite3_context *pCtx) {
  /* No rows if pAgg is NULL; if overflow has occurred, the error is already
  ** thrown from the 'step()' call */
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, 0);
  if (pAgg) {
    if (!pAgg->overflow) sqlite3_result_int64(pCtx, pAgg->sum);
  }
  else {
    sqlite3_result_int64(pCtx, 0);
  }
}

/* xInverse() function for the timespan_total() aggregate SQL window function */
static void timeTotalInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  /* If the value is not NULL, it has already been included in the sum, so we
  ** just subtract it. */
  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  pAgg->sum -= sqlite3_value_int64(argv[0]);
}

/* xValue() function for the timespan_total() aggreage SQL window function */
static void timeTotalValue(sqlite3_context *pCtx) {
  /* If we overflowed at any point, the error is raised then, so just return
  ** the current value. */
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  sqlite3_result_int64(pCtx, pAgg->sum);
}

/* Implements the timespan_avg() aggregate SQL function.
** SQL Usage: timespan_avg(C)
**
**    C - an INTEGER column that contains 64-bit signed integer TimeSpan values.
**
** Aggregate Window Function: returns the average TimeSpan of all non-NULL
** values in the column.
**
** NOTE: Window functions require SQLite version 3.25.0 or greater. If the
** SQLite version in use is less than 3.25.0, this function is a normal aggregate
** function.
**
** This function will return 0 if there are only NULL values in the column. This
** behavior diverges from the SQLite `avg()` function.
**
** Note that the return value of this function is an integer. A TimeSpan value
** is defined as an integral number of ticks, so a fractional result would be
** meaningless. Besides, the resolution of a TimeSpan is in 100-nanosecond
** ticks, so integer division will result in a fairly precise value.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The operation resulted in signed integer overflow</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void timeAvgStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 sum;
  SumCtx *pAgg;

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  sum = pAgg->sum;
  if (addCheck64(&sum, sqlite3_value_int64(argv[0]))) {
    pAgg->overflow = true;
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  pAgg->cnt++;
  pAgg->sum = sum;
}

/* xFinal() function for the timespan_avg() aggregate SQL window function */
static void timeAvgFinal(sqlite3_context *pCtx) {
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, 0);
  if (pAgg) {
    if (!pAgg->overflow) sqlite3_result_int64(pCtx, pAgg->sum / pAgg->cnt);
  }
  else {
    sqlite3_result_int64(pCtx, 0);
  }
}

/* xInverse() function for the timespan_avg() aggregate SQL window function */
static void timeAvgInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  pAgg->cnt--;
  pAgg->sum -= sqlite3_value_int64(argv[0]);
}

/* xValue() function for the timespan_avg() aggreate SQL window function */
static void timeAvgValue(sqlite3_context *pCtx) {
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  sqlite3_result_int64(pCtx, pAgg->sum / pAgg->cnt);
}
#endif /* !UTILEXT_OMIT_TIME */

#ifndef UTILEXT_OMIT_DECIMAL

typedef UtilityExtensions::DecimalExt DecEx;

/* Implements the dec_abs() SQL function.
** SQL Usage: dec_abs(V)
**
**    V - a TEXT value that represents a decimal number.
**
** Returns a string representing the absolute value of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decAbsFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  void *zResult;
  int rc;
  bool isWide;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalAbs(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_add() SQL function
** SQL Usage: dec_add(V1, V2, ...)
**
**    V1  - a TEXT value that represents a decimal number.
**    V2  - a TEXT value that represents a decimal number.
**    ... - any number of TEXT values that represent valid decimal numbers, up
**        - to the per-connection limit for function arguments.
**
** Returns a string representing the sum of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result of the operation exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not represent a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decAddFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr *aValues;
  int n = 0;

  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
      n++;
    }
  }
  if (n == 0) {
    free(aValues);
    sqlite3_result_null(pCtx);
    return;
  }
  rc = DecEx::DecimalAdd(n, aValues, isWide, &zResult);
  free(aValues);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_avg() multi-argument SQL function.
** SQL Usage: dec_avg(V1, V2, ...)
**
**    V1  - a text value that represents a decimal number.
**    V2  - a text value that represents a decimal number.
**    ... - any number of text values that represent valid decimal numbers, up
**        - to the per-connection limit for function arguments.
**
** Returns the average of all non-NULL arguments as text.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument does not represent a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decAvgAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr *aValues;
  int n = 0;

  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
      n++;
    }
  }
  if (n == 0) {
    sqlite3_result_null(pCtx);
    free(aValues);
    return;
  }
  rc = DecEx::DecimalAverageAny(n, aValues, isWide, &zResult);
  free(aValues);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* xFinal() function for the dec_avg() SQL aggregate function.
** Per-call context is cleaned up on the managed side, so we just need to grab
** the result. The xFinal() function is also called if xStep() or xValue()
** sets an error result; the aggregate context will be -1 in that event, and
** handled by managed code.
*/
static void decAvgFinal(sqlite3_context *pCtx) {
  void *zValue;
  /* if pAgg is NULL here, we have no rows, and we're going to end up returning
  ** 0.0 */
  int *pAgg = (int*)sqlite3_aggregate_context(pCtx, 0);
  bool isWide = getEnc16(pCtx);
  int rc = DecEx::DecimalAverageFinal(pAgg, isWide != 0, &zValue);
  /* we don't have to check for encoder failure, since a valid decimal value
  ** doesn't have any invalid Unicode code points in it. */
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zValue, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    case ERR_AGGREGATE:
      /* an error was set in xStep() or xValue(), so just let it go */
      break;
    default:
      assert(0);
  }
}

/* xInverse() function for the dec_avg() aggregate SQL function.
** This implementation is almost exactly like the xStep() function, but since
** we know that xStep() has been called with this value before, we don't have
** to do the error-checking; we will assume success.
*/
static void decAvgInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  int *pAgg;
  DbStr input = { 0 };

  assert(argc == 1);
  pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalAverageInverse(&input, isWide, pAgg);
  assert(rc == RESULT_OK);
}

/* Implements the dec_avg() aggregate SQL function.
** SQL Usage: dec_avg(C)
**
**    C - a TEXT column that contains valid decimal numbers
**
** Aggregate Window Function: returns a string representing the average of all
** non-NULL values in the column.
**
** NOTE: Window functions require SQLite version 3.25.0 or greater. If the
** SQLite version in use is less than 3.25.0, this function is a normal aggregate
** function.
**
** This function will return '0.0' if there are only NULL values in the column.
** This behavior diverges from the SQLite `avg()` function.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error Code</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>Any value is not recognized as a valid decimal number.
**  This behavior diverges from the SQLite <i>avg()</i> function.</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decAvgStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int *pAgg;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  rc = DecEx::DecimalAverageStep(&input, isWide, pAgg);
  if (rc != RESULT_OK) {
    /* flag an error state for the call to xFinal() */
    *pAgg = -1;
    switch (rc) {
      case ERR_NOMEM:
        sqlite3_result_error_nomem(pCtx);
        break;
      case ERR_DECIMAL_PARSE:
        sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
        break;
      case ERR_DECIMAL_OVFLOW:
        sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
        break;
      default:
        assert(0);
    }
  }
}

/* xValue() function for the aggregate dec_avg() SQL function.
** This function is identical to the xFinal() function since there's no per-
** call context to clean up.
*/
static void decAvgValue(sqlite3_context *pCtx) {
  void *zValue;
  bool isWide = getEnc16(pCtx);
  int *pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = DecEx::DecimalAverageValue(pAgg, isWide, &zValue);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zValue, isWide);
      return;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
  /* flag error state for call to xFinal() */
  *pAgg = -1;
}

/* Implements the dec_ceil() SQL function.
** SQL Usage: dec_ceil(V)
**
**    V - a TEXT value that represents a valid decimal number
**
** Returns a string representing the smallest integer that is larger than or
** equal to `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decCeilFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalCeiling(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_cmp() SQL function.
** SQL Usage: dec_cmp(V1, V2)
**
**    V1 - a TEXT value that represents a valid decimal number
**    V2 - a TEXT value that represents a valid decimal number
**
** Returns:
**
** <table style="font-size:smaller">
** <th>Condition</th><th>Return</th>
** <tr><td>V1 > V2</td><td>1</td></tr>
** <tr><td>V1 == V2</td><td>0</td></tr>
** <tr><td>V1 < V2</td><td>-1</td></tr>
** <tr><td>V1 == NULL or V2 == NULL</td><td>NULL</td></tr>
** </table>
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>V1 or V2 does not represent a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decCmpFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  int result;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  isWide = getEnc16(pCtx);
  lhs.pText = getText(argv[0], isWide, &lhs.cb);
  rhs.pText = getText(argv[1], isWide, &rhs.cb);
  rc = DecEx::DecimalCompare(&lhs, &rhs, isWide, &result);
  switch (rc) {
    case RESULT_OK:
      sqlite3_result_int(pCtx, result);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

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
static int decCollate(void *pEnc,
                      int cbLeft,
                      const void *pLeft,
                      int cbRight,
                      const void *pRight)
{
  /* A collation sequence cannot fail, and must be deterministic. We get to
  ** assume that the managed collation function has done its job correctly, and
  ** just return the result.*/
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };

  assert(pLeft && pRight); /*SQLite deals with NULLs beforehand */
  bool isWide = PTR_TO_INT(pEnc) != 0;
  lhs.pText = pLeft;
  lhs.cb = cbLeft;
  rhs.pText = pRight;
  rhs.cb = cbRight;
  return DecEx::DecimalCollate(&lhs, &rhs, isWide);
}

/* Implements the dec_div() SQL function.
** SQL Usage: dec_div(V1, V2)
**
**    V1 - a TEXT value that represents a valid decimal number
**    V2 - a TEXT value that represents a valid decimal number
**
** Returns a string representing the decimal (mathematical) quotient of dividng
** `V1` by `V2` ( `V1 / V2`).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>The result is division by zero</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decDivFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  isWide = getEnc16(pCtx);
  lhs.pText = getText(argv[0], isWide, &lhs.cb);
  rhs.pText = getText(argv[1], isWide, &rhs.cb);
  rc = DecEx::DecimalDivide(&lhs, &rhs, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    case ERR_DECIMAL_DIVZ:
      sqlite3_result_error_code(pCtx, SQLITE_ERROR);
      break;
    default:
      assert(0);
  }
};

/* Implements the dec_floor() SQL function.
** SQL Usage: dec_floor(V)
**
**    V - a TEXT value that represents a decimal number.
**
** Returns a string representing the smallest integer that is less than or equal
** to `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V does not represent a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decFloorFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input;

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalFloor(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
}

/* dec_max() function with argc > 2 arguments */
static void decMaxAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr *aValues;
  int n = 0;

  assert(argc > 2);
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
      n++;
    }
  }
  if (n == 0) {
    sqlite3_result_null(pCtx);
    free(aValues);
    return;
  }
  rc = DecEx::DecimalMaxAny(n, aValues, isWide, &zResult);
  free(aValues);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Handles the common case of 2 arguments for the dec_max() function */
static void decMaxTwo(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };
  DbStr *pLeft = &lhs;
  DbStr *pRight = &rhs;

  assert(argc == 2);
  isWide = getEnc16(pCtx);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    pLeft = NULL;
  }
  else {
    lhs.pText = getText(argv[0], isWide, &lhs.cb);
  }
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    pRight = NULL;
  }
  else {
    rhs.pText = getText(argv[1], isWide, &rhs.cb);
  }
  rc = DecEx::DecimalMax2(pLeft, pRight, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_max() SQL function.
** SQL Usage: dec_max(V1, V2, ...)
**
**    V1  - a TEXT value that represents a valid decimal number
**    V2  - a TEXT value that represents a valid decimal number
**    ... - any number of TEXT values that represent valid decimal numbers, up
**        - to the per-connection limit of the number of function arguments.
**
** Returns a string representing the maximum value of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decMaxFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  if (argc == 2)
    decMaxTwo(pCtx, argc, argv);
  else
    decMaxAny(pCtx, argc, argv);
}

/* dec_min() function with argc > 2 arguments */
static void decMinAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr *aValues;
  int n = 0;

  assert(argc > 2);
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
      n++;
    }
  }
  if (n == 0) {
    sqlite3_result_null(pCtx);
    free(aValues);
    return;
  }
  rc = DecEx::DecimalMinAny(n, aValues, isWide, &zResult);
  free(aValues);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Handles the common case of 2 arguments for the dec_min() function */
static void decMinTwo(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };
  DbStr *pLeft = &lhs;
  DbStr *pRight = &rhs;

  assert(argc == 2);
  isWide = getEnc16(pCtx);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    pLeft = NULL;
  }
  else {
    lhs.pText = getText(argv[0], isWide, &lhs.cb);
  }
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    pRight = NULL;
  }
  else {
    rhs.pText = getText(argv[1], isWide, &rhs.cb);
  }
  rc = DecEx::DecimalMin2(pLeft, pRight, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_min() SQL function.
** SQL Usage: dec_min(V1, V2, ...)
**
**    V1  - a TEXT value that represents a valid decimal number
**    V2  - a TEXT value that represents a valid decimal number
**    ... - any number of TEXT values that represent valid decimal numbers, up
**        - to the per-connection limit of the number of function arguments.
**
** Returns a string representing the minimum value of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decMinFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  if (argc == 2)
    decMinTwo(pCtx, argc, argv);
  else
    decMinAny(pCtx, argc, argv);
}

/* Implements the dec_mult() multi-argument SQL function.
** SQL Usage: dec_mult(V1, V2, ...)
**
**    V1  - a TEXT value that represents a valid decimal number
**    V2  - a TEXT value that represents a valid decimal number
**    ... - any number of TEXT values that represent valid decimal numbers, up
**        - to the per-connection limit of the number of function arguments.
**
** Returns a string representing the product of all non-NULL arguments.
**
** Returns NULL if all arguments are NULL, or if no arguments are specified.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL argument is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decMultFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr *aValues;
  int n = 0;

  if (argc == 0) return;
  if (argc == 1) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
      n++;
    }
  }
  if (n == 0) {
    sqlite3_result_null(pCtx);
    free(aValues);
    return;
  }
  rc = DecEx::DecimalMultiply(n, aValues, isWide, &zResult);
  free(aValues);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case RESULT_NULL:
      sqlite3_result_null(pCtx);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_neg() SQL function.
** SQL Usage: dec_neg(V)
**
**    V - a TEXT value that represents a decimal number.
**
** Returns a string representing `V` with the sign reversed.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decNegFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalNegate(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_rem() SQL function.
** SQL Usage: dec_rem(V1, V2)
**
**    V1 - a TEXT value that represents a valid decimal number
**    V2 - a TEXT value that represents a valid decimal number
**
** Returns a string representing the decimal remainder of dividing `V1` by `V2`
** (`V1 / V2`).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>The result is division by zero</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decRemFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  isWide = getEnc16(pCtx);
  lhs.pText = getText(argv[0], isWide, &lhs.cb);
  rhs.pText = getText(argv[1], isWide, &rhs.cb);
  rc = DecEx::DecimalRemainder(&lhs, &rhs, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    case ERR_DECIMAL_DIVZ:
      sqlite3_result_error_code(pCtx, SQLITE_ERROR);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_round() SQL function.
** SQL Usage: dec_round(V)
**            dec_round(V, N)
**            dec_round(V, N, M)
**
**    V - a TEXT value that represents a decimal number.
**    N - the number of decimal digits to round to.
**    M - the rounding mode to use, either 'even' or 'norm'
**
** Returns a string representing `V` rounded to the specified number of digits,
** using the specified rounding mode. If `N` is not specified, `V` is rounded to
** the appropriate integer. If `M` is not specified, mode 'even' is used. That
** mode is the same as `MidpointRounding.ToEven` (also known as banker's rounding).
** The 'norm' mode is the same as `MidpointRounding.AwayFromZero` (also known as
** normal rounding).
**
** Returns NULL if `V` is NULL.
**
** If the data type of `N` is not `INTEGER`, it is converted to an integer using
** SQLite's normal type conversion procedure. That means that if `N` is not a
** sensible integer value, it is converted to 0.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOTFOUND</td><td>M is not recognized</td></tr>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero or greater than 28</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decRoundFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  const void *zMode = NULL;
  int cbMode = 0;
  int nDigits = 0;
  DbStr input = { 0 };

  assert(argc >= 1 && argc <= 3);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  if (argc > 2) {
    zMode = getText(argv[2], isWide, &cbMode);
  }
  else {
    if (isWide) {
      zMode = L"even";
      cbMode = 8;
    }
    else {
      zMode = "even";
      cbMode = 4;
    }
  }
  if (argc > 1) {
    nDigits = sqlite3_value_int(argv[1]);
  }
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalRound(&input, isWide, nDigits, zMode, cbMode, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    case ERR_DECIMAL_MODE:
      sqlite3_result_error_code(pCtx, SQLITE_NOTFOUND);
      break;
    case ERR_DECIMAL_PREC:
      sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_sub() SQL function.
** SQL Usage: dec_sub(V1, V2)
**
**    V1 - a TEXT value that represents a valid decimal number
**    V2 - a TEXT value that represents a valid decimal number
**
** Returns a string representing the result of subtracting `V2` from `V1` (`V1 - V2`).
**
** Returns NULL if any argument is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>V1 or V2 is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decSubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  void *zResult;
  DbStr lhs = { 0 };
  DbStr rhs = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  isWide = getEnc16(pCtx);
  lhs.pText = getText(argv[0], isWide, &lhs.cb);
  rhs.pText = getText(argv[1], isWide, &rhs.cb);
  rc = DecEx::DecimalSubtract(&lhs, &rhs, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    case ERR_DECIMAL_OVFLOW:
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      break;
    default:
      assert(0);
  }
}

/* xFinal() function for the dec_avg() SQL aggregate window function. */
static void decSumFinal(sqlite3_context *pCtx) {
  void *zValue;
  bool isWide = getEnc16(pCtx);
  int *pAgg = (int*)sqlite3_aggregate_context(pCtx, 0);
  int rc = DecEx::DecimalSumFinal(pAgg, isWide, &zValue);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zValue, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_AGGREGATE:
      /* an error was set in xStep() or xValue(), so just let it go */
      break;
    default:
      assert(0);
  }
}

/* xInverse() function for the dec_sum() aggregate window function. */
static void decSumInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  int *pAgg;
  DbStr input = { 0 };

  assert(argc == 1);
  pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalSumInverse(&input, isWide, pAgg);
  assert(rc == RESULT_OK);
}

/* Implements the dec_sum() aggregate SQL function.
** SQL Usage: dec_sum(C)
**
**    C - a TEXT column that contains text representations of valid decimal
**      - numbers
**
** Aggregate Window Function: returns a string representing the sum of all
** non-NULL values in the column.
**
** NOTE: Window functions require SQLite version 3.25.0 or greater. If the
** SQLite version in use is less than 3.25.0, this function is a normal aggregate
** function.
**
** This function will return '0.0' if there are only NULL values in the column.
** This behavior diverges from the SQLite `sum()` function and matches the
** SQLite `total()` function.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The result exceeded the range of a decimal number</td></tr>
**  <tr><td>SQLITE_FORMAT</td><td>Any non-NULL value is not a valid decimal number.
**  This behavior diverges from both the SQLite `sum()` and `total()` functions.</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decSumStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  int *pAgg;
  DbStr input = { 0 };

  assert(argc == 1);
  pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalSumStep(&input, isWide, pAgg);
  if (rc != RESULT_OK) {
    *pAgg = -1;/* flag an error state for the call to xFinal() */
    switch (rc) {
      case ERR_NOMEM:
        sqlite3_result_error_nomem(pCtx);
        break;
      case ERR_DECIMAL_PARSE:
        sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
        break;
      case ERR_DECIMAL_OVFLOW:
        sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
        break;
      default:
        assert(0);
    }
  }
}

/* The xValue() function for the dec_sum() SQL aggregate window function. */
static void decSumValue(sqlite3_context *pCtx) {
  void *zValue;
  bool isWide = getEnc16(pCtx);
  int *pAgg = (int*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = DecEx::DecimalSumValue(pAgg, isWide, &zValue);
  /* the sum is calculated each step, so there is no overflow error possible
  ** at this point */
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zValue, isWide);
      return;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      *pAgg = -1; /* flag error state for call to xFinal() */
      break;
    default:
      assert(0);
  }
}

/* Implements the dec_trunc() SQL function.
** SQL Usage: dec_trunc(V)
**
**    V - a TEXT value that represents a decimal number.
**
** Returns a string representing the integer portion of `V`.
**
** Returns NULL if `V` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_FORMAT</td><td>V is not a valid decimal number</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void decTruncFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = DecEx::DecimalTruncate(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_DECIMAL_PARSE:
      sqlite3_result_error_code(pCtx, SQLITE_FORMAT);
      break;
    default:
      assert(0);
  }
}
#endif /* !UTILEXT_OMIT_DECIMAL */

#ifndef UTILEXT_OMIT_STRING

typedef UtilityExtensions::StringExt StrEx;

/* Implements the charindex[_i]() SQL function.
** SQL Usage: charindex[_i](S, P)
**            charindex[_i](S, P, I)
**
**    S - the string to search
**    P - the pattern to find in `S`
**    I - the integer 1-based index in `S` to start searching from
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_RANGE</td><td>I evaluates to less than 1 or greater than the length of S</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void charindexFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int index;
  int mode;
  bool isWide;
  bool noCase;
  int result;
  int rc;
  DbStr input = { 0 };
  DbStr pattern = { 0 };

  assert(argc >= 2 && argc <= 3);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  if (argc == 3) {
    if (sqlite3_value_type(argv[2]) == SQLITE_NULL) {
      index = 1;
    }
    else {
      index = sqlite3_value_int(argv[2]);
      if (index < 1) {
        sqlite3_result_error_code(pCtx, SQLITE_RANGE);
        return;
      }
    }
  }
  else {
    index = 1;
  }
  mode = getData(pCtx);
  isWide = ((mode & UTF16_CASE) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  input.pText = getText(argv[0], isWide, &input.cb);
  pattern.pText = getText(argv[1], isWide, &pattern.cb);
  rc = StrEx::CharIndex(&input, &pattern, index, isWide, noCase, &result);
  switch (rc) {
    case RESULT_OK:
      sqlite3_result_int(pCtx, result);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_INDEX:
      sqlite3_result_error_code(pCtx, SQLITE_RANGE);
      break;
    default:
      assert(0);
  }
}

/* Implements the exfilter[_i]() SQL function
** SQL Usage: exfilter[_i](S, M)
**
**    S - the source string to filter the characters from
**    M - a string containing the matching characters to remove from `S`
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void exfilterFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  void *zResult;
  int mode;
  bool isWide;
  bool noCase;
  int rc;
  DbStr source = { 0 };
  DbStr match = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  mode = getData(pCtx);
  isWide = ((mode & UTF16_CASE) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  source.pText = getText(argv[0], isWide, &source.cb);
  match.pText = getText(argv[1], isWide, &match.cb);
  rc = StrEx::ExFilter(&source, &match, isWide, noCase, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the infilter[_i]() SQL function.
** SQL Usage: infilter[_i](S, M)
**
**    S - the source string to filter the characters from
**    M - a string containing the matching characters to retain in `S`
**
** Returns a string that contains only the characters in `S` that are also
** contained in `M`. Any characters in `S` that are not contained in
** `M` are removed from `S`.
**
** Returns NULL if any argument is NULL.
**
** Returns an emtpy string if `S` or `M` is an empty string.
**
** [comparison] - case sensitive or not, depending on version called
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void infilterFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  void *zResult;
  int mode;
  bool isWide;
  bool noCase;
  int rc;
  DbStr source = { 0 };
  DbStr match = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  mode = getData(pCtx);
  isWide = ((mode & UTF16_CASE) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  source.pText = getText(argv[0], isWide, &source.cb);
  match.pText = getText(argv[1], isWide, &match.cb);
  rc = StrEx::InFilter(&source, &match, isWide, noCase, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the leftstr() SQL function.
** SQL Usage: leftstr(S, N)
**
**    S - the source string to modify
**    N - the number of characters to retain from `S`
**
** Returns a string containing the leftmost `N` characters from `S`.
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void leftFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int count;
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMN(argv[0]);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::LeftString(&input, count, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* The crux of the implementation of the like() function overload is to be able
** to provide Unicode case-folding when case-sensitivity is enabled; the default
** sqlite function does fine with UTF-16 when case is ignored.
**
** The set_case_sensitive_like() function is a bit cumbersome in use, but it is
** usually only set once when the extension is loaded, so it's not really worth
** the effort to subclass the default VFS and override the xFileControl function
** in order to trap the set_case_sensitive_like PRAGMA.
*/
#ifndef UTILEXT_OMIT_LIKE

/* Cached mutex used to serialize access to the shared state of the like
** function case-sensitivity. If the threading mode is single or multi, then
** this is null */
static sqlite3_mutex *LikeMutex = nullptr;
static bool Serialized = false;

#define NEED_MUTEX LikeMutex == nullptr && Serialized

#pragma warning( push )
#pragma warning( disable : 4820 ) /* struct padding added     */

/* User data for like() and set_case_sensitive_like() functions. The shared
** pointer is a per-connection flag for whether or not like() is case-
** sensitive; the default is true (not case-sensitive). */
typedef struct LikeState LikeState;
struct LikeState {
  bool *pNoCase;  /* shared pointer (default true) */
  bool enc16;     /* true if encoding is UTF16 */
};

#pragma warning( pop )

/* We have 4 like overloads, and 2 set_like_case overloads */
#define LIKE_STATE_CNT 6

/* Allocate user data for the like() and set_case_sensitive_like() overloads;
** user data destructors are keyed on the pointer value, so each overload has
** to have its own copy of the state */
static int allocLikeStates(LikeState **aStates) {
  bool *pFlag = (bool*)malloc(sizeof(*pFlag));
  if (!pFlag) return 1;
  *pFlag = true;
  for (int i = 0; i < LIKE_STATE_CNT; i++) {
    aStates[i] = (LikeState*)malloc(sizeof(*aStates[i]));
    if (!aStates[i]) {
      free(pFlag);
      for (int j = 0; j < i; j++) {
        free(aStates[j]);
      }
      return SQLITE_NOMEM;
    }
    aStates[i]->pNoCase = pFlag;
  }
  /* First 3 are for UTF8, next 3 are for UTF16 */
  aStates[0]->enc16 = aStates[1]->enc16 = aStates[2]->enc16 = false;
  aStates[3]->enc16 = aStates[4]->enc16 = aStates[5]->enc16 = true;
  return 0;
}

/* Make sure the shared pointer is released */
static void destroyLikeState(void *ps) {
  free(((LikeState*)ps)->pNoCase);
  free(ps);
}

/* Implements the like() override.
** SQL Usage: like(P,S) { S LIKE P }
**            like(P,S,E) { S LIKE P ESCAPE E }
**
**    P - the like pattern to match
**    S - the string to test against `P`
**    E - an optional escape character for `P`
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_TOOBIG</td><td>The length in bytes of P exceeded the like
**  pattern limit defined in SQLite</td></tr>
**  <tr><td>SQLITE_MISUSE</td><td>E resolves to more than 1 Unicode character in length</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void likeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  sqlite3 *db;
  bool isWide;
  bool noCase;
  int result;
  int rc;
  LikeState *pState;
  DbStr input = { 0 };
  DbStr pattern = { 0 };
  DbStr escape = { 0 };
  DbStr *pEscape = &escape;

  assert(argc == 2 || argc == 3);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  if (sqlite3_compileoption_used("LIKE_DOESNT_MATCH_BLOBS")) {
    if (sqlite3_value_type(argv[0]) == SQLITE_BLOB ||
        sqlite3_value_type(argv[1]) == SQLITE_BLOB)
    {
      sqlite3_result_int(pCtx, 0);
      return;
    }
  }
  if (argc == 3 && sqlite3_value_type(argv[2]) == SQLITE_NULL) return;
  isWide = getEnc16(pCtx);
  db = sqlite3_context_db_handle(pCtx);
  if (sqlite3_value_bytes(argv[0]) >
      sqlite3_limit(db, SQLITE_LIMIT_LIKE_PATTERN_LENGTH, -1))
  {
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  input.pText = getText(argv[1], isWide, &input.cb);
  pattern.pText = getText(argv[0], isWide, &pattern.cb);
  if (argc == 3) {
    escape.pText = getText(argv[2], isWide, &escape.cb);
  }
  else {
    pEscape = NULL;
  }
  sqlite3_mutex_enter(LikeMutex);
  pState = (LikeState*)sqlite3_user_data(pCtx);
  noCase = *pState->pNoCase;
  sqlite3_mutex_leave(LikeMutex);
  rc = StrEx::Like(&input, &pattern, pEscape, isWide, noCase, &result);
  switch (rc) {
    case RESULT_OK:
      sqlite3_result_int(pCtx, result);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_ESC_LENGTH:
      sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
      break;
    default:
      assert(0);
  }
}

/* Implements the set_case_sensitive_like() SQL function. [MISC]
** SQL Usage: set_case_sensitive_like(B)
**
**    B - A boolean option to enable case-sensitive like.
**
** Returns the previous setting of the case-sensitivity of the `like()`
** function as a boolean integer.
**
** Returns NULL if `B` is not recognized.
**
** Returns the current setting if `B` is NULL.
**
** `B` can be 'true'['false'], 'on'['off'], 'yes'['no'], '1'['0'], or 1[0].
*/
static void setLikeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int result;
  char *zOption;
  int cbOption;
  bool noCase;
  LikeState *pState;

  assert(argc == 1);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_mutex_enter(LikeMutex);
    pState = (LikeState*)sqlite3_user_data(pCtx);
    result = *pState->pNoCase ? 0 : 1;
    sqlite3_mutex_leave(LikeMutex);
    sqlite3_result_int(pCtx, result);
    return;
  }
  zOption = (char*)getText(argv[0], false, &cbOption);
  if (sqlite3_strnicmp(zOption, "1", cbOption) == 0 ||
      sqlite3_strnicmp(zOption, "true", cbOption) == 0 ||
      sqlite3_strnicmp(zOption, "yes", cbOption) == 0 ||
      sqlite3_strnicmp(zOption, "on", cbOption) == 0)
  {
    noCase = false;
  }
  else if (sqlite3_strnicmp(zOption, "0", cbOption) == 0 ||
           sqlite3_strnicmp(zOption, "false", cbOption) == 0 ||
           sqlite3_strnicmp(zOption, "no", cbOption) == 0 ||
           sqlite3_strnicmp(zOption, "off", cbOption) == 0)
  {
    noCase = true;
  }
  else {
    return; /* result NULL on unrecognized option */
  }
  sqlite3_mutex_enter(LikeMutex);
  pState = (LikeState*)sqlite3_user_data(pCtx);
  result = *pState->pNoCase ? 0 : 1;
  *pState->pNoCase = noCase;
  sqlite3_mutex_leave(LikeMutex);
  sqlite3_result_int(pCtx, result);
}
#endif /* !UTILEXT_OMIT_LIKE */

/* Implements the lower() SQL function override.
** SQL Usage: lower(S)
**
**    S - the string to convert
**
** Returns `S` converted to lower case, using the casing conventions of the
** currently defined .NET Framework culture.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void lowerFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::UpperLower(&input, isWide, false, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the padcenter() SQL function.
** SQL Usage: padcenter(S, N)
**
**    S - the string to pad
**    N - the desired total length of the padded string
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void padcFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int iLen;
  void *zResult;
  bool isWide;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMN(argv[0]);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::PadCenter(&input, iLen, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the padleft() SQL function.
** SQL Usage: padleft(S, N)
**
**    S - the string to pad
**    N - the desired total length of the padded string
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void padlFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int iLen;
  void *zResult;
  bool isWide;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMN(argv[0]);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::PadLeft(&input, iLen, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the padright() SQL function.
** SQL Usage: padright(S, N)
**
**    S - the string to pad
**    N - the desired total length of the padded string
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void padrFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int iLen;
  void *zResult;
  bool isWide;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMN(argv[0]);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::PadRight(&input, iLen, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the replicate() SQL function.
** SQL Usage: replicate(S, N)
**
**    S - the string to replicate
**    N - the number of times to repeat `S`
**
** Returns a string that contains `S` repeated `N` times.
**
** Returns NULL if any argument is NULL.
**
** Returns an empty string if `N` is equal to zero, or if `S` is an empty string.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void replicateFunc(sqlite3_context *pCtx,
                          int argc,
                          sqlite3_value **argv)
{
  int count;
  void *zResult;
  bool isWide;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  if (input.cb == 0) {
    count = 0;
  }
  rc = StrEx::Replicate(&input, count, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the reverse() SQL function.
** SQL Usage: reverse(S)
**
**    S - the string to reverse
**
** Returns a string containing the characters in `S` in reverse order.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void reverseFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::Reverse(&input, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the rightstr() SQL function.
** SQL Usage: rightstr(S, N)
**
**    S - the string to modify
**    N - the number of characters to retain from `S`
**
** Returns a string containing the rightmost `N` characters from `S`.
**
** Returns NULL if `S` is NULL.
**
** Returns `S` if `N` is NULL, or if `N` is greater than the length of `S`.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>N is less than zero</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void rightFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int count;
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 2);
  CHECK_NULL_COLUMN(argv[0]);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::RightString(&input, count, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the set_culture() SQL function. [MISC]
** SQL Usage: set_culture(L)
**
**    L - a recognized NLS locale name or integer LCID
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
** NOTE: The `set_culture()` function is NOT threadsafe. This function is
** intended to be called once at application start. If this function is called
** on one connection while there are other open connections, the effects on the
** other connections are undefined, and will probably be chaotic.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOTFOUND</td><td>`L` is not a recognized NLS locale name or
**  identifier</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void cultureFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  const void *zInput;
  int cbInput;
  bool isWide;
  int prev;
  int err;

  assert(argc == 1);
  isWide = getEnc16(pCtx);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    StrEx::SetCulture(NULL, 0, isWide, &prev);
    sqlite3_result_int(pCtx, prev);
    return;
  }
  zInput = getText(argv[0], isWide, &cbInput);
  err = StrEx::SetCulture(zInput, cbInput, isWide, &prev);
  switch (err) {
    case RESULT_OK:
      sqlite3_result_int(pCtx, prev);
      break;
    case ERR_CULTURE:
      sqlite3_result_error_code(pCtx, SQLITE_NOTFOUND);
      break;
    default:
      assert(0);
  }
}

/* Implements the str_concat() SQL function
** SQL Usage: str_concat(S, ...)
**
**    S   - the separator string to use
**    ... - two or more values (interpreted as text) to be concatenated, up to
**        - the per-connection limit for function arguments.
**
** Returns a string containing all non-NULL values separated by S.
**
** Returns NULL if all supplied values are NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_MISUSE</td><td>S is NULL, or less than 3 arguments are
**  supplied</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void strcatFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr *aValues;
  void *zResult;
  int rc;
  bool isWide;
  int n = 0; /* count of non-null args */

  if (argc < 3) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = getEnc16(pCtx);
  aValues[0].pText = getText(argv[0], isWide, &aValues[0].cb);
  for (int i = 1; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      n++;
      aValues[n].pText = getText(argv[i], isWide, &aValues[n].cb);
    }
  }
  if (n == 0) {
    free(aValues);
    sqlite3_result_null(pCtx);
    return;
  }
  rc = StrEx::Join(n + 1, aValues, isWide, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the upper() SQL function override.
** SQL Usage: upper(S)
**
**    S - the string to convert
**
** Returns `S` converted to upper case, using the casing conventions of the
** currently defined .NET Framework culture.
**
** Returns NULL if `S` is NULL.
**
** Errors -
**
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void upperFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  void *zResult;
  int rc;
  DbStr input = { 0 };

  assert(argc == 1);
  CHECK_NULL_COLUMN(argv[0]);
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  rc = StrEx::UpperLower(&input, isWide, true, &zResult);
  switch (rc) {
    case RESULT_OK:
      assert(zResult);
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    default:
      assert(0);
  }
}

/* Implements the 'utf[_i]' collation sequence
** SQL Usage: COLLATE UTF[_I]
**
** Performs a sort-order comparison according the Unicode casing rules defined
** on the current .NET Framework culture.
**
** [comparison] - case sensitive or not, depending on version called
*/
static int utfCollate(void *pMode,
                      int cbLeft,
                      const void *pLeft,
                      int cbRight,
                      const void *pRight)
{
  /* As with the decimal collation, we let the managed method do all the work
  ** and assume the correct answer; we just return the result to SQLite */
  int mode;
  bool isWide;
  bool noCase;
  DbStr lhs;
  DbStr rhs;

  assert(pLeft && pRight); /* SQLite will sort the NULLs before hand */
  mode = PTR_TO_INT(pMode);
  isWide = ((mode & UTF16_CASE) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  lhs.pText = pLeft;
  lhs.cb = cbLeft;
  rhs.pText = pRight;
  rhs.cb = cbRight;
  return StrEx::UtfCollate(&lhs, &rhs, isWide, noCase);
}
#endif /* !UTILEXT_OMIT_STRING */

#ifndef UTILEXT_OMIT_REGEX

typedef UtilityExtensions::RegexExt RegExt;

/* Implements the regexp() SQL function.
** SQL Usage: regexp(P, S) { S REGEXP P }
**            regexp(P, S, T) { no equivalent operator }
**
**    P - the regex pattern used for the match
**    S - the string to test for a match
**    T - the timeout interval in milliseconds for the regular expression
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>There was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i>
**  to retrieve the parse error message.</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void regexFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  char *zError;
  int rc;
  int result;
  int ms = -1;
  DbStr input = { 0 };
  DbStr pattern = { 0 };

  assert(argc == 2 || argc == 3);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  if (argc == 3) {
    CHECK_NULL_COLUMN(argv[2]);
  }
  isWide = getEnc16(pCtx);
  pattern.pText = getText(argv[0], isWide, &pattern.cb);
  input.pText = getText(argv[1], isWide, &input.cb);
  if (argc == 3) {
    ms = sqlite3_value_int(argv[2]);
  }
  rc = RegExt::Regexp(&input, &pattern, isWide, ms, &zError, &result);
  switch (rc) {
    case RESULT_OK:
      sqlite3_result_int(pCtx, result);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_REGEX_PARSE:
      sqlite3_result_error(pCtx, zError, -1);
      free(zError);
      break;
    case ERR_REGEX_TIMEOUT:
      sqlite3_result_error_code(pCtx, SQLITE_ABORT);
      break;
    default:
      assert(0);
  }
}

/* Implements the regsub() SQL function
** SQL Usage: regsub(S, P, R)
**            regsub(S, P, R, T)
**
**    S - the string to test for a match
**    P - the regular expression pattern
**    R - the replacement text to use
**    T - the timeout in milliseconds for the regular expression
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
** <table style="font-size:smaller">
**  <th>Error</th><th>Condition</th>
**  <tr><td>SQLITE_ABORT</td><td>The regex operation exceeded an alloted timeout interval</td></tr>
**  <tr><td>SQLITE_ERROR</td><td>There was an error parsing the regex pattern. Call <i>sqlite3_errmsg()</i>
**  to retrieve the parse error message.</td></tr>
**  <tr><td>SQLITE_NOMEM</td><td>Memory allocation failed</td></tr>
** </table>
*/
static void regsubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  char *zError;
  int rc;
  void *zResult;
  int ms = -1;
  DbStr input = { 0 };
  DbStr pattern = { 0 };
  DbStr sub = { 0 };

  assert(argc == 3 || argc == 4);
  CHECK_NULL_COLUMNS(argv[0], argv[1]);
  if (argc == 4) {
    CHECK_NULL_COLUMNS(argv[2], argv[3]);
  }
  else {
    CHECK_NULL_COLUMN(argv[2]);
  }
  if (argc == 4) {
    ms = sqlite3_value_int(argv[3]);
  }
  isWide = getEnc16(pCtx);
  input.pText = getText(argv[0], isWide, &input.cb);
  pattern.pText = getText(argv[1], isWide, &pattern.cb);
  sub.pText = getText(argv[2], isWide, &sub.cb);
  rc = RegExt::Regsub(&input, &pattern, &sub, isWide, ms, &zError, &zResult);
  switch (rc) {
    case RESULT_OK:
      setText(pCtx, zResult, isWide);
      break;
    case ERR_NOMEM:
      sqlite3_result_error_nomem(pCtx);
      break;
    case ERR_REGEX_PARSE:
      sqlite3_result_error(pCtx, zError, -1);
      free(zError);
      break;
    case ERR_REGEX_TIMEOUT:
      sqlite3_result_error_code(pCtx, SQLITE_ABORT);
      break;
    default:
      assert(0);
  }
}
#endif /* !UTILEXT_OMIT_REGEX */

#pragma warning( push )
#pragma warning( disable : 4820 ) /* struct padding added     */

static int registerFunctions(sqlite3 *db) {
  int vNum = sqlite3_libversion_number();
  static const struct {
    char *zName;
    signed char nArg;
    void(*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } aFuncs[] = {
  #ifndef UTILEXT_OMIT_DECIMAL
    { "dec_abs",        1,  decAbsFunc    },
    { "dec_add",       -1,  decAddFunc    },
    { "dec_avg",       -1,  decAvgAny     },
    { "dec_ceil",       1,  decCeilFunc   },
    { "dec_cmp",        2,  decCmpFunc    },
    { "dec_div",        2,  decDivFunc    },
    { "dec_floor",      1,  decFloorFunc  },
    { "dec_max",       -1,  decMaxFunc    },
    { "dec_min",       -1,  decMinFunc    },
    { "dec_mult",      -1,  decMultFunc   },
    { "dec_neg",        1,  decNegFunc    },
    { "dec_rem",        2,  decRemFunc    },
    { "dec_round",      1,  decRoundFunc  },
    { "dec_round",      2,  decRoundFunc  },
    { "dec_round",      3,  decRoundFunc  },
    { "dec_sub",        2,  decSubFunc    },
    { "dec_trunc",      1,  decTruncFunc  },
  #endif
  #ifndef UTILEXT_OMIT_STRING
    { "leftstr",        2,  leftFunc      },
    { "lower",          1,  lowerFunc     },
    { "padcenter",      2,  padcFunc      },
    { "padleft",        2,  padlFunc      },
    { "padright",       2,  padrFunc      },
    { "replicate",      2,  replicateFunc },
    { "reverse",        1,  reverseFunc   },
    { "rightstr",       2,  rightFunc     },
    { "str_concat",    -1,  strcatFunc    },
    { "upper",          1,  upperFunc     },
  #endif
  #ifndef UTILEXT_OMIT_REGEX
    { "regexp",         2,  regexFunc     },
    { "regexp",         3,  regexFunc     },
    { "regsub",         3,  regsubFunc    },
    { "regsub",         4,  regsubFunc    },
  #endif
  #ifndef UTILEXT_OMIT_TIME
    { "timespan_addto", 2,  timeAddToFunc },
    { "timespan_diff",  2,  timeDiffFunc  },
    { "timespan_str",   1,  timeStrFunc   },
  #endif
  };

  /* encoding-dependent user data */
  for (int i = 0; i != sizeof(aFuncs) / sizeof(aFuncs[0]); i++) {
    sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArg,
                            SQLITE_UTF8 | FUNC_FLAGS,
                            USE_UTF8, aFuncs[i].xFunc, 0, 0);

    sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArg,
                            SQLITE_UTF16 | FUNC_FLAGS,
                            USE_UTF16, aFuncs[i].xFunc, 0, 0);
  }

#ifndef UTILEXT_OMIT_DECIMAL
  sqlite3_create_collation(db, "decimal", SQLITE_UTF8, USE_UTF8, decCollate);

  sqlite3_create_collation(db, "decimal", SQLITE_UTF16, USE_UTF16, decCollate);

  if (vNum >= MIN_WINDOW_VERSION) {
    /* aggregate window functions with encoding-dependent user data */
    sqlite3_create_window_function(db, "dec_avg", 1,
                                   SQLITE_UTF8 | FUNC_FLAGS, USE_UTF8,
                                   decAvgStep, decAvgFinal,
                                   decAvgValue, decAvgInv, 0);

    sqlite3_create_window_function(db, "dec_avg", 1,
                                   SQLITE_UTF16 | FUNC_FLAGS, USE_UTF16,
                                   decAvgStep, decAvgFinal,
                                   decAvgValue, decAvgInv, 0);

    sqlite3_create_window_function(db, "dec_sum", 1,
                                   SQLITE_UTF8 | FUNC_FLAGS, USE_UTF8,
                                   decSumStep, decSumFinal,
                                   decSumValue, decSumInv, 0);

    sqlite3_create_window_function(db, "dec_sum", 1,
                                   SQLITE_UTF16 | FUNC_FLAGS, USE_UTF16,
                                   decSumStep, decSumFinal,
                                   decSumValue, decSumInv, 0);
  }
  else {
    /* normal aggregate functions with encoding-dependent user data */
    sqlite3_create_function(db, "dec_avg", 1, SQLITE_UTF8 | FUNC_FLAGS,
                            USE_UTF8, 0, decAvgStep, decAvgFinal);

    sqlite3_create_function(db, "dec_avg", 1, SQLITE_UTF16 | FUNC_FLAGS,
                            USE_UTF16, 0, decAvgStep, decAvgFinal);

    sqlite3_create_function(db, "dec_sum", 1, SQLITE_UTF8 | FUNC_FLAGS,
                            USE_UTF8, 0, decSumStep, decSumFinal);

    sqlite3_create_function(db, "dec_sum", 1, SQLITE_UTF16 | FUNC_FLAGS,
                            USE_UTF16, 0, decSumStep, decSumFinal);
  }
#endif /* !UTILEXT_OMIT_DECIMAL */

#ifndef UTILEXT_OMIT_STRING
#ifndef UTILEXT_OMIT_LIKE
  LikeState *aStates[LIKE_STATE_CNT];
  if (allocLikeStates(aStates)) return SQLITE_NOMEM;
  /* Array indices 0-2 are UTF8 encoding; 3-5 are UTF16 encoding */
  sqlite3_create_function_v2(db, "like", 2, SQLITE_UTF8 | FUNC_FLAGS,
                             aStates[0], likeFunc, 0, 0, free);

  sqlite3_create_function_v2(db, "like", 3, SQLITE_UTF8 | FUNC_FLAGS,
                             aStates[1], likeFunc, 0, 0, free);

  sqlite3_create_function_v2(db, "like", 2, SQLITE_UTF16 | FUNC_FLAGS,
                             aStates[3], likeFunc, 0, 0, free);

  sqlite3_create_function_v2(db, "like", 3, SQLITE_UTF16 | FUNC_FLAGS,
                             aStates[4], likeFunc, 0, 0, free);

  /* No SQLITE_DETERMINISTIC flag, SQLITE_DIRECTONLY explicit */
  sqlite3_create_function_v2(db, "set_case_sensitive_like", 1,
                             SQLITE_UTF8 | SQLITE_DIRECTONLY, aStates[2],
                             setLikeFunc, 0, 0, free);

  sqlite3_create_function_v2(db, "set_case_sensitive_like", 1,
                             SQLITE_UTF16 | SQLITE_DIRECTONLY, aStates[5],
                             setLikeFunc, 0, 0, destroyLikeState);
#endif /* !UTILEXT_OMIT_LIKE */

  /* SQLITE_DIRECTONLY explicit */
  sqlite3_create_function(db, "set_culture", 1,
                          SQLITE_UTF8 | SQLITE_DIRECTONLY,
                          USE_UTF8, cultureFunc, 0, 0);

  sqlite3_create_function(db, "set_culture", 1,
                          SQLITE_UTF16 | SQLITE_DIRECTONLY,
                          USE_UTF16, cultureFunc, 0, 0);

  sqlite3_create_collation(db, "utf", SQLITE_UTF8,
                           (void*)UTF8_CASE, utfCollate);

  sqlite3_create_collation(db, "utf_i", SQLITE_UTF8,
                           (void*)UTF8_NOCASE, utfCollate);

  sqlite3_create_collation(db, "utf", SQLITE_UTF16,
                           (void*)UTF16_CASE, utfCollate);

  sqlite3_create_collation(db, "utf_i", SQLITE_UTF16,
                           (void*)UTF16_NOCASE, utfCollate);

  static const struct {
    char *zName;
    signed char nArg;
    int modeFlag;
    void(*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } aCaseFuncs[] = {
    { "charindex",    2,  0,      charindexFunc },
    { "charindex_i",  2,  NOCASE, charindexFunc },
    { "charindex",    3,  0,      charindexFunc },
    { "charindex_i",  3,  NOCASE, charindexFunc },
    { "exfilter",     2,  0,      exfilterFunc  },
    { "exfilter_i",   2,  NOCASE, exfilterFunc  },
    { "infilter",     2,  0,      infilterFunc  },
    { "infilter_i",   2,  NOCASE, infilterFunc  }
  };

#pragma warning( push )
#pragma warning( disable : 4312 ) /* cast 32-bit int to void* */

  /* case-dependent and encoding-dependent user data */
  for (int i = 0; i < sizeof(aCaseFuncs) / sizeof(aCaseFuncs[0]); i++) {
    sqlite3_create_function(db, aCaseFuncs[i].zName, aCaseFuncs[i].nArg,
                            SQLITE_UTF8 | FUNC_FLAGS,
                            (void*)(UTF8_CASE | aCaseFuncs[i].modeFlag),
                            aCaseFuncs[i].xFunc, 0, 0);

    sqlite3_create_function(db, aCaseFuncs[i].zName, aCaseFuncs[i].nArg,
                            SQLITE_UTF16 | FUNC_FLAGS,
                            (void*)(UTF16_CASE | aCaseFuncs[i].modeFlag),
                            aCaseFuncs[i].xFunc, 0, 0);
  }

#pragma warning( pop ) /* 4312 */
#endif /* !UTILEXT_OMIT_STRING */

#ifndef UTILEXT_OMIT_TIME
  static const struct {
    char *zName;
    signed char nArg;
    void(*xFunc)(sqlite3_context*, int, sqlite3_value**);
  } timeFuncs[] = {
    { "timespan",     1,  timeCtor    },
    { "timespan",     3,  timeCtor    },
    { "timespan",     4,  timeCtor    },
    { "timespan",     5,  timeCtor    },
    { "timespan_add", -1, timeAddFunc },
    { "timespan_sub", 2,  timeSubFunc },
    { "timespan_cmp", 2,  timeCmpFunc },
    { "timespan_neg", 1,  timeNegFunc },
  };


  /* no user data, encoding is irrelevant */
  for (int i = 0; i != sizeof(timeFuncs) / sizeof(timeFuncs[0]); i++) {
    sqlite3_create_function(db, timeFuncs[i].zName, timeFuncs[i].nArg,
                            SQLITE_UTF8 | FUNC_FLAGS, 0,
                            timeFuncs[i].xFunc, 0, 0);
  }

  if (vNum >= MIN_WINDOW_VERSION) {
    sqlite3_create_window_function(db, "timespan_avg", 1,
                                   SQLITE_UTF8 | FUNC_FLAGS, 0,
                                   timeAvgStep, timeAvgFinal,
                                   timeAvgValue, timeAvgInv, 0);

    sqlite3_create_window_function(db, "timespan_total", 1,
                                   SQLITE_UTF8 | FUNC_FLAGS, 0,
                                   timeTotalStep, timeTotalFinal,
                                   timeTotalValue, timeTotalInv, 0);
  }
  else {
    sqlite3_create_function(db, "timespan_avg", 1, SQLITE_UTF8 | FUNC_FLAGS,
                            0, 0, timeAvgStep, timeAvgFinal);

    sqlite3_create_function(db, "timespan_total", 1, SQLITE_UTF8 | FUNC_FLAGS,
                            0, 0, timeTotalStep, timeTotalFinal);
  }
#endif /* !UTILEXT_OMIT_TIME */

#ifndef UTILEXT_OMIT_REGEX
  sqlite3_create_module(db, "regsplit", &splitvtabModule, 0);
#endif

  return SQLITE_OK;
} /* registerFunctions() */

#pragma warning( pop ) /* 4820 */

/* Dummy function pointer signature for auto_extension */
typedef void(*xBlank)(void);

extern "C" {

  __declspec(dllexport)
    int sqlite3_utilext_init(sqlite3 *db,
                             char **pzErrMsg,
                             const sqlite3_api_routines *pApi)
  {
    SQLITE_EXTENSION_INIT2(pApi);
    if (registerFunctions(db)) {
      *pzErrMsg = sqlite3_mprintf("state allocation failed");
      return SQLITE_NOMEM;
    }
#if !defined(UTILEXT_OMIT_STRING) && !defined(UTILEXT_OMIT_LIKE)
    Serialized = (sqlite3_db_mutex(db) != nullptr);
    if (NEED_MUTEX) {
      LikeMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
      if (!LikeMutex) {
        /* if we can't allocate a mutex, hopefully we can allocate a string */
        *pzErrMsg = sqlite3_mprintf("mutex allocation failed");
        return SQLITE_NOMEM;
      }
    }
#endif
    return 0;
  }

#pragma warning( push )
#pragma warning( disable : 4191 ) /* unsafe function pointer cast */

  __declspec(dllexport)
    int utilext_persist_init(sqlite3 *db,
                             char **pzErrMsg,
                             const sqlite3_api_routines *pApi)
  {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    if (registerFunctions(db)) {
      *pzErrMsg = sqlite3_mprintf("state allocation failed");
      return SQLITE_NOMEM;
    }
#if !defined(UTILEXT_OMIT_STRING) && !defined(UTILEXT_OMIT_LIKE)
    Serialized = (sqlite3_db_mutex(db) != nullptr);
    if (NEED_MUTEX) {
      LikeMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
      if (!LikeMutex) {
        *pzErrMsg = sqlite3_mprintf("mutex allocation failed");
        return SQLITE_NOMEM;
      }
    }
#endif
    rc = sqlite3_auto_extension((xBlank)sqlite3_utilext_init);
    if (rc == SQLITE_OK) rc = SQLITE_OK_LOAD_PERMANENTLY;
    return rc;
  }

#pragma warning( pop ) /* 4191 */

} /* extern "C" */
