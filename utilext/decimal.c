/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions for the Decimal extension functions.
 *
 * Most of these functions are very "boilerplate-ish", since they are really
 * just wrappers that fixup the data for consumption by their managed
 * counterparts, and return the result to SQLite.
 *
 * We could probably reduce the LOC count significantly with clever use of
 * macros, but we find that such things invariably result in more pain than
 * it's worth.
 *
 * All of these functions are marked as SQLITE_DETERMINISTIC, and none of them
 * are are marked as SQLITE_INNOCUOUS. Do so at your own risk.
 *
 * If necessary, the UTILEXT_MAKE_DIRECT preprocessor symbol can be defined to
 * add the SQLITE_DIRECTONLY flag to all functions.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_DECIMAL

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <assert.h>
#include <stdlib.h>
#include "DecimalExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::DecimalExt DecEx;

/* dec_abs(V) function */
void decAbsFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalAbs(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_add(V1,V2,...) function */
void decAddFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  DbStr result;
  DbStr *aValues;
  int n = 0;

  if (argc == 0) return;
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  isWide = util_getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      util_getText(argv[i], isWide, aValues + n);
      n++;
    }
  }
  if (n == 0) {
    free(aValues);
    sqlite3_result_null(pCtx);
    return;
  }
  rc = DecEx::DecimalAdd(n, aValues, &result);
  free(aValues);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_avg(V1,V2,...) function */
void decAvgAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  bool isWide;
  int rc;
  DbStr result;
  DbStr *aValues;
  int n = 0;

  assert(argc != 1);
  isWide = util_getEnc16(pCtx);
  if (argc == 0) goto ZERO_RESULT;
  aValues = (DbStr*)malloc(sizeof(*aValues) * argc);
  if (!aValues) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      util_getText(argv[i], isWide, aValues + n);
      n++;
    }
  }
  if (n == 0) {
    free(aValues);
ZERO_RESULT:
    if (isWide) {
      sqlite3_result_text16(pCtx, L"0.0", -1, SQLITE_TRANSIENT);
    }
    else {
      sqlite3_result_text(pCtx, "0.0", -1, SQLITE_TRANSIENT);
    }
    return;
  }
  rc = DecEx::DecimalAverageAny(n, aValues, &result);
  free(aValues);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xFinal() function for the dec_avg() SQL aggregate function.
** Per-call context is cleaned up on the managed side, so we just need to grab
** the result. The xFinal() function is also called if xStep() or xValue()
** sets an error result; the aggregate context will be -1 in that event, and
** handled by managed code.
*/
void decAvgFinal(sqlite3_context *pCtx) {
  DbStr result;
  /* if pAgg is NULL here, we have no rows, and we're going to end up returning
  ** 0.0 */
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, 0);
  int rc = DecEx::DecimalAverageFinal(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xInverse() function for the dec_avg() aggregate SQL function.
** This implementation is almost exactly like the xStep() function, but since
** we know that xStep() has been called with this value before, we don't have
** to do the error-checking; we will assume success.
*/
void decAvgInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int rc;
  u64 *pAgg;
  DbStr input;

  assert(argc == 1);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalAverageInverse(&input, pAgg);
  assert(rc == RESULT_OK);
}

/* xStep() for the dec_avg() function */
void decAvgStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  u64 *pAgg;
  int rc;
  DbStr input;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  rc = DecEx::DecimalAverageStep(&input, pAgg);
  if (rc != RESULT_OK) {
    /* flag an error state for the call to xFinal() */
    *pAgg = (u64)-1;
    util_setError(pCtx, rc);
  }
}

/* xValue() function for the aggregate dec_avg() SQL function.
** This function is identical to the xFinal() function since there's no per-
** call context to clean up.
*/
void decAvgValue(sqlite3_context *pCtx) {
  DbStr result;
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = DecEx::DecimalAverageValue(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
    /* flag error state for call to xFinal() */
    *pAgg = (u64)-1;
  }
}

/* dec_ceil(V) function */
void decCeilFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalCeiling(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_cmp(V1,V2) function */
void decCmpFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  bool isWide;
  int rc;
  int result;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = DecEx::DecimalCompare(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* 'decimal' collation sequence */
int decCollate(void *pEnc,
               int cbLeft,
               const void *pLeft,
               int cbRight,
               const void *pRight)
{
  /* A collation sequence cannot fail, and must be deterministic. We get to
  ** assume that the managed collation function has done its job correctly, and
  ** just return the result.*/
  DbStr lhs;
  DbStr rhs;

  assert(pLeft && pRight); /*SQLite deals with NULLs beforehand */
  lhs.pText = pLeft;
  lhs.cb = cbLeft;
  lhs.isWide = PTR_TO_INT(pEnc) != 0;
  rhs.pText = pRight;
  rhs.cb = cbRight;
  rhs.isWide = lhs.isWide;
  
  return DecEx::DecimalCollate(&lhs, &rhs);
}

/* dec_div(V1,V2) function */
void decDivFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr lhs;
  DbStr rhs;
  bool isWide;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = DecEx::DecimalDivide(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_floor(V) function */
void decFloorFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalFloor(&input, &result);
    if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_log(V[,B]) function */
void decLogFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  double base;
  int rc;

  assert(argc >= 1 && argc <= 2);
  if (argc == 1) {
    CHECK_ARGS_NULL(1);
  }
  else {
    CHECK_ARGS_NULL(2);
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  if (argc == 2) {
    base = sqlite3_value_double(argv[1]);
    rc = DecEx::DecimalLog(&input, base, &result);
  }
  else {
    rc = DecEx::DecimalLog(&input, &result);
  }
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else if (rc == ERR_DECIMAL_NAN) {
    sqlite3_result_null(pCtx);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_log10(V) function */
void decLog10Func(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalLog10(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else if (rc == ERR_DECIMAL_NAN) {
    sqlite3_result_null(pCtx);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_mult(V1,V2,...) function */
void decMultFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr *aValues;
  bool isWide;
  int rc;
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
  isWide = util_getEnc16(pCtx);
  for (int i = 0; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      util_getText(argv[i], isWide, aValues + n);
      n++;
    }
  }
  if (n == 0) {
    sqlite3_result_null(pCtx);
    free(aValues);
    return;
  }
  rc = DecEx::DecimalMultiply(n, aValues, &result);
  free(aValues);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_neg(V) function */
void decNegFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalNegate(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_pow(V,E) function */
void decPowFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr num;
  DbStr result;
  double exp;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  util_getText(argv[0], util_getEnc16(pCtx), &num);
  exp = sqlite3_value_double(argv[1]);
  rc = DecEx::DecimalPower(&num, exp, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else if (rc == ERR_DECIMAL_NAN) {
    sqlite3_result_null(pCtx);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_rem(V1,V2) function */
void decRemFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  bool isWide;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = DecEx::DecimalRemainder(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_round(V,[,N[,M]]) function */
void decRoundFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  DbStr mode;
  int rc;
  bool isWide;
  int nDigits = 0;

  assert(argc >= 1 && argc <= 3);
  CHECK_ARGS_NULL(1);
  isWide = util_getEnc16(pCtx);
  if (argc > 2) {
    util_getText(argv[2], isWide, &mode);
  }
  else {
    if (isWide) {
      mode.isWide = true;
      mode.pText = L"even";
      mode.cb = 8;
    }
    else {
      mode.isWide = false;
      mode.pText = "even";
      mode.cb = 4;
    }
  }
  if (argc > 1) {
    nDigits = sqlite3_value_int(argv[1]);
  }
  util_getText(argv[0], isWide, &input);
  rc = DecEx::DecimalRound(&input, nDigits, &mode, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_sub(V1,V2) function */
void decSubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  bool isWide;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = DecEx::DecimalSubtract(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xFinal() function for the dec_total() function. */
void decTotFinal(sqlite3_context *pCtx) {
  DbStr result;
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, 0);
  int rc = DecEx::DecimalTotalFinal(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xInverse() function for the dec_total() function. */
void decTotInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int rc;
  u64 *pAgg;
  DbStr input;

  assert(argc == 1);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalTotalInverse(&input, pAgg);
  assert(rc == RESULT_OK);
}

/* dec_total() aggregate function */
void decTotStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  int rc;
  u64 *pAgg;

  assert(argc == 1);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalTotalStep(&input, pAgg);
  if (rc != RESULT_OK) {
    *pAgg = (u64)-1;/* flag an error state for the call to xFinal() */
    util_setError(pCtx, rc);
  }
}

/* The xValue() function for the dec_total() function. */
void decTotValue(sqlite3_context *pCtx) {
  DbStr result;
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = DecEx::DecimalTotalValue(pAgg, util_getEnc16(pCtx), &result);
  /* the sum is calculated each step, so there is no overflow error possible
  ** at this point */
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* dec_trunc(V) function */
void decTruncFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = DecEx::DecimalTruncate(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

#endif /* !UTILEXT_OMIT_DECIMAL */
