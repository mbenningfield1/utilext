/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * RegexExt class implementation.
 *
 * We are wrapping only 3 Regex functions: Regex.Match(), Regex.Replace(), and
 * Regex.Split(). 
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_REGEX

#include <assert.h>
#include "RegexExt.h"

using namespace System;
using namespace System::Text::RegularExpressions;

using REX = UtilityExtensions::RegexExt;

namespace UtilityExtensions {

  int REX::Regexp(DbStr *pIn,
                  DbStr *pPattern,
                  int ms,
                  char **zError,
                  int *pResult)
  {
    assert(pIn);
    assert(pPattern);
    assert(pResult);
    int rc = RESULT_OK;
    TimeSpan ts = (ms <= 0) ? Regex::InfiniteMatchTimeout
                            : TimeSpan(ms * TimeSpan::TicksPerMillisecond);

    String^ input = Common::GetString(pIn);
    String^ pattern = Common::GetString(pPattern);
    bool flag = false;
    try {
      flag = Regex::IsMatch(input, pattern, RegexOptions::None, ts);
      *pResult = flag ? 1 : 0;
      return RESULT_OK;
    }
    catch (ArgumentException^ ex) {
      rc = Common::SetErrorString(ex->Message, zError);
      if (rc == RESULT_OK) {
        return ERR_REGEX_PARSE;
      }
      else {
        return rc;
      }
    }
    catch (RegexMatchTimeoutException^) {
      return ERR_REGEX_TIMEOUT;
    }
  }

  int REX::Regsub(DbStr *pSource,
                  DbStr *pPattern,
                  DbStr *pSub,
                  int ms,
                  char **zError,
                  DbStr *pResult)
  {
    assert(pSource);
    assert(pPattern);
    assert(pSub);
    int rc = RESULT_OK;
    TimeSpan ts = (ms <= 0) ? Regex::InfiniteMatchTimeout
                            : TimeSpan(ms * TimeSpan::TicksPerMillisecond);

    String^ source = Common::GetString(pSource);
    String^ pattern = Common::GetString(pPattern);
    String^ sub = Common::GetString(pSub);
    String^ result = nullptr;
    try {
      result = Regex::Replace(source, pattern, sub, RegexOptions::None, ts);
      return Common::SetString(result, pSource->isWide, pResult);
    }
    catch (ArgumentException^ ex) {
      rc = Common::SetErrorString(ex->Message, zError);
      if (rc == RESULT_OK) {
        return ERR_REGEX_PARSE;
      }
      else {
        return rc;
      }
    }
    catch (RegexMatchTimeoutException^) {
      return ERR_REGEX_TIMEOUT;
    }
  }

  int REX::Regsplit(DbStr *pSource,
                    DbStr *pPattern,
                    int ms,
                    char **zError,
                    DbStrArr *pResult)
  {
    assert(pSource);
    assert(pPattern);
    int rc = RESULT_OK;
    TimeSpan ts = (ms <= 0) ? Regex::InfiniteMatchTimeout
                            : TimeSpan(ms * TimeSpan::TicksPerMillisecond);

    // table-valued function text arguments are always retrieved as UTF-8.
    String^ source = Common::GetString(pSource);
    String^ pattern = Common::GetString(pPattern);
    array<String^>^ items = nullptr;
    try {
      items = Regex::Split(source, pattern, RegexOptions::None, ts);
      return Common::SetStringArray(items, pResult);
    }
    catch (ArgumentException^ ex) {
      rc = Common::SetErrorString(ex->Message, zError);
      if (rc == RESULT_OK) {
        return ERR_REGEX_PARSE;
      }
      else {
        return rc;
      }
    }
    catch (RegexMatchTimeoutException^) {
      return ERR_REGEX_TIMEOUT;
    }
  }
}

#endif // !UTILEXT_OMIT_REGEX
