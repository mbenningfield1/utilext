/*==============================================================================
 *
 *Written by: Mark Benningfield
 *
 *LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 *Class implementation for the static IntExt class.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_BIGINT

#include <assert.h>
#include "IntExt.h"

using namespace System::Collections::Generic;
using namespace System::Numerics;
using namespace System::Runtime::InteropServices;

namespace UtilityExtensions {

  int IntExt::BigIntCreate(DbStr *pIn, bool isWide, DbBytes *pResult) {
    String^ sVal = Common::GetString(pIn, isWide);
    BigInteger bi;
    if (BigInteger::TryParse(sVal, bi)) {
      return setValue(bi, pResult);
    }
    else {
      return ERR_BIGINT_PARSE;
    }
  }

  int IntExt::BigIntCreate(long iVal, DbBytes *pResult) {
    BigInteger bi = iVal;
    return setValue(bi, pResult);
  }

  int IntExt::BigIntCreate(double dVal, DbBytes *pResult) {
    try {
      BigInteger bi = (BigInteger)dVal;
      return setValue(bi, pResult);
    }
    catch (OverflowException^) {
      return ERR_BIGINT_OVFLOW;
    }
  }

  int IntExt::BigIntAbs(DbBytes *pIn, DbBytes *pResult) {
    BigInteger bi = getValue(pIn);
    bi = BigInteger::Abs(bi);
    return setValue(bi, pResult);
  }

  int IntExt::BigIntAdd2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = bl + br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntAddAll(DbBytes *aValues, int argc, DbBytes *pResult) {
    assert(argc > 2);
    BigInteger result = BigInteger::Zero;
    for (int i = 0; i < argc; i++) {
      DbBytes value = aValues[i];
      result += getValue(&value);
    }
    return setValue(result, pResult);
  }

  int IntExt::BigIntCompare(DbBytes *pLeft, DbBytes *pRight, int *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    *pResult = bl.CompareTo(br);
    return RESULT_OK;
  }

  int IntExt::BigIntDivide(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    if (br == BigInteger::Zero) {
      return ERR_BIGINT_DIVZ;
    }
    bl = bl / br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntGCD(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = BigInteger::GreatestCommonDivisor(bl, br);
    return setValue(bl, pResult);
  }

  int IntExt::BigIntLog(DbBytes *pIn, double *pResult) {
    BigInteger bi = getValue(pIn);
    try {
      *pResult = BigInteger::Log(bi);
      return RESULT_OK;
    }
    catch (ArgumentOutOfRangeException^) {
      return ERR_BIGINT_RANGE;
    }
  }

  int IntExt::BigIntLog(DbBytes *pIn, int base, double *pResult) {
    BigInteger bi = getValue(pIn);
    try {
      *pResult = BigInteger::Log(bi, (double)base);
      return RESULT_OK;
    }
    catch (ArgumentOutOfRangeException^) {
      return ERR_BIGINT_RANGE;
    }
  }

  int IntExt::BigIntLog10(DbBytes *pIn, double *pResult) {
    BigInteger bi = getValue(pIn);
    try {
      *pResult = BigInteger::Log10(bi);
      return RESULT_OK;
    }
    catch (ArgumentOutOfRangeException^) {
      return ERR_BIGINT_RANGE;
    }
  }

  int IntExt::BigIntMax2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = BigInteger::Max(bl, br);
    return setValue(bl, pResult);
  }

  int IntExt::BigIntMaxAny(DbBytes *aValues, int argc, DbBytes *pResult) {
    assert(argc > 2);
    array<BigInteger>^ inputs = gcnew array<BigInteger>(argc);
    for (int i = 0; i < argc; i++) {
      DbBytes buffer = aValues[i];
      inputs[i] = getValue(&buffer);
    }
    BigInteger max = inputs[0];
    for (int i = 1; i < argc; i++) {
      if (inputs[i] > max) max = inputs[i];
    }
    return setValue(max, pResult);
  }

  int IntExt::BigIntMin2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = BigInteger::Min(bl, br);
    return setValue(bl, pResult);
  }

  int IntExt::BigIntMinAny(DbBytes *aValues, int argc, DbBytes *pResult) {
    assert(argc > 2);
    array<BigInteger>^ inputs = gcnew array<BigInteger>(argc);
    for (int i = 0; i < argc; i++) {
      DbBytes buffer = aValues[i];
      inputs[i] = getValue(&buffer);
    }
    BigInteger min = inputs[0];
    for (int i = 1; i < argc; i++) {
      if (inputs[i] < min) min = inputs[i];
    }
    return setValue(min, pResult);
  }

  int IntExt::BigIntModPow(DbBytes *aValues, int argc, DbBytes *pResult) {
    assert(argc == 3); // input, exponent, modulus
    (void)(argc);
    DbBytes data = aValues[0];
    BigInteger input = getValue(&data);
    data = aValues[1];
    BigInteger exponent = getValue(&data);
    data = aValues[2];
    BigInteger modulus = getValue(&data);
    BigInteger result = BigInteger::ModPow(input, exponent, modulus);
    return setValue(result, pResult);
  }

  int IntExt::BigIntMultiply2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = bl * br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntMultiplyAll(DbBytes *aValues, int argc, DbBytes *pResult) {
    assert(argc > 2);
    array<BigInteger>^ inputs = gcnew array<BigInteger>(argc);
    for (int i = 0; i < argc; i++) {
      DbBytes buffer = aValues[i];
      BigInteger bi = getValue(&buffer);
      inputs[i] = bi;
    }
    BigInteger prod = inputs[0];
    for (int i = 1; i < argc; i++) {
      prod *= inputs[i];
    }
    return setValue(prod, pResult);
  }

  int IntExt::BigIntNegate(DbBytes *pIn, DbBytes *pResult) {
    BigInteger bi = getValue(pIn);
    bi = -bi;
    return setValue(bi, pResult);
  }

  int IntExt::BigIntPow(DbBytes *pIn, int exp, DbBytes *pResult) {
    assert(exp >= 0);
    BigInteger bi = getValue(pIn);
    bi = BigInteger::Pow(bi, exp);
    return setValue(bi, pResult);
  }

  int IntExt::BigIntRemainder(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    if (br == BigInteger::Zero) {
      return ERR_BIGINT_DIVZ;
    }
    bl = BigInteger::Remainder(bl, br);
    return setValue(bl, pResult);
  }

  int IntExt::BigIntSubtract(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = bl - br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntAnd(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = bl & br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntOr(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult) {
    BigInteger bl = getValue(pLeft);
    BigInteger br = getValue(pRight);
    bl = bl | br;
    return setValue(bl, pResult);
  }

  int IntExt::BigIntLeftShift(DbBytes *pIn, int shift, DbBytes *pResult) {
    assert(shift >= 0);
    BigInteger bi = getValue(pIn);
    bi = bi << shift;
    return setValue(bi, pResult);
  }

  int IntExt::BigIntNot(DbBytes *pIn, DbBytes *pResult) {
    BigInteger bi = getValue(pIn);
    bi = ~bi;
    return setValue(bi, pResult);
  }

  int IntExt::BigIntRightShift(DbBytes *pIn, int shift, DbBytes *pResult) {
    assert(shift >= 0);
    BigInteger bi = getValue(pIn);
    bi = bi >> shift;
    return setValue(bi, pResult);
  }

  int IntExt::setValue(BigInteger value, DbBytes *pResult) {
    return Common::SetBytes(value.ToByteArray(), pResult);
  }

  BigInteger IntExt::getValue(DbBytes *pInput) {
    return BigInteger(Common::GetBytes(pInput));
  }

  bool IntExt::parseBigInt(String^ input, BigInteger% result) {
    return BigInteger::TryParse(input, result);
  }
}

#endif /*UTILEXT_OMIT_BIGINT */
