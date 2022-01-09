/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the exported init functions that SQLite will look for when
 * loading the extension library, and the register function that hooks up all of
 * the native entry points for the extension functions.
 *
 * The other source files with a ".c" extension contain the application-defined
 * extension functions for the various subsets of the utilext library: string
 * functions, decimal functions, regex functions, timespan functions, and big
 * integer functions. These files are C code -- since we're using mixed-mode,
 * they are compiled as C++ (a pox be upon it).
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

#pragma warning( disable : 4820 ) /* struct padding added =>

** We aren't concerned about struct padding per se, and we've arranged our own
** structs to obtain as little padding as practical; And we are certainly not
** concerned about struct padding in the MS CRT header files, which VS insists
** on telling us about, even though there's not a thing we can do about it. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3ext.h"
#include "utilext.h"

SQLITE_EXTENSION_INIT1

/* Gets the complex user data for encoding and case */
int util_getData(sqlite3_context *pCtx) {
  return PTR_TO_INT(sqlite3_user_data(pCtx));
}

/* Return true if using UTF16 (simple user data) */
bool util_getEnc16(sqlite3_context *pCtx) {
  return PTR_TO_INT(sqlite3_user_data(pCtx)) != 0;
}

/* Gets narrow text in the ASCII range */
const char *util_getAscii(sqlite3_value *value, int *pBytes) {
  const char *pResult;
  /* Per the SQLite docs, the order of the *_text and *_bytes calls is
  ** significant; don't change it. */
  pResult = (const char*)sqlite3_value_text(value);
  *pBytes = sqlite3_value_bytes(value);
  return pResult;
}

/* Gets text as UTF-8 or UTF-16 */
void util_getText(sqlite3_value *value, bool isWide, DbStr *pStr) {
  assert(pStr);
  pStr->isWide = isWide;
  if (isWide) {
    pStr->pText = sqlite3_value_text16(value);
    pStr->cb = sqlite3_value_bytes16(value);
  }
  else {
    pStr->pText = sqlite3_value_text(value);
    pStr->cb = sqlite3_value_bytes(value);
  }
}

/* The 'pText' pointer in 'pResult' has been allocated by managed code, so we
** have to be sure to free it after we set the result for SQLite. If this
** function is called with a null pointer for the result, then managed code has
** returned 'OK' but the pointer is not valid, which indicates a very serious
** problem somewhere.
*/
void util_setText(sqlite3_context *pCtx, DbStr *pResult) {
  assert(pResult && pResult->pText);
  if (pResult->isWide) {
    sqlite3_result_text16(pCtx, pResult->pText, -1, SQLITE_TRANSIENT);
  }
  else {
    sqlite3_result_text(pCtx, (char*)pResult->pText, -1, SQLITE_TRANSIENT);
  }
  free((void*)pResult->pText);
}

/* Sets a function error result based on the error code returned from the
** managed function call.
*/
void util_setError(sqlite3_context *pCtx, int error) {
  if (error == RESULT_NULL) {
    sqlite3_result_null(pCtx);
  }
  else {
    sqlite3_result_error_code(pCtx, error);
  }
}

/* Overflow checked addition of signed long integers. */
int util_addCheck64(i64 *lhs, i64 rhs) {
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
int util_subCheck64(i64 *lhs, i64 rhs) {
  if (rhs == LLONG_MIN) {
    if (*lhs >= 0) return 1;
    *lhs -= rhs;
    return 0;
  }
  return util_addCheck64(lhs, -rhs);
}


void util_option(sqlite3_context *pCtx, int argc, sqlite3_value** argv) {
#ifdef UTILEXT_OMIT_STRING
#define STR_OK 0
#else
#define STR_OK 1
#endif
#ifdef UTILEXT_OMIT_DECIMAL
#define DEC_OK 0
#else
#define DEC_OK 1
#endif
#ifdef UTILEXT_OMIT_BIGINT
#define BINT_OK 0
#else
#define BINT_OK 1
#endif
#ifdef UTILEXT_OMIT_REGEX
#define REG_OK 0
#else
#define REG_OK 1
#endif
#ifdef UTILEXT_OMIT_LIKE
#define LIKE_OK 0
#else
#define LIKE_OK 1
#endif
#ifdef UTILEXT_OMIT_TIME
#define TIME_OK 0
#else
#define TIME_OK 1
#endif

  char *pInput;
  char bigint[] = "bigint";
  char decimal[] = "decimal";
  char regex[] = "regex";
  char string[] = "string";
  char like[] = "like";
  char time[] = "timespan";

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  pInput = (char*)sqlite3_value_text(argv[0]);
  if (_strnicmp(bigint, pInput, sizeof(bigint)) == 0) {
    sqlite3_result_int(pCtx, BINT_OK);
  }
  else if (_strnicmp(decimal, pInput, sizeof(decimal)) == 0) {
    sqlite3_result_int(pCtx, DEC_OK);
  }
  else if (_strnicmp(regex, pInput, sizeof(regex)) == 0) {
    sqlite3_result_int(pCtx, REG_OK);
  }
  else if (_strnicmp(string, pInput, sizeof(string)) == 0) {
    sqlite3_result_int(pCtx, STR_OK);
  }
  else if (_strnicmp(like, pInput, sizeof(like)) == 0) {
    sqlite3_result_int(pCtx, LIKE_OK);
  }
  else if (_strnicmp(time, pInput, sizeof(time)) == 0) {
    sqlite3_result_int(pCtx, TIME_OK);
  }
  else {
    sqlite3_result_null(pCtx);
  }
}

static int registerFunctions(sqlite3 *db) {
  /* scalar functions */
  static const struct {
    char *zName;
    void(*xFunc)(sqlite3_context*, int, sqlite3_value**);
    i8 nArg;
    int userData;
  } sFuncs[] = {
    { "util_capable",   util_option,    1, 0      },
  #ifndef UTILEXT_OMIT_DECIMAL
    { "dec_abs",        decAbsFunc,     1, 0      },
    { "dec_add",        decAddFunc,    -1, 0      },
    { "dec_avg",        decAvgAny,     -1, 0      },
    { "dec_ceil",       decCeilFunc,    1, 0      },
    { "dec_cmp",        decCmpFunc,     2, 0      },
    { "dec_div",        decDivFunc,     2, 0      },
    { "dec_floor",      decFloorFunc,   1, 0      },
    { "dec_log",        decLogFunc,     1, 0      },
    { "dec_log",        decLogFunc,     2, 0      },
    { "dec_log10",      decLog10Func,   1, 0      },
    { "dec_mult",       decMultFunc,   -1, 0      },
    { "dec_neg",        decNegFunc,     1, 0      },
    { "dec_pow",        decPowFunc,     2, 0      },
    { "dec_rem",        decRemFunc,     2, 0      },
    { "dec_round",      decRoundFunc,   1, 0      },
    { "dec_round",      decRoundFunc,   2, 0      },
    { "dec_round",      decRoundFunc,   3, 0      },
    { "dec_sub",        decSubFunc,     2, 0      },
    { "dec_trunc",      decTruncFunc,   1, 0      },
  #endif
  #ifndef UTILEXT_OMIT_STRING
    { "charindex",      charindexFunc,  2, 0      },
    { "charindex_i",    charindexFunc,  2, NOCASE },
    { "charindex",      charindexFunc,  3, 0      },
    { "charindex_i",    charindexFunc,  3, NOCASE },
    { "exfilter",       exfilterFunc,   2, 0      },
    { "exfilter_i",     exfilterFunc,   2, NOCASE },
    { "infilter",       infilterFunc,   2, 0      },
    { "infilter_i",     infilterFunc,   2, NOCASE },
    { "leftstr",        leftFunc,       2, 0      },
    { "lower",          lowerFunc,      1, 0      },
    { "padcenter",      padcFunc,       2, 0      },
    { "padleft",        padlFunc,       2, 0      },
    { "padright",       padrFunc,       2, 0      },
    { "replicate",      replicateFunc,  2, 0      },
    { "reverse",        reverseFunc,    1, 0      },
    { "rightstr",       rightFunc,      2, 0      },
    { "str_concat",     strcatFunc,    -1, 0      },
    { "upper",          upperFunc,      1, 0      },
  #endif
  #ifndef UTILEXT_OMIT_REGEX
    { "regexp",         regexFunc,      2, 0      },
    { "regexp",         regexFunc,      3, 0      },
    { "regsub",         regsubFunc,     3, 0      },
    { "regsub",         regsubFunc,     4, 0      },
  #endif
  #ifndef UTILEXT_OMIT_TIME
    { "timespan",       timeCtor,       1, 0      },
    { "timespan",       timeCtor,       3, 0      },
    { "timespan",       timeCtor,       4, 0      },
    { "timespan",       timeCtor,       5, 0      },
    { "timespan_add",   timeAddFunc,   -1, 0      },
    { "timespan_addto", timeAddToFunc,  2, 0      },
    { "timespan_avg",   timeAvgAny,    -1, 0      },
    { "timespan_diff",  timeDiffFunc,   2, 0      },
    { "timespan_cmp",   timeCmpFunc,    2, 0      },
    { "timespan_neg",   timeNegFunc,    1, 0      },
    { "timespan_str",   timeStrFunc,    1, 0      },
    { "timespan_sub",   timeSubFunc,    2, 0      },
  #endif
  #ifndef UTILEXT_OMIT_BIGINT
    { "bigint",         bintCtor,       1, 0      },
    { "bigint_abs",     bintAbs,        1, 0      },
    { "bigint_add",     bintAdd,       -1, 0      },
    { "bigint_and",     bintAnd,        2, 0      },
    { "bigint_avg",     bintAvgAny,    -1, 0      },
    { "bigint_cmp",     bintCmp,        2, 0      },
    { "bigint_div",     bintDiv,        2, 0      },
    { "bigint_gcd",     bintGcd,        2, 0      },
    { "bigint_log",     bintLog,        1, 0      },
    { "bigint_log",     bintLog,        2, 0      },
    { "bigint_log10",   bintLog10,      1, 0      },
    { "bigint_lsh",     bintLShift,     2, 0      },
    { "bigint_modpow",  bintModPow,     3, 0      },
    { "bigint_mult",    bintMult,      -1, 0      },
    { "bigint_neg",     bintNeg,        1, 0      },
    { "bigint_not",     bintNot,        1, 0      },
    { "bigint_or",      bintOr,         2, 0      },
    { "bigint_pow",     bintPow,        2, 0      },
    { "bigint_rem",     bintRem,        2, 0      },
    { "bigint_rsh",     bintRShift,     2, 0      },
    { "bigint_str",     bintStr,        1, 0      },
    { "bigint_sub",     bintSub,        2, 0      },
  #endif
  };

#pragma warning( disable : 4312 ) /* cast 32-bit integer to void* =>

  ** In several places in the following code, we cast a 32-bit integer to a
  ** void*, to be used as context data. The pointers are never de-referenced,
  ** they are just cast back to integers to take the value. */

  for (int i = 0; i < sizeof(sFuncs) / sizeof(sFuncs[0]); i++) {
    sqlite3_create_function(db, sFuncs[i].zName, sFuncs[i].nArg,
                            SQLITE_UTF8 | FUNC_FLAGS,
                            (void*)(UTF8_ENC | sFuncs[i].userData),
                            sFuncs[i].xFunc, 0, 0);

    sqlite3_create_function(db, sFuncs[i].zName, sFuncs[i].nArg,
                            SQLITE_UTF16 | FUNC_FLAGS,
                            (void*)(UTF16_ENC | sFuncs[i].userData),
                            sFuncs[i].xFunc, 0, 0);
  }

#ifndef UTILEXT_OMIT_REGEX
  sqlite3_create_module(db, "regsplit", &splitvtabModule, 0);
#endif

  /* We need at least one of these to be undefined, or we have no collation
  ** sequences.
  */
#if !defined(UTILEXT_OMIT_DECIMAL) ||\
    !defined(UTILEXT_OMIT_STRING) ||\
    !defined(UTILEXT_OMIT_BIGINT)
  /* collation sequences */
  static const struct {
    char *zName;
    int(*xComp)(void*, int, const void*, int, const void*);
    int userData;
  } cFuncs[] = {
#ifndef UTILEXT_OMIT_DECIMAL
    {"decimal", decCollate,  0      },
#endif
#ifndef UTILEXT_OMIT_STRING
    {"utf",     utfCollate,  0      },
    {"utf_i",   utfCollate,  NOCASE },
#endif
#ifndef UTILEXT_OMIT_BIGINT
    {"bigint",  bintCollate, 0      }
#endif

  };
  for (int i = 0; i < sizeof(cFuncs) / sizeof(cFuncs[0]); i++) {
    sqlite3_create_collation(db, cFuncs[i].zName,
                             SQLITE_UTF8,
                             (void*)(UTF8_ENC | cFuncs[i].userData),
                             cFuncs[i].xComp);

    sqlite3_create_collation(db, cFuncs[i].zName,
                             SQLITE_UTF16,
                             (void*)(UTF16_ENC | cFuncs[i].userData),
                             cFuncs[i].xComp);
  }
#endif /* !OMIT_STRING || !OMIT_DECIMAL || !OMIT_BIGINT */

  /* At least one of these has to be not defined, or we have no aggregate
  ** functions.
  */
#if !defined(UTILEXT_OMIT_DECIMAL) ||\
    !defined(UTILEXT_OMIT_TIME) ||\
    !defined(UTILEXT_OMIT_BIGINT)

  /* aggregate functions */
  static const struct {
    char *zName;
    int nArgs;
    void(*xStep)(sqlite3_context*, int, sqlite3_value**);
    void(*xFinal)(sqlite3_context*);
    void(*xInverse)(sqlite3_context*, int, sqlite3_value**);
    void(*xValue)(sqlite3_context*);
  } aFuncs[] = {
#ifndef UTILEXT_OMIT_DECIMAL
    {"dec_avg",        1, decAvgStep, decAvgFinal, decAvgInv, decAvgValue },
    {"dec_total",      1, decTotStep, decTotFinal, decTotInv, decTotValue },
#endif
#ifndef UTILEXT_OMIT_TIME
    {"timespan_avg",   1, timeAvgStep, timeAvgFinal, timeAvgInv, timeAvgVal },
    {"timespan_total", 1, timeTotStep, timeTotFinal, timeTotInv, timeTotVal },
#endif
#ifndef UTILEXT_OMIT_BIGINT
    {"bigint_avg",     1, bintAvgStep, bintAvgFinal, bintAvgInv, bintAvgValue },
    {"bigint_total",   1, bintTotStep, bintTotFinal, bintTotInv, bintTotValue },
#endif
  };

  int vNum = sqlite3_libversion_number();
  for (int i = 0; i < sizeof(aFuncs) / sizeof(aFuncs[0]); i++) {
    if (vNum >= MIN_WINDOW_VERSION) {
      sqlite3_create_window_function(db, aFuncs[i].zName, aFuncs[i].nArgs,
                                     SQLITE_UTF8 | FUNC_FLAGS, (void*)UTF8_ENC,
                                     aFuncs[i].xStep, aFuncs[i].xFinal,
                                     aFuncs[i].xValue, aFuncs[i].xInverse, 0);

      sqlite3_create_window_function(db, aFuncs[i].zName, aFuncs[i].nArgs,
                                     SQLITE_UTF16 | FUNC_FLAGS, (void*)UTF16_ENC,
                                     aFuncs[i].xStep, aFuncs[i].xFinal,
                                     aFuncs[i].xValue, aFuncs[i].xInverse, 0);
    }
    else { /* aggregates only */
      sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArgs,
                              SQLITE_UTF8 | FUNC_FLAGS, (void*)UTF8_ENC,
                              0, aFuncs[i].xStep, aFuncs[i].xFinal);

      sqlite3_create_function(db, aFuncs[i].zName, aFuncs[i].nArgs,
                              SQLITE_UTF16 | FUNC_FLAGS, (void*)UTF16_ENC,
                              0, aFuncs[i].xStep, aFuncs[i].xFinal);
    }
  }

#endif /* !OMIT_DECIMAL || !OMIT_TIME || !OMIT_BIGINT */

#ifndef UTILEXT_OMIT_LIKE

  /* Set up the state we have to maintain to deal with whether or not the
  ** like function overload is case-sensitive and register the functions that
  ** deal with our like implementation. */
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

  /* No SQLITE_DETERMINISTIC flag, SQLITE_DIRECTONLY explicitly set */
  sqlite3_create_function_v2(db, "set_case_sensitive_like", 1,
                             SQLITE_UTF8 | SQLITE_DIRECTONLY, aStates[2],
                             setLikeFunc, 0, 0, free);

  sqlite3_create_function_v2(db, "set_case_sensitive_like", 1,
                             SQLITE_UTF16 | SQLITE_DIRECTONLY, aStates[5],
                             setLikeFunc, 0, 0, destroyLikeState);

#endif /* !UTILEXT_OMIT_LIKE */

#ifndef UTILEXT_OMIT_STRING

  /* SQLITE_DIRECTONLY explicitly set on this function */
  sqlite3_create_function(db, "set_culture", 1,
                          SQLITE_UTF8 | SQLITE_DIRECTONLY,
                          (void*)UTF8_ENC, cultureFunc, 0, 0);

  sqlite3_create_function(db, "set_culture", 1,
                          SQLITE_UTF16 | SQLITE_DIRECTONLY,
                          (void*)UTF16_ENC, cultureFunc, 0, 0);

#endif /* !UTILEXT_OMIT_STRING */

  return SQLITE_OK;
} /* registerFunctions() */


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
#ifndef UTILEXT_OMIT_LIKE
    Serialized = (sqlite3_db_mutex(db) != nullptr);
    if (NEED_MUTEX) {
      LikeMutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
      if (!LikeMutex) {
        *pzErrMsg = sqlite3_mprintf("mutex allocation failed");
        return SQLITE_NOMEM;
      }
    }
    MatchBlobs = sqlite3_compileoption_used("LIKE_DOESNT_MATCH_BLOBS") != 0;
#endif
    return 0;
  }

#pragma warning( push )
#pragma warning( disable : 4191 ) /* unsafe function pointer cast */

  /* Dummy function pointer signature for auto_extension */
  typedef void(*xBlank)(void);

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
#ifndef UTILEXT_OMIT_LIKE
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
