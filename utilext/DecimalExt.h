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
    /// <param name="pResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalAbs(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps decimal addition.
    /// </summary>
    /// <param name="argc">The count of arguments in the
    /// <paramref name="aValues"/> array</param>
    /// <param name="aValues">Array of DbStr objects</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalAdd(int argc, DbStr *aValues, DbStr *pResult);

    /// <summary>
    /// Calculates the average of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the
    /// <paramref name="aValues"/> array</param>
    /// <param name="aValues">Array of DbStr objects</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalAverageAny(int argc, DbStr *aValues, DbStr *pResult);

    /// <summary>
    /// Implements the xFinal() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalAverageFinal(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalAverageInverse(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalAverageStep(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate dec_avg() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalAverageValue(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Wraps the Decimal.Ceiling() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalCeiling(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Implements the 'decimal' collation sequence.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <returns>
    /// The integer comparison result.
    /// </returns>
    static int DecimalCollate(DbStr *pLeft, DbStr *pRight);

    /// <summary>
    /// Wraps the Decimal.Compare() method.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="pResult">Pointer to hold the comparison result</param>
    /// <returns>
    /// An integer result code. If successful, writes the result of the
    /// comparison into <paramref name="pResult"/>.
    /// </returns>
    static int DecimalCompare(DbStr *pLeft, DbStr *pRight, int *pResult);

    /// <summary>
    /// Wraps decimal division.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalDivide(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Wraps the Decimal.Floor() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalFloor(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps the Math.Log() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalLog(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps the Math.Log() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="base">Base of the desired log</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalLog(DbStr *pIn, double base, DbStr *pResult);

    /// <summary>
    /// Wraps the Math.Log10() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalLog10(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Calculates the product of all arguments.
    /// </summary>
    /// <param name="argc">The count of arguments in the
    /// <paramref name="aValues"/> array</param>
    /// <param name="aValues">Array of DbStr objects</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalMultiply(int argc, DbStr *aValues, DbStr *pResult);

    /// <summary>
    /// Wraps the Decimal.Negate() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalNegate(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps the Math.Pow() function for decimal base values.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="exponent">Double exponent value</param>
    /// <param name="pResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalPower(DbStr *pIn, double exponent, DbStr *pResult);

    /// <summary>
    /// Wraps the Decimal.Remainder() method.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalRemainder(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Wraps the Math.Round() method for decimal values.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="digits">The number of digits after the </param>
    /// <param name="pMode"></param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalRound(DbStr *pIn, int digits, DbStr *pMode, DbStr *pResult);

    /// <summary>
    /// Wraps decimal subtraction.
    /// </summary>
    /// <param name="pLeft">The lhs comparison operand as text</param>
    /// <param name="pRight">The rhs comparison operand as text</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalSubtract(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Implements the xFinal() function for the aggregate dec_total() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalTotalFinal(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate dec_total() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalTotalInverse(DbStr *pIn, void *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate dec_total() SQL
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int DecimalTotalStep(DbStr *pIn, void *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate dec_total() SQL
    /// window function.
    /// </summary>
    /// <param name="pAgg">SQLite aggregate context pointer</param>
    /// <param name="isWide">True if text is desired in UTF16</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalTotalValue(void *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Wraps the Decimal.Truncate() method.
    /// </summary>
    /// <param name="pIn">Pointer to an DbStr structure that contains the
    /// decimal value as an encoded string</param>
    /// <param name="pResult">Pointer to hold the result string</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int DecimalTruncate(DbStr *pIn, DbStr *pResult);

  private:
    static Dictionary<IntPtr, Decimal>^ _values;
    static DecimalExt() {
      _values = gcnew Dictionary<IntPtr, Decimal>;
    }
    static bool parseDecimal(String^ input, Decimal% result);
    static Decimal roundDouble(double d);
  };
}
