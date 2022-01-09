/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Code adapted from "templatevtab.c" and "series.c" in the sqlite source repo.
 *
 * A virtual table implementation to perform Regex.Split() and return the
 * resulting items as rows. The table schema is:
 *
 *  CREATE TABLE x(item TEXT,
 *                 input TEXT HIDDEN,
 *                 pattern TEXT HIDDEN,
 *                 timeout INTEGER HIDDEN
 *  );
 *
 * Usage:
 *  select item from regsplit(input, pattern[, timeout]);
 *
 * Like the csv.c extension, the xBestIndex and xFilter functions just do a
 * full table scan, because anything else for this situation is rather
 * pointless.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_REGEX

/* Notes in "utilext.c" */
#pragma warning( disable : 4339 4514 )
#ifdef NDEBUG
#pragma warning( disable : 4100)
#endif

#pragma warning( disable : 4820 )
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sqlite3ext.h"
#include "RegexExt.h"

/* _INIT1 gets evaluated in functions.c */
SQLITE_EXTENSION_INIT3

typedef UtilityExtensions::RegexExt RegExt;

#pragma warning( push )
#pragma warning( disable : 4820 ) /* struct padding added */

/* Subclass the sqlite3_vtab_cursor base class */
typedef struct splitvtab_cursor splitvtab_cursor;
struct splitvtab_cursor {
  sqlite3_vtab_cursor base; /* Base class - must be first */
  DbStrArr data;            /* split results */
  char *zInput;             /* input arg */
  char *zPattern;           /* pattern arg */
  int timeout;              /* timeout arg */
  int index;                /* current index into the results array */
  bool isSet;               /* the split has been performed */
};

#pragma warning ( pop ) /* 4820 */


/* the constructor for splitvtab_vtab objects.
**
** All this routine needs to do is:
**
**    (1) Allocate the splitvtab_vtab object and initialize all fields.
**
**    (2) Tell SQLite (via the sqlite3_declare_vtab() interface) what the
**        result set of queries against the virtual table will look like.
*/
static int splitvtabConnect(sqlite3 *db,
                            void *pAux,
                            int argc,
                            const char *const*argv,
                            sqlite3_vtab **ppVtab,
                            char **pzErr)
{
  _CRT_UNUSED(pzErr);
  _CRT_UNUSED(pAux);
  _CRT_UNUSED(argc);
  _CRT_UNUSED(argv);
  sqlite3_vtab *pNew;
  int rc = sqlite3_declare_vtab(db,
                                "CREATE TABLE x("
                                "  item    TEXT,"
                                "  input   TEXT    HIDDEN,"
                                "  pattern TEXT    HIDDEN,"
                                "  timeout INTEGER HIDDEN"
                                ")");
  if (rc) return rc;
  /* we don't need to subclass the base table object, since our cursor maintains
  ** state for each function call
  */
  pNew = (sqlite3_vtab*)sqlite3_malloc(sizeof(*pNew));
  *ppVtab = pNew;
  if (pNew == nullptr) return SQLITE_NOMEM;
  memset(pNew, 0, sizeof(*pNew));
  return rc;
}

/* This method is the destructor for splitvtab_vtab objects. */
static int splitvtabDisconnect(sqlite3_vtab *pVtab) {
  sqlite3_free(pVtab);
  return SQLITE_OK;
}

/* Constructor for a new splitvtab_cursor object. */
static int splitvtabOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor) {
  _CRT_UNUSED(p);
  splitvtab_cursor *pCur = (splitvtab_cursor*)sqlite3_malloc(sizeof(*pCur));
  if (pCur == nullptr) return SQLITE_NOMEM;
  memset(pCur, 0, sizeof(*pCur));
  pCur->index = -1; /* start before the first row */
  *ppCursor = &pCur->base;
  return SQLITE_OK;
}

/* Destructor for a splitvtab_cursor. */
static int splitvtabClose(sqlite3_vtab_cursor *cur) {
  splitvtab_cursor *pCur = (splitvtab_cursor*)cur;
  free(pCur->zInput);
  free(pCur->zPattern);
  sqlite3_free(pCur);
  return SQLITE_OK;
}

/* Advance a splitvtab_cursor to its next row of output */
static int splitvtabNext(sqlite3_vtab_cursor *cur) {
  ((splitvtab_cursor*)cur)->index++;
  return SQLITE_OK;
}

/* Indexes for the table columns */
#define SPLIT_COL_ITEM    0
#define SPLIT_COL_INPUT   1
#define SPLIT_COL_PATTERN 2
#define SPLIT_COL_TIMEOUT 3

/* Return the column value for the specified column */
static int splitvtabColumn(sqlite3_vtab_cursor *cur,
                           sqlite3_context *ctx,
                           int i)
{
  splitvtab_cursor *pCur = (splitvtab_cursor*)cur;
  char *zValue = nullptr;
  if (i == SPLIT_COL_TIMEOUT) {
    sqlite3_result_int(ctx, pCur->timeout);
  }
  else {
    switch (i) {
      case SPLIT_COL_ITEM:
        zValue = pCur->data.pArr[pCur->index];
        break;
      case SPLIT_COL_INPUT:
        zValue = pCur->zInput;
        break;
      case SPLIT_COL_PATTERN:
        zValue = pCur->zPattern;
        break;
    }
    sqlite3_result_text(ctx, zValue, -1, SQLITE_TRANSIENT);
  }
  return SQLITE_OK;
}

/* Return the rowid for the current row. In this implementation, the rowid is
** the current index into the result array.
*/
static int splitvtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid) {
  *pRowid = ((splitvtab_cursor*)cur)->index;
  return SQLITE_OK;
}

/* Return TRUE if the cursor has been moved off of the last row of output. */
static int splitvtabEof(sqlite3_vtab_cursor *cur) {
  splitvtab_cursor *pCur = (splitvtab_cursor*)cur;

  return pCur->index >= pCur->data.n || pCur->index < 0;
}

/* Set up the results of splitting the input and move to the first row of the
** result set.
*/
static int splitvtabFilter(sqlite3_vtab_cursor *pVtabCursor,
                           int idxNum,
                           const char *idxStr,
                           int argc,
                           sqlite3_value **argv)
{
  _CRT_UNUSED(idxStr);
  assert(idxNum >= 3 && idxNum <= 7);
  DbStrArr result;
  char *zError;
  int rc;
  DbStr input = { 0 };
  DbStr pattern = { 0 };
  int ms = -1;
  splitvtab_cursor *pCur = (splitvtab_cursor*)pVtabCursor;
  if (!pCur->isSet) {
    /* return no rows if any argument is null */
    for (int i = 0; i < argc; i++) {
      if (sqlite3_value_type(argv[i]) == SQLITE_NULL) {
        pCur->index = -1;
        pCur->isSet = true;
        return SQLITE_OK;
      }
    }
    input.pText = sqlite3_value_text(argv[0]);
    input.cb = sqlite3_value_bytes(argv[0]);
    pattern.pText = sqlite3_value_text(argv[1]);
    pattern.cb = sqlite3_value_bytes(argv[1]);
    if (idxNum & 4) {
      ms = sqlite3_value_int(argv[2]);
    }
    rc = RegExt::Regsplit(&input, &pattern, ms, &zError, &result);
    switch (rc) {
      case ERR_NOMEM:
        return SQLITE_NOMEM;
      case ERR_REGEX_PARSE:
        pVtabCursor->pVtab->zErrMsg = sqlite3_mprintf("%s", zError);
        free(zError);
        return SQLITE_ERROR;
      case ERR_REGEX_TIMEOUT:
        return SQLITE_ABORT;
    }
    pCur->zInput = _strdup((char*)input.pText);
    pCur->zPattern = _strdup((char*)pattern.pText);
    pCur->timeout = ms;
    pCur->data = result;
    pCur->isSet = true;
  }
  pCur->index = 0;
  return SQLITE_OK;
}

/* The cost estimate reflects a full table scan only; The arguments to xFilter
** are [0] input string, [1] pattern string, and [2] timeout in ms.
*/
static int splitvtabBestIndex(sqlite3_vtab *tab,
                              sqlite3_index_info *pIdxInfo)
{
  _CRT_UNUSED(tab);
  sqlite3_index_info::sqlite3_index_constraint *pConstraint;
  int idxNum = 0; /* bitmask for arg values */
  int nArg = 0; /* how many args are passed to xFilter */
  if (pIdxInfo->nConstraint < 2) {
    tab->zErrMsg = sqlite3_mprintf("not enough arguments on regsplit() - min 2");
    return SQLITE_ERROR;
  }
  pConstraint = pIdxInfo->aConstraint;
  for (int i = 0; i < pIdxInfo->nConstraint; i++, pConstraint++) {
    if (pConstraint->iColumn < SPLIT_COL_INPUT) continue;
    if (!pConstraint->usable) return SQLITE_CONSTRAINT;
    if (pConstraint->op == SQLITE_INDEX_CONSTRAINT_EQ) {
      int iCol = pConstraint->iColumn - SPLIT_COL_INPUT;
      /* 0 input; 1 pattern; 2 timeout */
      assert(iCol >= 0 && iCol <= 2);
      idxNum |= 1 << iCol;
      pIdxInfo->aConstraintUsage[i].argvIndex = ++nArg;
      pIdxInfo->aConstraintUsage[i].omit = 1;
    }
  }
  /* estimated cost is swiped from csv.c, which is a constant cost for a full-
  ** table scan. I don't know that it will take a million CPU cycles for a
  ** simple split operation, but the specific cost is not important.
  */
  pIdxInfo->estimatedCost = 1000000;
  pIdxInfo->idxNum = idxNum;
  return SQLITE_OK;
}

/* Virtual table structure */
struct sqlite3_module splitvtabModule = {
  /* iVersion    */ 0,
  /* xCreate     */ 0,
  /* xConnect    */ splitvtabConnect,
  /* xBestIndex  */ splitvtabBestIndex,
  /* xDisconnect */ splitvtabDisconnect,
  /* xDestroy    */ 0,
  /* xOpen       */ splitvtabOpen,
  /* xClose      */ splitvtabClose,
  /* xFilter     */ splitvtabFilter,
  /* xNext       */ splitvtabNext,
  /* xEof        */ splitvtabEof,
  /* xColumn     */ splitvtabColumn,
  /* xRowid      */ splitvtabRowid,
  /* xUpdate     */ 0,
  /* xBegin      */ 0,
  /* xSync       */ 0,
  /* xCommit     */ 0,
  /* xRollback   */ 0,
  /* xFindMethod */ 0,
  /* xRename     */ 0,
  /* xSavepoint  */ 0,
  /* xRelease    */ 0,
  /* xRollbackTo */ 0,
  /* xShadowName */ 0
};

#endif /* !UTILEXT_OMIT_REGEX */
