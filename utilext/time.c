/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions for the TimeSpan extension functions.
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

#ifndef UTILEXT_OMIT_TIME

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <assert.h>
#include <stdlib.h>
#include "TimeExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::TimeExt TimeEx;

/* timespan(V|[D]H,M,S[,F]) function */
void timeCtor(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int *pArgs;
  i64 result;
  DbDate date;
  int rc;

  assert((argc == 1) || (argc < 6 && argc > 2));
  if (argc == 1) {
    CHECK_ARGS_NULL(1);
    date.type = sqlite3_value_type(argv[0]);
    switch (date.type) {
      case SQLITE_INTEGER:
        date.unix = sqlite3_value_int64(argv[0]);
        break;
      case SQLITE_FLOAT:
        date.julian = sqlite3_value_double(argv[0]);
        break;
      case SQLITE_TEXT:
        util_getText(argv[0], util_getEnc16(pCtx), &date.iso);
        break;
      default:
        sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
        return;
    }
    rc = TimeEx::TimespanCreate(&date, &result);
    
    if (rc == RESULT_OK) {
      sqlite3_result_int64(pCtx, result);
    }
    else {
      util_setError(pCtx, rc);
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

/* timespan_avg(V1,V2,...) function */
void timeAvgAny(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 *aValues;
  int n = 0;
  i64 sum = 0;

  assert(argc != 1);
  if (argc == 0) {
    sqlite3_result_int64(pCtx, 0);
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
    sqlite3_result_int64(pCtx, 0);
    return;
  }
  for (int i = 0; i < n; i++) {
    if (util_addCheck64(&sum, aValues[i])) {
      free(aValues);
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      return;
    }
  }
  free(aValues);
  sqlite3_result_int64(pCtx, sum / n);
}

/* timespan_add(V1,V2,...) function */
void timeAddFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
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
    if (util_addCheck64(&sum, aValues[i])) {
      free(aValues);
      sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
      return;
    }
  }
  free(aValues);
  sqlite3_result_int64(pCtx, sum);
}

/* timespan_sub(V1,V2) function */
void timeSubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 lhs;
  i64 rhs;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  lhs = sqlite3_value_int64(argv[0]);
  rhs = sqlite3_value_int64(argv[1]);
  if (util_subCheck64(&lhs, rhs)) {
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  sqlite3_result_int64(pCtx, lhs);
}

/* timespan_str(V) function */
void timeStrFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int rc;
  i64 time;
  DbStr result;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  time = sqlite3_value_int64(argv[0]);
  rc = TimeEx::TimespanStr(time, util_getEnc16(pCtx), &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* timespan_neg(V) function */
void timeNegFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 result;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  result = sqlite3_value_int64(argv[0]);
  if (result == LLONG_MIN) {
    sqlite3_result_int64(pCtx, LLONG_MAX);
  }
  else {
    sqlite3_result_int64(pCtx, -result);
  }
}

/* timespan_cmp(V1,V2) function */
void timeCmpFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 lhs;
  i64 rhs;
  int result = 0;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
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

/* timespan_addto(D,V) function */
void timeAddToFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbDate startDate;
  DbDate result;
  int rc = 0;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  int t = sqlite3_value_type(argv[0]);
  switch (t) {
    case SQLITE_INTEGER:
      startDate.unix = sqlite3_value_int64(argv[0]);
      break;
    case SQLITE_FLOAT:
      startDate.julian = sqlite3_value_double(argv[0]);
      break;
    case SQLITE_TEXT:
      util_getText(argv[0], util_getEnc16(pCtx), &startDate.iso);
      break;
    default:
      sqlite3_result_error_code(pCtx, SQLITE_MISMATCH);
      return;
  }
  startDate.type = t;
  rc = TimeEx::TimespanAddTo(sqlite3_value_int64(argv[1]), &startDate, &result);
  if (rc == RESULT_OK) {
    switch (t) {
      case SQLITE_INTEGER:
        sqlite3_result_int64(pCtx, result.unix);
        break;
      case SQLITE_FLOAT:
        sqlite3_result_double(pCtx, result.julian);
        break;
      case SQLITE_TEXT:
        util_setText(pCtx, &result.iso);
        break;
    }    
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* timespan_diff(D1,D2) function */
void timeDiffFunc(sqlite3_context *pCtx, int argc, sqlite3_value ** argv) {
  int rc;
  bool isWide;
  i64 result;
  int t;
  DbDate d1;
  DbDate d2;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
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
  isWide = util_getEnc16(pCtx);
  switch (d1.type) {
    case SQLITE_INTEGER:
      d1.unix = sqlite3_value_int64(argv[0]);
      break;
    case SQLITE_FLOAT:
      d1.julian = sqlite3_value_double(argv[0]);
      break;
    case SQLITE_TEXT:
      util_getText(argv[0], isWide, &d1.iso);
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
      util_getText(argv[1], isWide, &d2.iso);
      break;
  }
  rc = TimeEx::TimespanDiff(&d1, &d2, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int64(pCtx, result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* xStep() for timespan_total(V) */
void timeTotStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 sum;
  SumCtx *pAgg;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  sum = pAgg->sum;
  if (util_addCheck64(&sum, sqlite3_value_int64(argv[0]))) {
    pAgg->overflow = true;
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
  }
  pAgg->sum = sum;
}

/* xFinal() function for the timespan_total() */
void timeTotFinal(sqlite3_context *pCtx) {
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

/* xInverse() function for the timespan_total() */
void timeTotInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  /* If the value is not NULL, it has already been included in the sum, so we
  ** just subtract it. */
  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  pAgg->sum -= sqlite3_value_int64(argv[0]);
}

/* xValue() function for the timespan_total() */
void timeTotVal(sqlite3_context *pCtx) {
  /* If we overflowed at any point, the error is raised then, so just return
  ** the current value. */
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  sqlite3_result_int64(pCtx, pAgg->sum);
}

/* xStep() for timespan_avg(V) */
void timeAvgStep(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  i64 sum;
  SumCtx *pAgg;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  if (!pAgg) {
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  sum = pAgg->sum;
  if (util_addCheck64(&sum, sqlite3_value_int64(argv[0]))) {
    pAgg->overflow = true;
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  pAgg->cnt++;
  pAgg->sum = sum;
}

/* xFinal() function for the timespan_avg() */
void timeAvgFinal(sqlite3_context *pCtx) {
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, 0);
  if (pAgg) {
    if (!pAgg->overflow) {
      i64 temp = pAgg->sum;
      if (temp < 0) {
        temp = -((i64)((u64)(-temp) / pAgg->cnt));
      }
      else {
        temp = (i64)((u64)(temp) / pAgg->cnt);
      }
      sqlite3_result_int64(pCtx, temp);
    }
  }
  else {
    sqlite3_result_int64(pCtx, 0);
  }
}

/* xInverse() function for the timespan_avg() */
void timeAvgInv(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  pAgg->cnt--;
  pAgg->sum -= sqlite3_value_int64(argv[0]);
}

/* xValue() function for the timespan_avg() */
void timeAvgVal(sqlite3_context *pCtx) {
  SumCtx *pAgg = (SumCtx*)sqlite3_aggregate_context(pCtx, sizeof(*pAgg));
  i64 temp = pAgg->sum;
  if (temp < 0) {
    temp = -((i64)((u64)(-temp) / pAgg->cnt));
  }
  else {
    temp = (i64)((u64)(temp) / pAgg->cnt);
  }
  sqlite3_result_int64(pCtx,temp);
}


#endif /* !UTILEXT_OMIT_TIME */