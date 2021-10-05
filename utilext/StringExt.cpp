/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * StringExt class implementation.
 *
 * Implements the string-handling convenience functions, as well as the Unicode
 * collation sequences and the Unicode 'like' function.
 *
 * The 'like' comparison is ported directly from the "func.c" file in the sqlite
 * source repo, with one major difference: the combined 'glob' comparison is
 * omitted. For matching character sequences 'glob-fashion', we use the REGEXP
 * function. The GLOB function in the sqlite core is unaffected.
 *
 * Note: if we ever wanted to port this library to .NET 5 or later, we would
 * have to make sure that the switch from NLS to ICU is both compatible with
 * existing code, and produces the same results.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_STRING

#include <assert.h>
#include <string.h>
#include "StringExt.h"

using namespace System;
using namespace System::Globalization;
using namespace System::Text;
using namespace System::Threading;

using EXT = UtilityExtensions::StringExt;

namespace UtilityExtensions {

  int EXT::CharIndex(DbStr *pIn,
                     DbStr *pPattern,
                     int index,
                     bool isWide,
                     bool noCase,
                     int *pResult)
  {
    int result = -1;
    assert(index > 0); // should start out greater than zero

    index--; // adjust back to zero-based index
    String^ source = Common::GetString(pIn, isWide);
    String^ pattern = Common::GetString(pPattern, isWide);
    StringInfo^ srcInfo = gcnew StringInfo(source);
    CompareOptions opt = noCase ? CompareOptions::IgnoreCase :
                                  CompareOptions::None;
    CompareInfo^ ci = Common::Culture->CompareInfo;
    int lte = srcInfo->LengthInTextElements;
    if (lte == source->Length) {
      // grapheme indexes are the same as char indexes, so do it the easy way
      if (index > 0 && index >= source->Length) {
        return ERR_INDEX;
      }
      result = ci->IndexOf(source, pattern, index, opt);
    }
    else {
      if (index > 0 && index >= lte) {
        return ERR_INDEX;
      }
      array<int>^ indices = StringInfo::ParseCombiningCharacters(source);
      int idx = ci->IndexOf(source, pattern, index, opt);
      if (idx > 0) {
        for (int i = 0; i < indices->Length - 1; i++) {
          if (idx == indices[i]) {
            result = i;
            break;
          }
        }
      }
      else {
        result = idx;
      }
    }
    *pResult = result + 1;
    return RESULT_OK;
  }
    
  int EXT::ExFilter(DbStr *pIn,
                    DbStr *pMatch,
                    bool isWide,
                    bool noCase,
                    void **ppResult)
  {
    StringBuilder^ sb = nullptr;
    String^ result = nullptr;
    String^ input = Common::GetString(pIn, isWide);
    String^ match = Common::GetString(pMatch, isWide);
    if (input->Length == 0) {
      return Common::SetString("", isWide, ppResult);
    }
    if (match->Length == 0) {
      return Common::SetString(input, isWide, ppResult);
    }
    sb = gcnew StringBuilder(input->Length);
    StringInfo^ srcInfo = gcnew StringInfo(input);
    StringInfo^ mchInfo = gcnew StringInfo(match);
    if (srcInfo->LengthInTextElements == input->Length &&
        mchInfo->LengthInTextElements == match->Length)
    {
      CompareOptions cmpOpt = noCase ? CompareOptions::IgnoreCase :
                                       CompareOptions::None;
      array<wchar_t>^ source = input->ToCharArray();
      CompareInfo^ ci = Common::Culture->CompareInfo;
      for (int i = 0; i != source->Length; i++) {
        if (ci->IndexOf(match, source[i], cmpOpt) < 0) {
          sb->Append(source[i]);
        }
      }
    }
    else {
      array<String^>^ srcChars = nullptr;
      array<String^>^ ptChars = nullptr;
      if (noCase) {
        srcChars = parseGraphemes(input->ToLower(Common::Culture));
        ptChars = parseGraphemes(match->ToLower(Common::Culture));
      }
      else {
        srcChars = parseGraphemes(input);
        ptChars = parseGraphemes(match);
      }
      for (int i = 0; i < srcChars->Length; i++) {
        if (Array::IndexOf<String^>(ptChars, srcChars[i]) < 0) {
          sb->Append(srcChars[i]);
        }
      }
    }
    result = sb->ToString();
    return Common::SetString(result, isWide, ppResult);
  }

  int EXT::InFilter(DbStr *pIn,
                    DbStr *pMatch,
                    bool isWide,
                    bool noCase,
                    void **ppResult)
  {
    StringBuilder^ sb = nullptr;
    String^ result = nullptr;
    String^ input = Common::GetString(pIn, isWide);
    String^ match = Common::GetString(pMatch, isWide);
    if ((input->Length == 0) || (match->Length == 0)) {
      return Common::SetString("", isWide, ppResult);
    }
    sb = gcnew StringBuilder(input->Length);
    StringInfo^ srcInfo = gcnew StringInfo(input);
    StringInfo^ mchInfo = gcnew StringInfo(match);
    if (srcInfo->LengthInTextElements == input->Length &&
        mchInfo->LengthInTextElements == match->Length)
    {
      CompareOptions cmpOpt = noCase ? CompareOptions::IgnoreCase :
                                       CompareOptions::None;
      array<wchar_t>^ source = input->ToCharArray();
      CompareInfo^ ci = Common::Culture->CompareInfo;
      for (int i = 0; i != source->Length; i++) {
        if (ci->IndexOf(match, source[i], cmpOpt) >= 0) {
          sb->Append(source[i]);
        }
      }
    }
    else {
      array<String^>^ srcChars = nullptr;
      array<String^>^ ptChars = nullptr;
      if (noCase) {
        srcChars = parseGraphemes(input->ToLower(Common::Culture));
        ptChars = parseGraphemes(match->ToLower(Common::Culture));
      }
      else {
        srcChars = parseGraphemes(input);
        ptChars = parseGraphemes(match);
      }
      for (int i = 0; i < srcChars->Length; i++) {
        if (Array::IndexOf<String^>(ptChars, srcChars[i]) >= 0) {
          sb->Append(srcChars[i]);
        }
      }
    }

    result = sb->ToString();
    return Common::SetString(result, isWide, ppResult);
  }

  int EXT::Join(int argc, DbStr *aValues, bool isWide, void **ppResult) {
    assert(argc >= 3);
    // first element in aValues is the separator
    assert(aValues && aValues[0].pText);
    String^ sep = Common::GetString(aValues[0].pText, aValues[0].cb, isWide);
    int len = argc - 1;
    array<String^>^ inputs = gcnew array<String^>(len);
    for (int i = 0, j = 1; i < len; i++, j++) {
      assert(aValues[j].pText);
      inputs[i] = Common::GetString(aValues[j].pText, aValues[j].cb, isWide);
    }
    return Common::SetString(String::Join(sep, inputs), isWide, ppResult);
  }

  int EXT::LeftString(DbStr *pIn, int count, bool isWide, void **ppResult) {
    assert(count >= 0);
    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    if (count >= si->LengthInTextElements) {
      return Common::SetString(input, isWide, ppResult);
    }
    else {
      String^ result = si->SubstringByTextElements(0, count);
      return Common::SetString(result, isWide, ppResult);
    }
  }

  int EXT::Like(DbStr *pIn,
                DbStr *pPattern,
                DbStr *pEscape,
                bool isWide,
                bool noCase,
                int *result)
  {
    String^ esc = nullptr;
    if (pEscape) {
      esc = Common::GetString(pEscape, isWide);
      StringInfo^ si = gcnew StringInfo(esc);
      if (si->LengthInTextElements != 1) return ERR_ESC_LENGTH;
    }
    String^ pattern = Common::GetString(pPattern, isWide);
    array<String^>^ aPattern = parseGraphemes(pattern);
    String^ input = Common::GetString(pIn, isWide);
    array<String^>^ aInput = parseGraphemes(input);
    CmpState^ state = gcnew CmpState(aInput->Length, aPattern->Length, noCase);
    *result = likeCompare(aInput, aPattern, esc, state) ? 1 : 0;
    return RESULT_OK;
  }

  int EXT::PadCenter(DbStr *pIn, int len, bool isWide, void **ppResult) {
    String^ result = nullptr;
    assert(len >= 0);

    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    int cLen = si->LengthInTextElements;
    if (cLen >= len) {
      result = input;
    }
    else {
      int lenAdj = input->Length - cLen;
      int totalPad = len + lenAdj - input->Length;
      result = input->PadLeft(input->Length + (totalPad / 2))->PadRight(input->Length + totalPad);
    }
    return Common::SetString(result, isWide, ppResult);
  }

  int EXT::PadLeft(DbStr *pIn, int len, bool isWide, void **ppResult) {
    String^ result = nullptr;
    assert(len >= 0);

    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    int cLen = si->LengthInTextElements;
    if (cLen >= len) {
      return Common::SetString(input, isWide, ppResult);
    }
    else {
      len += input->Length - cLen;
      result = input->PadLeft(len);
      return Common::SetString(result, isWide, ppResult);
    }
  }

  int EXT::PadRight(DbStr *pIn, int len, bool isWide, void **ppResult) {
    String^ result = nullptr;
    assert(len >= 0);

    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    int cLen = si->LengthInTextElements;
    if (cLen >= len) {
      return Common::SetString(input, isWide, ppResult);
    }
    else {
      len += input->Length - cLen;
      result = input->PadRight(len);
      return Common::SetString(result, isWide, ppResult);
    }
  }

  int EXT::Replicate(DbStr *pIn, int count, bool isWide, void **ppResult) {
    String^ result = nullptr;
    int destCursor = 0;
    assert(count >= 0);

    if (count == 0) {
      return Common::SetString("", isWide, ppResult);
    }
    String^ input = Common::GetString(pIn, isWide);
    int len = input->Length;
    array<wchar_t>^ resultchars = gcnew array<wchar_t>(len * count);
    for (int i = 0; i < count; i++) {
      input->CopyTo(0, resultchars, destCursor, len);
      destCursor += len;
    }
    result = gcnew String(resultchars);
    return Common::SetString(result, isWide, ppResult);
  }

  int EXT::Reverse(DbStr *pIn, bool isWide, void **ppResult) {
    String^ result = nullptr;
    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    if (si->LengthInTextElements == input->Length) {
      array<wchar_t>^ chars = input->ToCharArray();
      Array::Reverse(chars);
      result = gcnew String(chars);
    }
    else {
      array<String^>^ gchars = parseGraphemes(input);
      StringBuilder^ sb = gcnew StringBuilder(input->Length);
      for (int i = gchars->Length - 1; i >= 0; i--) {
        sb->Append(gchars[i]);
      }
      result = sb->ToString();
    }

    return Common::SetString(result, isWide, ppResult);
  }

  int EXT::RightString(DbStr *pIn, int count, bool isWide, void **ppResult) {
    assert(count >= 0);
    String^ input = Common::GetString(pIn, isWide);
    StringInfo^ si = gcnew StringInfo(input);
    if (count >= si->LengthInTextElements) {
      return Common::SetString(input, isWide, ppResult);
    }
    else {
      String^ result = si->SubstringByTextElements(si->LengthInTextElements -
                                                   count);
      return Common::SetString(result, isWide, ppResult);
    }
  }

  int EXT::UpperLower(DbStr *pIn, bool isWide, bool upper, void **ppResult) {
    String^ input = Common::GetString(pIn, isWide);
    if (input->Length == 0) {
      return Common::SetString("", isWide, ppResult);
    }
    TextInfo^ ti = Common::Culture->TextInfo;
    if (upper) {
      return Common::SetString(ti->ToUpper(input), isWide, ppResult);
    }
    else {
      return Common::SetString(ti->ToLower(input), isWide, ppResult);
    }
  }

  int EXT::SetCulture(const void *zIn, int cbIn, bool isWide, int *previous) {
    int err;
    if (zIn) {
      String^ lc = Common::GetString(zIn, cbIn, isWide);
      err = Common::SetCultureInfo(lc, previous);
    }
    else {
      *previous = Common::Culture->LCID;
      err = RESULT_OK;
    }
    return err;
  }

  int EXT::UtfCollate(DbStr *pLeft,
                      DbStr *pRight,
                      bool isWide,
                      bool noCase)
  {
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
    if (noCase) {
      return Common::Culture->CompareInfo->Compare(sLeft,
                                                   sRight,
                                                   CompareOptions::IgnoreCase);
    }
    else {
      return Common::Culture->CompareInfo->Compare(sLeft, sRight);
    }
  }

  array<String^>^ EXT::parseGraphemes(String^ input) {
    assert(input != nullptr);
    array<int>^ indices = StringInfo::ParseCombiningCharacters(input);
    array<String^>^ result = gcnew array<String^>(indices->Length);
    int end = input->Length;
    for (int i = input->Length - 1; i >= 0; i--) {
      int start = indices[i];
      int len = end - start;
      result[i] = input->Substring(start, len);
      end = start;
    }
    return result;
  }

  bool EXT::areEqual(String^ left, String^ right, bool noCase) {
    CompareOptions opt = CompareOptions::None;
    if (noCase) opt = CompareOptions::IgnoreCase;
    return Common::Culture->CompareInfo->Compare(left, right, opt) == 0;
  }

  bool EXT::likeCompare(array<String^>^ input,
                        array<String^>^ pattern,
                        String^ escape,
                        CmpState^ state)
  {
    // Here we are reproducing the likeCompare() implementation from func.c
    // in the SQLite core; we leave out the GLOB handling, since we handle
    // character sequences with REGEXP. This routine is a touch slower than
    // the core, since we are dealing with graphemes as strings in all cases.
    String^ cp = nullptr;
    String^ cs = nullptr;
    String^ ct = nullptr;
    int escIdx = 0;
    while ((cp = readPtrn(pattern, state)) != nullptr) {
      if (cp == "%") {
        // SQLite ticket [c0aeea67d58ae0fd] does not apply; we are not using
        // the "matchOne" variable (for globbing), so we don't have to test for
        // that variable being non-zero in the second boolean clause
        while ((cp = readPtrn(pattern, state)) == "%" || cp == "_") {
          if (cp == "_" && (ct = readSrc(input, state)) == nullptr) {
            return false;
          }
        }
        if (cp == nullptr) {
          return true; // % at end of pattern is a match
        }
        else if (areEqual(cp, escape, state->NoCase)) {
          cp = readPtrn(pattern, state);
          if (cp == nullptr) {
            return false;
          }
        }
        while ((cs = readSrc(input, state)) != nullptr) {
          if (!areEqual(cs, cp, state->NoCase)) {
            continue;
          }
          if (likeCompare(input, pattern, escape, state)) {
            return true;
          }
        }
        return false;
      }
      if (areEqual(cp, escape, state->NoCase)) {
        cp = readPtrn(pattern, state);
        if (cp == nullptr) {
          return false;
        }
        escIdx = state->PtrnIdx;
      }
      cs = readSrc(input, state);
      if (areEqual(cs, cp, state->NoCase)) {
        continue;
      }
      if (cp == "_" && escIdx != state->PtrnIdx && cs != nullptr) {
        continue;
      }
      return false;
    }
    return state->SrcIdx >= state->SrcLen ? true : false;
  }
}

#endif /* !UTILEXT_OMIT_STRING */
