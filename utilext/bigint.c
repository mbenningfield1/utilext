/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions for the BigInteger extension functions.
 *
 * We're storing BigIntegers as hex strings, since it is the best trade-off
 * for serialization performance. Preliminary testing shows:
 *
 *    Average time to parse decimal number 13.1257872 ticks
 *    Average time to make decimal number 0.6125384 ticks
 *    Average decimal round trip 6.8691628 ticks
 *
 *    Average time to make hex number (factory) 4.7342268 ticks
 *    Average time to parse hex number 2.2537052 ticks
 *    Average time to make hex number (custom) 0.7139236 ticks
 *    Average hex number round trip (factory) 3.493966 ticks
 *    Average hex number round trip (custom) 1.4838144 ticks
 *
 *    Average time to parse byte[] 0.1449664 ticks
 *    Average time to make byte[] 0.2439856 ticks
 *    Average byte[] round trip 0.194476 ticks
 *
 * Here we can see that producing a decimal string is really rather quick, but
 * parsing a decimal string is morbidly slow compared to anything else. Storing
 * BigIntegers as binary would be by far the best choice, but without a custom
 * collation sequence, that's a non-starter. The BigInteger byte array format
 * is in little-endian byte order, which tosses binary comparison (memcmp) right
 * out the window.
 *
 * Since we can't do that, we managed to gain a fair amount by writing our own
 * function to produce a hex number, by using a static lookup table, which costs
 * us less than 800 bytes, and gains us better than twice the performance of
 * round-tripping a BigInteger to hex and back.
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

#ifndef UTILEXT_OMIT_BIGINT

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <assert.h>
#include <stdlib.h>
#include "BigIntExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::BigIntExt IntExt;

/* bigint_abs(V) function */
void bintAbs(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr data;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &data);
  rc = IntExt::BigIntAbs(&data, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_add(V1,V2,...) function */
void bintAdd(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr *pArr;
  int rc;
  int n = 0;
  int type;
  bool isWide;

  if (argc == 0) return;
  isWide = util_getEnc16(pCtx);
  pArr = (DbStr*)malloc(sizeof(*pArr) * argc);
  if (pArr) {
    for (int i = 0; i < argc; i++) {
      type = sqlite3_value_type(argv[i]);
      if (type != SQLITE_NULL) {
        util_getText(argv[i], isWide, pArr + n);
        n++;
      }
    }
    if (n == 0) {
      sqlite3_result_null(pCtx);
      free(pArr);
      return;
    }
    else {
      rc = IntExt::BigIntAdd(pArr, n, &result);
    }
    if (rc == RESULT_OK) {
      util_setText(pCtx, &result);
    }
    else {
      sqlite3_result_error_code(pCtx, rc);
    }
    free(pArr);
  }
  else {
    sqlite3_result_error_code(pCtx, SQLITE_NOMEM);
  }
}

/* bigint_and(V1,V2) function */
void bintAnd(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  int rc;
  bool isWide;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = IntExt::BigIntAnd(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_avg(V1,V2,...) function */
void bintAvgAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr *pArr;
  int rc;
  int type;
  bool isWide;
  int n = 0;

  assert(argc != 1);
  isWide = util_getEnc16(pCtx);

  if (argc == 0) goto ZERO_RESULT;
  pArr = (DbStr*)malloc(sizeof(*pArr) * argc);
  if (pArr) {
    for (int i = 0; i < argc; i++) {
      type = sqlite3_value_type(argv[i]);
      if (type != SQLITE_NULL) {
        util_getText(argv[i], isWide, pArr + n);
        n++;
      }
    }
    if (n == 0) {
      free(pArr);
ZERO_RESULT:
      if (isWide) {
        sqlite3_result_text16(pCtx, L"0", -1, SQLITE_TRANSIENT);
      }
      else {
        sqlite3_result_text(pCtx, "0", -1, SQLITE_TRANSIENT);
      }
      return;
    }
    else {
      rc = IntExt::BigIntAverage(pArr, n, &result);
    }
    if (rc == RESULT_OK) {
      util_setText(pCtx, &result);
    }
    else {
      sqlite3_result_error_code(pCtx, rc);
    }
    free(pArr);
  }
  else {
    sqlite3_result_error_code(pCtx, SQLITE_NOMEM);
  }
}

/* xFinal() for the bigint_avg() aggregate function */
void bintAvgFinal(sqlite3_context *pCtx) {
  DbStr result;

  /* if pAgg is NULL here, we have no rows, and we're going to end up returning
  ** 0 */
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, 0);
  int rc = IntExt::BigIntAverageFinal(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xInverse() for the bigint_avg() aggregate function */
void bintAvgInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  int rc;
  u64 *pAgg;

  assert(argc == 1);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntAverageInverse(&input, pAgg);
  assert(rc == RESULT_OK);
}

/* xStep() for the bigint_avg() aggregate function */
void bintAvgStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
  rc = IntExt::BigIntAverageStep(&input, pAgg);
  if (rc != RESULT_OK) {
    /* flag an error state for the call to xFinal() */
    *pAgg = (u64)-1;
    util_setError(pCtx, rc);
  }
}

/* xValue() for the bigint_avg() aggregate function */
void bintAvgValue(sqlite3_context *pCtx) {
  DbStr result;
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = IntExt::BigIntAverageValue(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
    /* flag error state for call to xFinal() */
    *pAgg = (u64)-1;
  }
}

/* 'bigint' collation sequence */
int bintCollate(void *pEnc,
                int cbLeft,
                const void *pLeft,
                int cbRight,
                const void *pRight)
{
  /* Pretty much the same as the other collation sequences */
  DbStr lhs;
  DbStr rhs;

  assert(pLeft && pRight); /*SQLite deals with NULLs beforehand */
  lhs.pText = pLeft;
  lhs.cb = cbLeft;
  lhs.isWide = PTR_TO_INT(pEnc) != 0;
  rhs.pText = pRight;
  rhs.cb = cbRight;
  rhs.isWide = lhs.isWide;
  
  return IntExt::BigIntCollate(&lhs, &rhs);
}

/* bigint_cmp(V1,V2) function. */
void bintCmp(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
  rc = IntExt::BigIntCompare(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint() ctor function */
void bintCtor(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr number;
  int rc;
  i64 iVal;
  double dVal;
  bool isWide;
  int type;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  isWide = util_getEnc16(pCtx);
  type = sqlite3_value_type(argv[0]);
  switch (type) {
    case SQLITE_INTEGER:
      iVal = sqlite3_value_int64(argv[0]);
      rc = IntExt::BigIntCreate(iVal, isWide, &result);
      break;
    case SQLITE_FLOAT:
      dVal = sqlite3_value_double(argv[0]);
      rc = IntExt::BigIntCreate(dVal, isWide, &result);
      break;
    case SQLITE_TEXT:
      util_getText(argv[0], isWide, &number);
      rc = IntExt::BigIntCreate(&number, &result);
      break;
    default:
      sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
      return;
  }
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_div(V1,V2) function. */
void bintDiv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
  rc = IntExt::BigIntDivide(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_gcd(V1,V2) function. */
void bintGcd(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
  rc = IntExt::BigIntGCD(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_log(V[,B]) function. */
void bintLog(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  double base;
  double result;
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
    rc = IntExt::BigIntLog(&input, base, &result);
  }
  else {
    rc = IntExt::BigIntLog(&input, &result);
  }
  if (rc == RESULT_OK) {
    sqlite3_result_double(pCtx, result);
  }
  else if(rc == ERR_BIGINT_OVFLOW) {
    sqlite3_result_null(pCtx);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_log10(V) function. */
void bintLog10(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  double result;
  int rc;
  
  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntLog10(&input, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_double(pCtx, result);
  }
  else if(rc == ERR_BIGINT_OVFLOW) {
    sqlite3_result_null(pCtx);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_lsh(V,S) function */
void bintLShift(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;
  int shift;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  shift = sqlite3_value_int(argv[1]);
  rc = IntExt::BigIntLeftShift(&input, shift, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_modpow(V,E,M) function */
void bintModPow(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr inputs[3];
  DbStr result;
  int rc;
  bool isWide;

  assert(argc == 3);
  CHECK_ARGS_NULL(3);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &inputs[0]);
  util_getText(argv[1], isWide, &inputs[1]);
  util_getText(argv[2], isWide, &inputs[2]);
  rc = IntExt::BigIntModPow(inputs, argc, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_mult(V1,V2,...) function */
void bintMult(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr *pArr;
  int rc;
  int n = 0;
  int type;
  bool isWide;

  if (argc == 0) return;
  isWide = util_getEnc16(pCtx);
  pArr = (DbStr*)malloc(sizeof(*pArr) * argc);
  if (pArr) {
    for (int i = 0; i < argc; i++) {
      type = sqlite3_value_type(argv[i]);
      if (type != SQLITE_NULL) {
        util_getText(argv[i], isWide, pArr + n);
        n++;
      }
    }
    if (n == 0) {
      sqlite3_result_null(pCtx);
      free(pArr);
      return;
    }
    else {
      rc = IntExt::BigIntMultiply(pArr, n, &result);
    }
    if (rc == RESULT_OK) {
      util_setText(pCtx, &result);
    }
    else {
      sqlite3_result_error_code(pCtx, rc);
    }
    free(pArr);
  }
  else {
    sqlite3_result_error_code(pCtx, SQLITE_NOMEM);
  }
}

/* bigint_neg(V) function */
void bintNeg(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntNegate(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_not(V) function */
void bintNot(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntNot(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_or(V1,V2) function */
void bintOr(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  int rc;
  bool isWide;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = IntExt::BigIntOr(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* bigint_pow(V,E) function */
void bintPow(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;
  int exp;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  exp = sqlite3_value_int(argv[1]);
  if (exp < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_ERROR);
    return;
  }
  rc = IntExt::BigIntPow(&input, exp, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_rem(V1,V2) function */
void bintRem(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  int rc;
  bool isWide;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = IntExt::BigIntRemainder(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_rsh(V,S) function */
void bintRShift(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;
  int shift;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  shift = sqlite3_value_int(argv[1]);
  rc = IntExt::BigIntRightShift(&input, shift, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_str(V) function */
void bintStr(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr result;
  DbStr input;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntString(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* bigint_sub(V1,V2) function */
void bintSub(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr lhs;
  DbStr rhs;
  DbStr result;
  int rc;
  bool isWide;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &lhs);
  util_getText(argv[1], isWide, &rhs);
  rc = IntExt::BigIntSubtract(&lhs, &rhs, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xFinal() for the bigint_total() aggregate function */
void bintTotFinal(sqlite3_context *pCtx) {
  DbStr result;

  /* if pAgg is NULL here, we have no rows, and we're going to end up returning
  ** 0 */
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, 0);
  int rc = IntExt::BigIntTotalFinal(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xInverse() for the bigint_total() aggregate function */
void bintTotInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  int rc;
  u64 *pAgg;

  assert(argc == 1);
  pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = IntExt::BigIntTotalInverse(&input, pAgg);
  assert(rc == RESULT_OK);
}

/* xStep() for the bigint_total() aggregate function */
void bintTotStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
  rc = IntExt::BigIntTotalStep(&input, pAgg);
  if (rc != RESULT_OK) {
    /* flag an error state for the call to xFinal() */
    *pAgg = (u64)-1;
    util_setError(pCtx, rc);
  }
}

/* xValue() for the bigint_total() aggregate function */
void bintTotValue(sqlite3_context *pCtx) {
  DbStr result;
  u64 *pAgg = (u64*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  int rc = IntExt::BigIntTotalValue(pAgg, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
    /* flag error state for call to xFinal() */
    *pAgg = (u64)-1;
  }
}

#endif /* !UTILEXT_OMIT_BIGINT */
