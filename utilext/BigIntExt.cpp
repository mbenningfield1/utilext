/*==============================================================================
 *
 *Written by: Mark Benningfield
 *
 *LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 *Class implementation for the static BigIntExt class.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_BIGINT

#include <assert.h>
#include <string.h>
#include "BigIntExt.h"

using namespace System::Collections::Generic;
using namespace System::Globalization;
using namespace System::Numerics;
using namespace System::Runtime::InteropServices;

using IntExt = UtilityExtensions::BigIntExt;

namespace UtilityExtensions {

  int IntExt::BigIntAbs(DbStr *pIn, DbStr *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = BigInteger::Abs(bi);
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntAdd(DbStr *aValues, int argc, DbStr *pResult) {
    assert(argc > 0);
    BigInteger result = BigInteger::Zero;
    BigInteger ival;
    for (int i = 0; i < argc; i++) {
      if (tryGetValue(aValues + i, ival)) {
        result += ival;
      }
      else {
        return ERR_BIGINT_PARSE;
      }
    }
    return setValue(result, aValues->isWide, pResult);
  }
  
  int IntExt::BigIntAnd(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      bl = bl & br;
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntAverage(DbStr *aValues, int argc, DbStr *pResult) {
    assert(argc >= 2);
    array<BigInteger>^ iVals = gcnew array<BigInteger>(argc);
    BigInteger val;
    for (int i = 0; i < argc; i++) {
      if (tryGetValue(aValues + i, val)) {
        iVals[i] = val;
      }
      else {
        return ERR_BIGINT_PARSE;
      }
    }
    BigInteger sum = iVals[0];
    for (int i = 1; i < argc; i++) {
      sum += iVals[i];
    }
    return setValue(roundAverage(sum, (u64)argc), aValues->isWide, pResult);
  }

  int IntExt::BigIntAverageFinal(u64 *pAgg, bool isWide, DbStr *pResult) {
    // if the query returns no rows and the xFinal() function is called without
    // a prior call to xStep(), then pAgg is a NULL pointer.

    if (pAgg && *pAgg == -1) {
      // an error occurred on the native side, and this call is made to dispose
      // of aggregate context, so clear the hash key
      _values->Remove((IntPtr)pAgg);
      return ERR_AGGREGATE;
    }
    BigInteger result = BigInteger::Zero;
    IntPtr ctx = (IntPtr)pAgg;
    if (pAgg) {
      assert(*pAgg > 0);
      u64 count = *pAgg;
      result = roundAverage(_values[ctx], count);
      _values->Remove(ctx);
    }
    return setValue(result, isWide, pResult);
  }

  int IntExt::BigIntAverageInverse(DbStr *pIn, u64 *pAgg) {
    // opposite of step(), so decrement the count and decrease the sum; we are
    // removing a value from the window that we must have put there in the first
    // place, so assert that this is a good value.

    assert(pIn);
    IntPtr ctx = (IntPtr)pAgg;
    BigInteger bi = getValue(pIn);
    _values[ctx] = _values[ctx] - bi;
    (*pAgg)--;
    return RESULT_OK;
  }

  int IntExt::BigIntAverageStep(DbStr *pIn, u64 *pAgg) {
    IntPtr ctx = (IntPtr)pAgg;
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      BigInteger sum = BigInteger::Zero;
      if (_values->ContainsKey(ctx)) {
        sum = _values[ctx];
        _values[ctx] = sum + bi;
        (*pAgg)++;
      }
      else {
        _values[ctx] = bi;
        *pAgg = 1;
      }
      return RESULT_OK;
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntAverageValue(u64 *pAgg, bool isWide, DbStr *pResult) {
    // step() has already been called with at least one valid number, so
    // there should be a key into the hashtable for this context
    IntPtr ctx = (IntPtr)pAgg;
    assert(_values->ContainsKey(ctx));
    u64 count = *pAgg;
    BigInteger result = roundAverage(_values[ctx], count);
    return setValue(result, isWide, pResult);
  }

  // Just like the decimal collation; we cannot fail, so deal with invalid
  // inputs:
  //  N,N - left and right are NOT BigIntegers
  //  N,I - left is NOT BigInteger, right is
  //  I,N - left is Big Integer, right is NOT
  //  I,I - both args are BigInteger
  int IntExt::BigIntCollate(DbStr *pLeft, DbStr *pRight) {
    bool nonLhs = false;
    bool nonRhs = false;
    BigInteger bl;
    BigInteger br;
    nonLhs = !tryGetValue(pLeft, bl);
    nonRhs = !tryGetValue(pRight, br);
    if (nonLhs) {
      if (nonRhs) {
        // N,N - return memcmp
        return memcmp(pLeft->pText, pRight->pText, pLeft->cb > pRight->cb ?
                                     (size_t)pRight->cb : (size_t)pLeft->cb);
      }
      else {
        // N,I - return N < I
        return -1;
      }
    }
    else if (nonRhs) {
      // I,N - return I > N
      return 1;
    }
    else {
      // I,I - return compare
      return BigInteger::Compare(bl, br);
    }
  }

  int IntExt::BigIntCompare(DbStr *pLeft, DbStr *pRight, int *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      *pResult = bl.CompareTo(br);
      return RESULT_OK;
    }
    return ERR_BIGINT_PARSE;
  }
  
  int IntExt::BigIntCreate(DbStr *pIn, DbStr *pResult) {
    String^ sVal = Common::GetString(pIn);
    BigInteger bi;
    double d;
    if (BigInteger::TryParse(sVal, bi)) {
      return setValue(bi, pIn->isWide, pResult);
    }
    else if (BigInteger::TryParse(sVal, NumberStyles::HexNumber, CultureInfo::InvariantCulture, bi)) {
      return setValue(bi, pIn->isWide, pResult);
    }
    else if (sVal->StartsWith("0x", StringComparison::InvariantCultureIgnoreCase)) {
      sVal = sVal->Substring(2);
      if (BigInteger::TryParse(sVal, NumberStyles::HexNumber, CultureInfo::InvariantCulture, bi)) {
        return setValue(bi, pIn->isWide, pResult);
      }
      else {
        return ERR_BIGINT_PARSE;
      }
    }
    else if (double::TryParse(sVal, d)) {
      try {
        bi = BigInteger(d);
        return setValue(bi, pIn->isWide, pResult);
      }
      catch (OverflowException^) {
        return ERR_BIGINT_OVFLOW;
      }
    }
    else {
      return ERR_BIGINT_PARSE;
    }
  }

  int IntExt::BigIntCreate(i64 iVal, bool isWide, DbStr *pResult) {
    BigInteger bi = iVal;
    return setValue(bi, isWide, pResult);
  }

  int IntExt::BigIntCreate(double dVal, bool isWide, DbStr *pResult) {
    try {
      BigInteger bi = (BigInteger)dVal;
      return setValue(bi, isWide, pResult);
    }
    catch (OverflowException^) {
      return ERR_BIGINT_OVFLOW;
    }
  }

  int IntExt::BigIntDivide(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      if (br == BigInteger::Zero) {
        return ERR_BIGINT_DIVZ;
      }
      bl = bl / br;
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntGCD(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      bl = BigInteger::GreatestCommonDivisor(bl, br);
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }
  
  int IntExt::BigIntLeftShift(DbStr *pIn, int shift, DbStr *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = bi << shift;
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntLog(DbStr *pIn, double *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      try {
        double d = BigInteger::Log(bi);
        if (Double::IsNaN(d) || Double::IsInfinity(d)) {
          return ERR_BIGINT_OVFLOW;
        }
        *pResult = d;
        return RESULT_OK;
      }
      catch (ArgumentOutOfRangeException^) {
        return ERR_BIGINT_RANGE;
      }
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntLog(DbStr *pIn, double base, double *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      try {
        double d = BigInteger::Log(bi, base);
        if (Double::IsNaN(d) || Double::IsInfinity(d)) {
          return ERR_BIGINT_OVFLOW;
        }
        *pResult = d;
        return RESULT_OK;
      }
      catch (ArgumentOutOfRangeException^) {
        return ERR_BIGINT_RANGE;
      }
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntLog10(DbStr *pIn, double *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      try {
        double d = BigInteger::Log10(bi);
        if (Double::IsNaN(d) || Double::IsInfinity(d)) {
          return ERR_BIGINT_OVFLOW;
        }
        *pResult = d;
        return RESULT_OK;
      }
      catch (ArgumentOutOfRangeException^) {
        return ERR_BIGINT_RANGE;
      }
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntModPow(DbStr *aValues, int argc, DbStr *pResult) {
    assert(argc == 3); // input, exponent, modulus
    (void)(argc);
    BigInteger input;
    BigInteger exp;
    BigInteger mod;
    if (tryGetValue(aValues, input) && 
        tryGetValue(aValues + 1, exp) && 
        tryGetValue(aValues + 2, mod))
    {
      if (exp < BigInteger::Zero) {
        return ERR_BIGINT_RANGE;
      }
      if (mod == BigInteger::Zero) {
        return ERR_BIGINT_DIVZ;
      }
      return setValue(BigInteger::ModPow(input, exp, mod),
                      aValues->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntMultiply(DbStr *aValues, int argc, DbStr *pResult) {
    assert(argc > 0);
    BigInteger bi;
    BigInteger prod = BigInteger::One;
    for (int i = 0; i < argc; i++) {
      if (tryGetValue(aValues + i, bi)) {
        prod *= bi;
      }
      else {
        return ERR_BIGINT_PARSE;
      }
    }
    return setValue(prod, aValues->isWide, pResult);
  }

  int IntExt::BigIntNegate(DbStr *pIn, DbStr *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = -bi;
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }
  
  int IntExt::BigIntNot(DbStr *pIn, DbStr *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = ~bi;
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }
  
  int IntExt::BigIntOr(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      bl = bl | br;
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntPow(DbStr *pIn, int exp, DbStr *pResult) {
    assert(exp >= 0);
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = BigInteger::Pow(bi, exp);
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntRemainder(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      if (br == BigInteger::Zero) {
        return ERR_BIGINT_DIVZ;
      }
      bl = BigInteger::Remainder(bl, br);
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntRightShift(DbStr *pIn, int shift, DbStr *pResult) {
    assert(shift >= 0);
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      bi = bi >> shift;
      return setValue(bi, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntString(DbStr *pIn, DbStr *pResult) {
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      String^ result = bi.ToString();
      return Common::SetString(result, pIn->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntSubtract(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    BigInteger bl;
    BigInteger br;
    if (tryGetValue(pLeft, bl) && tryGetValue(pRight, br)) {
      bl = bl - br;
      return setValue(bl, pLeft->isWide, pResult);
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntTotalFinal(u64 *pAgg, bool isWide, DbStr *pResult) {
    if (pAgg && *pAgg == -1) {
      // an error occurred on the native side, and this call is just to dispose
      // of the aggregate context, so clear the hash key.
      _values->Remove((IntPtr)pAgg);
      return ERR_AGGREGATE;
    }

    // if no rows are returned, pAgg will be NULL, so handle that
    IntPtr ctx = (IntPtr)pAgg;
    BigInteger bi = BigInteger::Zero;
    if (pAgg) {
      bi = _values[ctx];
    }
    _values->Remove(ctx);
    return setValue(bi, isWide, pResult);
  }

  int IntExt::BigIntTotalInverse(DbStr *pIn, u64 *pAgg) {
    // opposite of step
    assert(pIn);
    assert(pAgg);
    IntPtr ctx = (IntPtr)pAgg;
    BigInteger bi = getValue(pIn);
    _values[ctx] = _values[ctx] - bi;
    return RESULT_OK;
  }

  int IntExt::BigIntTotalStep(DbStr *pIn, u64 *pAgg) {
    IntPtr ctx = (IntPtr)pAgg;
    BigInteger bi;
    if (tryGetValue(pIn, bi)) {
      if (_values->ContainsKey(ctx)) {
        _values[ctx] = _values[ctx] + bi;
      }
      else {
        _values[ctx] = bi;
      }
      return RESULT_OK;
    }
    return ERR_BIGINT_PARSE;
  }

  int IntExt::BigIntTotalValue(u64 *pAgg, bool isWide, DbStr *pResult) {
    // step() has already been called with at least one valid number, so
    // we can assert that the key for this context exists.
    IntPtr ctx = (IntPtr)pAgg;
    assert(_values->ContainsKey(ctx));
    return setValue(_values[ctx], isWide, pResult);
  }

  // private methods

  int IntExt::setValue(BigInteger value, bool isWide, DbStr *pResult) {
    return Common::SetString(toHex(value), isWide, pResult);
  }

  BigInteger IntExt::getValue(DbStr *pInput) {
    // called in xInverse functions, so the input is known good
    String^ s = Common::GetString(pInput);
    assert(s->Length > 0);
    return BigInteger::Parse(s, NumberStyles::HexNumber,
                             CultureInfo::InvariantCulture);
  }

  bool IntExt::tryGetValue(DbStr *pInput, BigInteger% result) {
    String^ s = Common::GetString(pInput);
    if (s->Length > 0) {
      if (BigInteger::TryParse(s, NumberStyles::HexNumber,
                               CultureInfo::InvariantCulture, result))
      {
        return true;
      }
      return false;
    }
    return false;
  }

  // Our own version of BigInteger.ToString("X"). Performance test results
  // rate this version around 3 times as fast as the factory version. We are
  // sacrificing a little memory for a hex map table, and our strings
  // don't drop the high 'F' or '0' when possible like the factory version
  // does. So we occasionally wind up with one more character in the string
  // than is strictly needed, which doesn't seem to have a measurable effect
  // on parse time, and we have a fixed cost of less than 800 bytes for the
  // map table.
  String^ IntExt::toHex(BigInteger bi) {
    array<unsigned char>^ b = bi.ToByteArray();
    array<wchar_t>^ terms = gcnew array<wchar_t>(b->Length * 2);
    int j = b->Length - 1;
    for (int i = 0; j >= 0; i += 2, j--) {
      terms[i] = HexTable[b[j]][0];
      terms[i + 1] = HexTable[b[j]][1];
    }
    return gcnew String(terms);
  }

  BigInteger IntExt::roundAverage(BigInteger sum, u64 count) {
    // we want the remainder so we can see if we need to round up (away from 0)
    BigInteger rem;
    BigInteger n = count;
    BigInteger result = BigInteger::DivRem(sum, n, rem);

    // since count is always going to be a long, the remainder will also
    // always be a long, so we don't need BigIntegers to figure out if we are
    // going to round up the result (need Abs() here since casting BigInt to
    // u64 with a negative value throws Overflow)
    u64 iRem = (u64)BigInteger::Abs(rem);

    if (result > BigInteger::Zero) {
      if ((count & 1) == 1) {
        // count is odd, so to round up the remainder has to be greater than count
        // divided by 2
        if (iRem > (count >> 1)) {
          result++;
        }
      }
      else {
        // count is even, so to round up the remainder can also be equal to count
        // divided by 2 (exactly 0.5)
        if (iRem >= (count >> 1)) {
          result++;
        }
      }
    }
    else { // ditto for negative result
      if ((count & 1) == 1) {
        if (iRem > (count >> 1)) {
          result--;
        }
      }
      else {
        if (iRem >= (count >> 1)) {
          result--;
        }
      }
    }
    return result;
  }
}

#endif /* !UTILEXT_OMIT_BIGINT */
