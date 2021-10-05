/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static RegexExt class.
 *
 *============================================================================*/

#pragma once

#include "Common.h"

using namespace System;
using namespace System::Text::RegularExpressions;
using namespace System::Collections::Generic;

namespace UtilityExtensions {

  ref class RegexExt abstract sealed {

  internal:
    /// <summary>
    /// Tests the specified string for a match using a regular expression based
    /// on the specified pattern.
    /// </summary>
    /// <param name="pIn">Pointer to a native string</param>
    /// <param name="pPattern">Pointer to a native pattern string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="ms">Timeout interval in milliseconds</param>
    /// <param name="zError">Pointer to hold any error message</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the boolean integer result of
    /// the match is written to <paramref name="pResult"/>.
    /// </returns>
    static int Regexp(
      DbStr *pIn,
      DbStr *pPattern,
      bool isWide,
      int ms,
      char **zError,
      int *pResult
    );

    /// <summary>
    /// Replaces text in the source string that matches the regular expression
    /// pattern with the specified substitution string.
    /// </summary>
    /// <param name="pSource">Pointer to a native string</param>
    /// <param name="pPattern">Pointer to a native pattern string</param>
    /// <param name="pSub">Pointer to a native substitution string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="ms">Timeout interval in milliseconds</param>
    /// <param name="zError">Pointer to hold any error message</param>
    /// <param name="ppResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the a string is allocated and
    /// assigned to <paramref name="ppResult"/>. 
    /// </returns>
    static int Regsub(
      DbStr *pSource,
      DbStr *pPattern,
      DbStr *pSub,
      bool isWide,
      int ms,
      char **zError,
      void **ppResult
    );

    /// <summary>
    /// Splits the source string into a list of strings based on the specified
    /// regular expression pattern.
    /// </summary>
    /// <param name="pSource">Pointer to a native string</param>
    /// <param name="pPattern">Pointer to a native pattern string</param>
    /// <param name="ms">Timeout interval in milliseconds</param>
    /// <param name="zError">Pointer to hold any error message</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string array is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int Regsplit(
      DbStr *pSource,
      DbStr *pPattern,
      int ms,
      char **zError,
      DbStrArr *pResult
    );
  };
}