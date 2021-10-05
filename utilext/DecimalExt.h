/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static DecimalExt class.
 *
 *============================================================================*/

#pragma once

#include "Common.h"

using namespace System;
using namespace System::Collections::Generic;

namespace UtilityExtensions {

  ref class DecimalExt abstract sealed {

  internal:
    /// <summary>
    /// Wraps the Math.Abs() method for a decimal value.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalAbs(DbStr *pIn, bool isWide, void **zResult);

    /// <summary>
    /// Wraps decimal addition.
    /// </summary>
    /// <param name="argc">The count of arguments in the 'aValues' array</param>
    /// <param name="aValues">Array of 'DbStr' objects</param>
    /// <param name="isWide">True if the strings in 'aValues' are in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalAdd(int argc, DbStr *aValues, bool isWide, void **zResult);

    /// <summary>
    /// Calculates the average of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the 'aValues' array</param>
    /// <param name="aValues">Array of 'DbStr' objects</param>
    /// <param name="isWide">True if the strings in 'aValues' are in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalAverageAny(
      int argc,
      DbStr *aValues,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Implements the xFinal() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalAverageFinal(int *pAgg, bool isWide, void **zResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalAverageInverse(DbStr *pIn, bool isWide, int *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalAverageStep(DbStr *pIn, bool isWide, int *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalAverageValue(int *pAgg, bool isWide, void **zResult);

    /// <summary>
    /// Wraps the Decimal.Ceiling() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalCeiling(DbStr *pIn, bool isWide, void **zResult);

    /// <summary>
    /// Implements the 'decimal' collation sequence.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <returns>
    /// The integer comparison result.
    /// </returns>
    static int DecimalCollate(DbStr *pLeft, DbStr *pRight, bool isWide);

    /// <summary>
    /// Wraps the Decimal.Compare() method.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="pResult">Pointer to hold the comparison result</param>
    /// <returns>
    /// An integer result code. If successful, writes the result of the
    /// comparison into <paramref name="pResult"/>.
    /// </returns>
    static int DecimalCompare(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      int *pResult
    );

    /// <summary>
    /// Wraps decimal division.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalDivide(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Wraps the Decimal.Floor() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalFloor(DbStr *pIn, bool isWide, void **zResult);

    /// <summary>
    /// Calculates the maximum value of both arguments.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalMax2(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Calculates the maximum value of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the 'aValues' array</param>
    /// <param name="aValues">Array of 'DbStr' objects</param>
    /// <param name="isWide">True if the strings in 'aValues' are in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalMaxAny(int argc, DbStr *aValues, bool isWide, void **zResult);

    /// <summary>
    /// Calculates the minimum value of both arguments.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalMin2(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Calculates the minimum value of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the 'aValues' array</param>
    /// <param name="aValues">Array of 'DbStr' objects</param>
    /// <param name="isWide">True if the strings in 'aValues' are in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalMinAny(int argc, DbStr *aValues, bool isWide, void **zResult);

    /// <summary>
    /// Calculates the product of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the 'aValues' array</param>
    /// <param name="aValues">Array of 'DbStr' objects</param>
    /// <param name="isWide">True if the strings in 'aValues' are in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalMultiply(int argc, DbStr *aValues, bool isWide,void **zResult);

    /// <summary>
    /// Wraps the Decimal.Negate() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalNegate(DbStr *pIn, bool isWide, void **zResult);

    /// <summary>
    /// Wraps the Decimal.Remainder() method.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalRemainder(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Wraps the Decimal.Round() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalRound(
      DbStr *pIn,
      bool isWide,
      int digits,
      const void *zMode,
      int cbMode,
      void **zResult
    );

    /// <summary>
    /// Wraps decimal subtraction.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalSubtract(
      DbStr *pLeft,
      DbStr *pRight,
      bool isWide,
      void **zResult
    );

    /// <summary>
    /// Implements the xFinal() function for the aggregate dec_sum() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalSumFinal(int *pAgg, bool isWide, void **zResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate dec_sum() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalSumInverse(DbStr *pIn, bool isWide, void *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate dec_sum() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalSumStep(DbStr *pIn, bool isWide, void *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate dec_sum() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="zResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalSumValue(void *pAgg, bool isWide, void **zResult);

    /// <summary>
    /// Wraps the Decimal.Truncate() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="isWide">True if text is UTF16</param>
    /// <param name="zResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="zResult"/>.
    /// </returns>
    static int DecimalTruncate(DbStr *pIn, bool isWide, void **zResult);

  private:
    static Dictionary<IntPtr, Decimal>^ _values;
    static DecimalExt() {
      _values = gcnew Dictionary<IntPtr, Decimal>;
    }
    static bool parseDecimal(String^ input, Decimal% result);
  };
}
