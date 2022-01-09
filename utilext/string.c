/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * This file contains the C functions for the string-handling extension
 * functions.
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

#ifndef UTILEXT_OMIT_STRING

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <assert.h>
#include <stdlib.h>
#include "StringExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::StringExt StrEx;

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

/* Init extern variables */
sqlite3_mutex *LikeMutex = nullptr;
bool Serialized = false;
bool MatchBlobs = false;

/* Allocate user data for the like() and set_case_sensitive_like() overloads;
** user data destructors are keyed on the pointer value, so each overload has
** to have its own copy of the state; that way all but one of the functions
** can use "free()" to destroy its copy of the state, and only one of them uses
** "destroyLikeState()" to get rid of the allocated shared pointer.
*/
int allocLikeStates(LikeState **aStates) {
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
    aStates[i]->pNoCase = pFlag; /* all copies share this pointer */
  }
  /* First 3 are for UTF8, next 3 are for UTF16 */
  aStates[0]->enc16 = aStates[1]->enc16 = aStates[2]->enc16 = false;
  aStates[3]->enc16 = aStates[4]->enc16 = aStates[5]->enc16 = true;
  return 0;
}

/* Make sure the shared pointer "pNoCase" is released */
void destroyLikeState(void *ps) {
  free(((LikeState*)ps)->pNoCase);
  free(ps);
}

/* like(P,S,E) override */
void likeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  sqlite3 *db;
  bool isWide;
  bool noCase;
  int result;
  int rc;
  LikeState *pState;
  DbStr input;
  DbStr pattern;
  DbStr escape;
  DbStr *pEscape = &escape;

  assert(argc == 2 || argc == 3);
  CHECK_ARGS_NULL(2);
  if (!MatchBlobs) {
    if (sqlite3_value_type(argv[0]) == SQLITE_BLOB ||
        sqlite3_value_type(argv[1]) == SQLITE_BLOB)
    {
      sqlite3_result_int(pCtx, 0);
      return;
    }
  }
  if (argc == 3 && sqlite3_value_type(argv[2]) == SQLITE_NULL) return;
  db = sqlite3_context_db_handle(pCtx);
  if (sqlite3_value_bytes(argv[0]) >
      sqlite3_limit(db, SQLITE_LIMIT_LIKE_PATTERN_LENGTH, -1))
  {
    sqlite3_result_error_code(pCtx, SQLITE_TOOBIG);
    return;
  }
  isWide = util_getEnc16(pCtx);
  util_getText(argv[0], isWide, &pattern);
  util_getText(argv[1], isWide, &input);
  if (argc == 3) {
    util_getText(argv[2], isWide, &escape);
  }
  else {
    pEscape = NULL;
  }
  sqlite3_mutex_enter(LikeMutex);
  pState = (LikeState*)sqlite3_user_data(pCtx);
  noCase = *pState->pNoCase;
  sqlite3_mutex_leave(LikeMutex);
  rc = StrEx::Like(&input, &pattern, pEscape, noCase, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* set_case_sensitive_like(B) function */
void setLikeFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  int result;
  const char *zOption;
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
  zOption = util_getAscii(argv[0], &cbOption);
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

/* charindex[_i](S,P[,I]) function */
void charindexFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr pattern;
  int index;
  int mode;
  bool isWide;
  bool noCase;
  int result;
  int rc;

  assert(argc >= 2 && argc <= 3);
  CHECK_ARGS_NULL(2);
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
  mode = util_getData(pCtx);
  isWide = ((mode & UTF16_ENC) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  util_getText(argv[0], isWide, &input);
  util_getText(argv[1], isWide, &pattern);
  rc = StrEx::CharIndex(&input, &pattern, index, noCase, &result);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* exfilter[_i](S,M) SQL function */
void exfilterFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr source;
  DbStr match;
  DbStr result;
  int mode;
  bool isWide;
  bool noCase;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  mode = util_getData(pCtx);
  isWide = ((mode & UTF16_ENC) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  util_getText(argv[0], isWide, &source);
  util_getText(argv[1], isWide, &match);
  rc = StrEx::ExFilter(&source, &match, noCase, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* infilter[_i](S,M) SQL function */
void infilterFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr source;
  DbStr match;
  DbStr result;
  int mode;
  bool isWide;
  bool noCase;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  mode = util_getData(pCtx);
  isWide = ((mode & UTF16_ENC) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  util_getText(argv[0], isWide, &source);
  util_getText(argv[1], isWide, &match);
  rc = StrEx::InFilter(&source, &match, noCase, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* leftstr(S,N) SQL function */
void leftFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int count;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(1);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::LeftString(&input, count, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* lower(S) function */
void lowerFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::UpperLower(&input, false, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* padcenter(S,N) function */
void padcFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int iLen;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(1);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::PadCenter(&input, iLen, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* padleft(S,N) function */
void padlFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int iLen;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(1);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::PadLeft(&input, iLen, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* padright(S,N) SQL function */
void padrFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int iLen;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(1);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  iLen = sqlite3_value_int(argv[1]);
  if (iLen < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::PadRight(&input, iLen, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* replicate(S,N) SQL function */
void replicateFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int count;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(2);
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  if (input.cb == 0) {
    count = 0;
  }
  rc = StrEx::Replicate(&input, count, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* reverse(S) function */
void reverseFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::Reverse(&input, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* rightstr(S,N) function */
void rightFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int count;
  int rc;

  assert(argc == 2);
  CHECK_ARGS_NULL(1);
  if (sqlite3_value_type(argv[1]) == SQLITE_NULL) {
    sqlite3_result_value(pCtx, argv[0]);
    return;
  }
  count = sqlite3_value_int(argv[1]);
  if (count < 0) {
    sqlite3_result_error_code(pCtx, SQLITE_MISUSE);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::RightString(&input, count, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* set_culture(L) SQL function */
void cultureFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  int prev;
  int rc;

  assert(argc == 1);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
    StrEx::SetCulture(NULL, &prev);
    sqlite3_result_int(pCtx, prev);
    return;
  }
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::SetCulture(&input, &prev);
  if (rc == RESULT_OK) {
    sqlite3_result_int(pCtx, prev);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* str_concat(S,...) function */
void strcatFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr *aValues;
  DbStr result;
  int rc;
  bool isWide;
  int n = 0; /* count of non-null args besides separator */

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
  isWide = util_getEnc16(pCtx);
  /* get the separator first */
  util_getText(argv[0], isWide, aValues);
  for (int i = 1; i < argc; i++) {
    if (sqlite3_value_type(argv[i]) != SQLITE_NULL) {
      n++;
      util_getText(argv[i], isWide, aValues + n);
    }
  }
  if (n == 0) {
    free(aValues);
    sqlite3_result_null(pCtx);
    return;
  }
  rc = StrEx::Join(n + 1, aValues, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
  }
}

/* upper(S) function */
void upperFunc(sqlite3_context *pCtx, int argc, sqlite3_value **argv) {
  DbStr input;
  DbStr result;
  int rc;

  assert(argc == 1);
  CHECK_ARGS_NULL(1);
  util_getText(argv[0], util_getEnc16(pCtx), &input);
  rc = StrEx::UpperLower(&input, true, &result);
  if (rc == RESULT_OK) {
    util_setText(pCtx, &result);
  }
  else {
    util_setError(pCtx, rc);
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
int utfCollate(void *pMode,
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
  isWide = ((mode & UTF16_ENC) == 1);
  noCase = ((mode & NOCASE) == NOCASE);
  lhs.pText = pLeft;
  lhs.cb = cbLeft;
  lhs.isWide = isWide;
  rhs.pText = pRight;
  rhs.cb = cbRight;
  rhs.isWide = isWide;
  return StrEx::UtfCollate(&lhs, &rhs, noCase);
}

#endif /* !UTILEXT_OMIT_STRING */