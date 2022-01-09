/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static StringExt class.
 *
 *============================================================================*/

#pragma once

#include "Common.h"

using namespace System;

namespace UtilityExtensions {

  /// <summary>
  /// Used to maintain state throughout a like comparison operation. An instance
  /// of this class is passed back and forth quite a bit, so make it a class
  /// instead of a struct so we're not doing a bunch of unnecessary copying.
  /// </summary>
  ref class CmpState {
  internal:
    int SrcIdx;
    int SrcLen;
    int PtrnIdx;
    int PtrnLen;
    bool NoCase;
    CmpState(int srcLen, int patternLen, bool noCase) {
      SrcLen = srcLen;
      PtrnLen = patternLen;
      NoCase = noCase;
    };
  };

// Macros twinned from the like comparison code in "func.c" from the sqlite core
#define readSrc(A,B) (B->SrcIdx >= B->SrcLen) ? nullptr : A[B->SrcIdx++]
#define readPtrn(A,B) (B->PtrnIdx >= B->PtrnLen) ? nullptr : A[B->PtrnIdx++]

  ref class StringExt abstract sealed {

  internal:
    /// <summary>
    /// Finds the 1-based index of the specified pattern in the specified
    /// string, starting at the specified index.
    /// </summary>
    /// <param name="pIn">Encoded native source string</param>
    /// <param name="pPattern">Encoded native pattern string</param>
    /// <param name="index">Index in <paramref name="pIn"/> to begin search</param>
    /// <param name="noCase">True to ignore case when matching</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the index is written into
    /// <paramref name="pResult"/>.
    /// </returns>
    static int CharIndex(
      DbStr *pIn,
      DbStr *pPattern,
      int index,
      bool noCase,
      int *pResult
    );

    /// <summary>
    /// Produces a string that has only the characters in the specified source
    /// string that are NOT in the specified match string.
    /// </summary>
    /// <param name="pIn">Encoded native source string</param>
    /// <param name="pMatch">Encoded native match string</param>
    /// <param name="noCase">True if the match should be case-insensitive</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int ExFilter(DbStr *pIn, DbStr *pMatch, bool noCase, DbStr *pResult);

    /// <summary>
    /// Produces a string that has only the characters in the specified source
    /// string that are ALSO in the specified match string.
    /// </summary>
    /// <param name="pIn">Encoded native source string</param>
    /// <param name="pMatch">Encoded native match string</param>
    /// <param name="noCase">True if the match should be case-insensitive</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int InFilter(DbStr *pIn, DbStr *pMatch, bool noCase, DbStr *pResult);

    /// <summary>
    /// Produces a string that concatenates all of the specified values.
    /// </summary>
    /// <param name="argc">The count of values in the
    /// <paramref name="aValues"/> array</param>
    /// <param name="aValues">Array of DbStr objects</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int Join(int argc, DbStr *aValues, DbStr *pResult);

    /// <summary>
    /// Produces a string that contains the specified number of characters from
    /// the start of the specified string.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="count">The number of characters to include</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int LeftString(DbStr *pIn, int count, DbStr *pResult);

    /// <summary>
    /// Overrides the built-in 'like()' SQL function to provide Unicode case-
    /// folding.
    /// </summary>
    /// <param name="pIn">Encoded native input string</param>
    /// <param name="pPattern">Encoded native pattern string</param>
    /// <param name="pEscape">Encoded native escape character string</param>
    /// <param name="noCase">True to ignore case when comparing</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the boolean result of the match
    /// is written as an integer into <paramref name="pResult"/>.
    /// </returns>
    static int Like(
      DbStr *pIn,
      DbStr *pPattern,
      DbStr *pEscape,
      bool noCase,
      int *pResult
    );

    /// <summary>
    /// Produces a string that pads the specified string with spaces at the
    /// beginning and end to equal the specified length.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="len">The desired length of the result</param>
    /// <param name="pResult">Pointer to hold the result.</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int PadCenter(DbStr *pIn, int len, DbStr *pResult);

    /// <summary>
    /// Produces a string that pads the specified string with spaces at the
    /// beginning to equal the specified length.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="len">The desired length of the result</param>
    /// <param name="pResult">Pointer to hold the result.</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int PadLeft(DbStr *pIn, int len, DbStr *pResult);

    /// <summary>
    /// Produces a string that pads the specified string with spaces at the end
    /// to equal the specified length.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="len">The desired length of the result</param>
    /// <param name="pResult">Pointer to hold the result.</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int PadRight(DbStr *pIn, int len, DbStr *pResult);

    /// <summary>
    /// Produces a string that consists of the specified string repeated the
    /// specified number of times.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="count">The number of times to repeat the string</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int Replicate(DbStr *pIn, int count, DbStr *pResult);

    /// <summary>
    /// Reverses the order of characters in the specified string.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigend to <paramref name="pResult"/>.
    /// </returns>
    static int Reverse(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Produces a string that contains the specified number of characters from
    /// the end of the specified string.
    /// </summary>
    /// <param name="pIn">Encoded native string</param>
    /// <param name="count">The number of characters to include</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int RightString(DbStr *pIn, int count, DbStr *pResult);

    /// <summary>
    /// Sets the CultureInfo to the specified identifier.
    /// </summary>
    /// <param name="pIn">Pointer to UTF8 native string that identifies the
    /// desired culture</param>
    /// <param name="previous">Pointer to hold the previous culture identifier</param>
    /// <returns>
    /// An integer result code. If successful, the LCID of the previous culture
    /// is written to <paramref name="previous"/>.
    /// </returns>
    /// <remarks>
    /// The <paramref name="pIn"/> argument can be in the form of "0xHHHH" or
    /// "NNNNN" for a recognized NLS integer LCID value, or in the form "xx-XX"
    /// for a recognized NLS locale name.
    /// <para>
    /// If <paramref name="pIn"/> is an empty string, the InvariantCulture is
    /// used.
    /// </para>
    /// <para>
    /// If <paramref name="pIn"/> is NULL, then the LCID of the current culture
    /// is written to <paramref name="previous"/>, and no change is made.
    /// </para>
    /// </remarks>
    static int SetCulture(DbStr *pIn, int *previous);

    /// <summary>
    /// Converts the specified string to upper- or lower-case.
    /// </summary>
    /// <param name="pIn">Encoded string input</param>
    /// <param name="upper">True if converting to upper case</param>
    /// <param name="pResult">Pointer to hold the resulting string</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and
    /// assigned to <paramref name="pResult"/>.
    /// </returns>
    static int UpperLower(DbStr *pIn, bool upper, DbStr *pResult);

    /// <summary>
    /// Impements the Unicode 'utf' collation sequence.
    /// </summary>
    /// <param name="pLeft">Encoded native string (lhs)</param>
    /// <param name="pRight">Encoded native string (rhs)</param>
    /// <param name="noCase">True if the comparison should be case-insensitive</param>
    /// <returns>
    /// The integer result of the comparison.
    /// </returns>
    static int UtfCollate(DbStr *pLeft, DbStr *pRight, bool noCase);

  private:
    static array<String^>^ parseGraphemes(String^ input);

    static bool likeCompare(
      array<String^>^ input,
      array<String^>^ pattern,
      String^ escape,
      CmpState^ state
    );

    static bool areEqual(String^ left, String^ right, bool noCase);
  };
}
