/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static BigIntExt class.
 *
 *============================================================================*/

#pragma once
#include "Common.h"

using namespace System::Collections::Generic;
using namespace System::Numerics;

namespace UtilityExtensions {

  ref class BigIntExt abstract sealed {

  internal:

    /// <summary>
    /// Wraps the BigInteger.Abs() method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAbs(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Multiple-argument addition.
    /// </summary>
    /// <param name="aValues">An array of DbStr objects</param>
    /// <param name="argc">Count of arguments in the <paramref name="aValues"/>
    /// array</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAdd(DbStr *aValues, int argc, DbStr *pResult);
    
    /// <summary>
    /// Wraps the bitwise-AND operator.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAnd(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Multiple argument average function.
    /// </summary>
    /// <param name="aValues">An array of DbStr objects</param>
    /// <param name="argc">Count of arguments in the <paramref name="aValues"/>
    /// array</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAverage(DbStr *aValues, int argc, DbStr *pResult);

    /// <summary>
    /// Implements the xFinal() function for the aggregate 'bigint_avg()' window
    /// function.
    /// </summary>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAverageFinal(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate 'bigint_avg()'and
    /// 'bigint_avgrem()' window functions.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int BigIntAverageInverse(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate 'bigint_avg()' and
    /// 'bigint_avgrem()' window functions.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int BigIntAverageStep(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate 'bigint_avg()' window
    /// function.
    /// </summary>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntAverageValue(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Implements the 'bigint' collation sequence.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <returns>
    /// The integer result of the comparison test.
    /// </returns>
    static int BigIntCollate(DbStr *pLeft, DbStr *pRight);

    /// <summary>
    /// Wraps the Compare() method.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to hold the comparison result</param>
    /// <returns>
    /// The integer result of the comparison test.
    /// </returns>
    static int BigIntCompare(DbStr *pLeft, DbStr *pRight, int *pResult);
        
    /// <summary>
    /// Constructor for creating a BigInteger value from a string.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntCreate(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Constructor for creating a BigInteger value from an integer.
    /// </summary>
    /// <param name="iVal">64-bit signed integer value</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntCreate(i64 iVal, bool isWide, DbStr *pResult);

    /// <summary>
    /// Constructor for creating a BigInteger value from a double.
    /// </summary>
    /// <param name="dVal">Double floating-point value to use</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntCreate(double dVal, bool isWide, DbStr *pResult);

    /// <summary>
    /// Wraps the / operator.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntDivide(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Wraps the BigInteger.GreatestCommonDivisor() method.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs argument</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs argument</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntGCD(DbStr *pLeft, DbStr *pRight, DbStr *pResult);
    
    /// <summary>
    /// Wraps the left-shift operator.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="shift">Number of bits to shift</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntLeftShift(DbStr *pIn, int shift, DbStr *pResult);

    /// <summary>
    /// Wraps the BigInteger.Log() method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to hold the log result</param>
    /// <returns>
    /// An integer result code. The log result is stored in
    /// <paramref name="pResult"/>.
    /// </returns>
    static int BigIntLog(DbStr *pIn, double *pResult);

    /// <summary>
    /// Wraps the BigInteger.Log(base) method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="base">Base of the desired log</param>
    /// <param name="pResult">Pointer to hold the log result</param>
    /// <returns>
    /// An integer result code. The log result is stored in
    /// <paramref name="pResult"/>.
    /// </returns>
    static int BigIntLog(DbStr *pIn, double base, double *pResult);

    /// <summary>
    /// Wraps the BigInteger.Log10() method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to hold the log result</param>
    /// <returns>
    /// An integer result code. The log result is stored in
    /// <paramref name="pResult"/>
    /// </returns>
    static int BigIntLog10(DbStr *pIn, double *pResult);

    /// <summary>
    /// Wraps the BigInteger.ModPow() method.
    /// </summary>
    /// <param name="aValues">An array of DbStr objects</param>
    /// <param name="argc">Count of arguments in the 'aValues' array</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntModPow(DbStr *aValues, int argc, DbStr *pResult);

    /// <summary>
    /// Multiple-argument multiplication.
    /// </summary>
    /// <param name="aValues">An array of DbStr objects</param>
    /// <param name="argc">Count of arguments in the 'aValues' array</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntMultiply(DbStr *aValues, int argc, DbStr *pResult);

    /// <summary>
    /// Wraps the - operator.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntNegate(DbStr *pIn, DbStr *pResult);
    
    /// <summary>
    /// Wraps the ~ operator.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntNot(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps the | operator.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntOr(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Wraps the BigInteger.Pow() method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="exp">Exponent to use</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntPow(DbStr *pIn, int exp, DbStr *pResult);

    /// <summary>
    /// Wraps the % operator.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntRemainder(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Wraps the right-shift operator.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="shift">Number of bits to shift</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntRightShift(DbStr *pIn, int shift, DbStr *pResult);

    /// <summary>
    /// Wraps the BigInteger.ToString() method.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pResult">Pointer to hold the string result</param>
    /// <returns>
    /// An integer result code. If successful, allocates and assigns a string to
    /// <paramref name="pResult"/>.
    /// </returns>
    static int BigIntString(DbStr *pIn, DbStr *pResult);

    /// <summary>
    /// Wraps the - operator.
    /// </summary>
    /// <param name="pLeft">Pointer to a DbStr structure holding the
    /// BigInteger lhs operand</param>
    /// <param name="pRight">Pointer to a DbStr structure holding the
    /// BigInteger rhs operand</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is
    /// allocated and stored in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntSubtract(DbStr *pLeft, DbStr *pRight, DbStr *pResult);

    /// <summary>
    /// Implements the xFinal() function for the aggregate 'bigint_total()'
    /// window function.
    /// </summary>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and stored
    /// in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntTotalFinal(u64 *pAgg, bool isWide, DbStr *pResult);

    /// <summary>
    /// Implements the xInverse() function for the aggregate 'bigint_total()'
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int BigIntTotalInverse(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xStep() function for the aggregate 'bigint_total()'
    /// window function.
    /// </summary>
    /// <param name="pIn">Pointer to a DbStr structure holding the
    /// BigInteger value</param>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <returns>
    /// An integer result code.
    /// </returns>
    static int BigIntTotalStep(DbStr *pIn, u64 *pAgg);

    /// <summary>
    /// Implements the xValue() function for the aggregate 'bigint_total()'
    /// window function.
    /// </summary>
    /// <param name="pAgg">Pointer to a ulong aggregate context.</param>
    /// <param name="pResult">Pointer to a DbStr structure to hold the
    /// BigInteger result</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated and stored
    /// in <paramref name="pResult"/>.
    /// </returns>
    static int BigIntTotalValue(u64 *pAgg, bool isWide, DbStr *pResult);

  private:
    static int setValue(BigInteger value, bool isWide, DbStr *pResult);
    static BigInteger getValue(DbStr *pInput);
    static bool tryGetValue(DbStr *pInput, BigInteger% result);
    static String^ toHex(BigInteger bi);
    static BigInteger roundAverage(BigInteger sum, u64 count);
    static Dictionary<IntPtr, BigInteger>^ _values;
    static BigIntExt() {
      _values = gcnew Dictionary<IntPtr, BigInteger>;
    }
    static array<String^>^ HexTable = gcnew array<String^> {
      "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B",
      "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17",
      "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23",
      "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
      "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B",
      "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47",
      "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53",
      "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
      "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B",
      "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77",
      "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83",
      "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
      "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B",
      "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
      "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3",
      "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
      "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB",
      "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
      "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3",
      "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
      "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB",
      "FC", "FD", "FE", "FF"
    };
  };
}
