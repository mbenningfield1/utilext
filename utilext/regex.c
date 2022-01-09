/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions for the regex extension functions, except
 * for the regex_split() function, which is in "splitvtab.c".
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

#ifndef UTILEXT_OMIT_REGEX

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <assert.h>
#include <stdlib.h>
#include "RegexExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::RegexExt RegExt;

void regexFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr pattern;
  bool isWide;
  char *zError;
  int rc;
  int result;
  int ms = -1;

  assert(argc == 2 || argc == 3);
  if (argc == 2) {
    CHECK_ARGS_NULL(2);
  }
  else {
    CHECK_ARGS_NULL(3);
  }
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &pattern);
  util_getText(argv[1], isWide, &input);
  if (argc == 3) {
    ms = sqlite3_value_int(argv[2]);
  }
  rc = RegExt::Regexp(&input, &pattern, ms, &zError, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, result);
  }
  else if (rc == ERR_REGEX_PARSE) {
    sqlite3_result_error(pCtx, zError, -1);
    free(zError);
  }
  else {
    util_setError(pCtx, rc);
  }
}


void regsubFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr pattern;
  DbStr sub;
  DbStr result;
  bool isWide;
  char *zError;
  int rc;
  int ms = -1;

  assert(argc == 3 || argc == 4);
  if (argc == 3) {
    CHECK_ARGS_NULL(3);
  }
  else {
    CHECK_ARGS_NULL(4);
  }
  if (argc == 4) {
    ms = sqlite3_value_int(argv[3]);
  }
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &input);
  util_getText(argv[1], isWide, &pattern);
  util_getText(argv[2], isWide, &sub);
  rc = RegExt::Regsub(&input, &pattern, &sub, ms, &zError, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else if (rc == ERR_REGEX_PARSE) {
    sqlite3_result_error(pCtx, zError, -1);
    free(zError);
  }
  else {
    util_setError(pCtx, rc);
  }
}

#endif /* !UTILEXT_OMIT_REGEX */
