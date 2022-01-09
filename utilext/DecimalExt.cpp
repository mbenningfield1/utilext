/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * DecimalExt class implementation.
 *
 * The methods defined in this class are basically just wrappers around the
 * .NET Decimal structure methods.
 *
 * The scalar functions take string pointers from native code, fix up managed
 * strings from them, parse them to Decimal values, and perform the operation.
 * Then they reverse the process to send a heap-allocated string pointer back
 * to native code.
 *
 * The aggregate function implementations maintain state in managed code,
 * because that's where the work is done. They all use a hashtable for storing
 * a Decimal value, keyed on the SQLite aggregate user context pointer. The
 * average aggregate implementation uses that pointer (int*) to store the value
 * count.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_DECIMAL

#include <assert.h>
#include <string.h>
#include "DecimalExt.h"

using namespace System;
using namespace System::Globalization;
using namespace System::Numerics;

using DEC = UtilityExtensions::DecimalExt;

namespace UtilityExtensions {

  int DEC::DecimalAbs(DbStr *pIn, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal result;
    if (parseDecimal(sValue, result)) {
      result = Math::Abs(result);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalAdd(int argc, DbStr *aValues, DbStr *pResult) {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    bool isWide = aValues[0].isWide;
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues + i);
      Decimal d;
      if (!parseDecimal(s, d)) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    try {
      for (int i = 1; i < argc; i++) {
        result += dVals[i];
      }
      return Common::SetString(result.ToString(), isWide, pResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalAverageAny(int argc, DbStr *aValues, DbStr *pResult) {
    assert(argc > 0);
    bool isWide = aValues[0].isWide;
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues + i);
      Decimal d;
      if (!parseDecimal(s, d)) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    for (int i = 1; i < argc; i++) {
      result += dVals[i];
    }
    result /= argc;
    return Common::SetString(result.ToString(), isWide, pResult);
  }

  int DEC::DecimalAverageFinal(u64 *pAgg, bool isWide, DbStr *pResult) {
    // if the query returns no rows and the xFinal() function is called without
    // a prior call to xStep(), then pAgg is a NULL pointer.

    if (pAgg && *pAgg == -1) {
      // xStep() or xValue() returned an error and cleared the hash key; this
      // call is only to dispose of the aggregate context, which is already
      // done on this side, and the native side just sets the allocation to
      // zero.
      return ERR_AGGREGATE;
    }
    int err = RESULT_OK;
    IntPtr context = (IntPtr)pAgg;
    String^ result = nullptr;
    if (!pAgg) {
      result = L"0.0";
    }
    else {
      try {
        Decimal d = _values[context] / *pAgg;
        result = d.ToString();
      }
      catch (OverflowException^) {
        err = ERR_DECIMAL_OVFLOW;
      }
    }
    _values->Remove(context);
    if (err == RESULT_OK) {
      return Common::SetString(result, isWide, pResult);
    }
    return err;
  }

  int DEC::DecimalAverageInverse(DbStr *pIn, u64 *pAgg) {
    // opposite of step(), so decrement the count and decrease the sum; we are
    // removing a value from the window that we must have put there in the first
    // place, so assert that this is a good value.

    assert(pIn);
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::Common::GetString(pIn);
    Decimal d;
    bool flag = parseDecimal(input, d);
    assert(flag);
    if (flag) {
      _values[context] = _values[context] - d;
      (*pAgg)--;
      return RESULT_OK;
    }
    assert(false);
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalAverageStep(DbStr *pIn, u64 *pAgg) {
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::GetString(pIn);
    Decimal d;
    Decimal sum;
    bool flag = parseDecimal(input, d);
    if (!flag) {
      _values->Remove(context);
      return ERR_DECIMAL_PARSE;
    }
    else {
      if (_values->ContainsKey(context)) {
        sum = _values[context];
        try {
          _values[context] = sum + d;
          (*pAgg)++;
        }
        catch (OverflowException^) {
          _values->Remove(context);
          return ERR_DECIMAL_OVFLOW;
        }
      }
      else {
        _values[context] = d;
        *pAgg = 1;
      }
    }
    return RESULT_OK;
  }

  int DEC::DecimalAverageValue(u64 *pAgg, bool isWide, DbStr *pResult) {
    // step() has already been called with at least one valid number, so
    // there should be a key into the hashtable for this context
    IntPtr context = (IntPtr)pAgg;
    assert(_values->ContainsKey(context));
    Decimal sum = _values[context];
    assert(*pAgg > 0);
    try {
      Decimal result = sum / *pAgg;
      return Common::SetString(result.ToString(), isWide, pResult);
    }
    catch (OverflowException^) {
      _values->Remove(context);
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalCeiling(DbStr *pIn, DbStr *pResult) {
    String^ input = Common::GetString(pIn);
    Decimal d;
    if (parseDecimal(input, d)) {
      return Common::SetString(Decimal::Ceiling(d).ToString(),
                               pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  // Since a collation sequence cannot fail, we have to deal deterministically
  // with invalid string inputs; NULL values are handled by SQLite and are never
  // passed to a collation sequence.
  // Each input can have one of 2 states:
  //    N - non-decimal string
  //    D - decimal string
  // Which gives us 2^2 permutations:
  //    N,N - return memcmp
  //    N,D - return N < D
  //    D,N - return D > N
  //    D,D - return decimal cmp
  // 
  // If any argument that is not a decimal compares to an argument that is,
  // we sort the non-decimal before the decimal, as if it were NULL. For 2
  // arguments where neither is a decimal, we just compare with BINARY.
  int DEC::DecimalCollate(DbStr *pLeft, DbStr *pRight) {
    bool nonLhs = false;
    bool nonRhs = false;
    String^ sLeft = Common::GetString(pLeft);
    String^ sRight = Common::GetString(pRight);
    Decimal left;
    nonLhs = !parseDecimal(sLeft, left);
    Decimal right;
    nonRhs = !parseDecimal(sRight, right);
    if (nonLhs) {
      if (nonRhs) {
        // N,N - return memcmp
        return memcmp(pLeft->pText, pRight->pText, pLeft->cb > pRight->cb ?
                                     (size_t)pRight->cb : (size_t)pLeft->cb);
      }
      else {
        // N,D - return N < D
        return -1;
      }
    }
    else if (nonRhs) {
      // D,N - return D > N
      return 1;
    }
    else {
      // D,D - return decimal cmp
      return Decimal::Compare(left, right);
    }
  }

  int DEC::DecimalCompare(DbStr *pLeft, DbStr *pRight, int *pResult) {
    String^ sLeft = Common::GetString(pLeft);
    String^ sRight = Common::GetString(pRight);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    *pResult = Decimal::Compare(left, right);
    return RESULT_OK;
  }

  int DEC::DecimalDivide(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    String^ sLeft = Common::GetString(pLeft);
    String^ sRight = Common::GetString(pRight);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = left / right;
      return Common::SetString(result.ToString(), pLeft->isWide, pResult);
    }
    catch (DivideByZeroException^) {
      return ERR_DECIMAL_DIVZ;
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalFloor(DbStr *pIn, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal result;
    if (parseDecimal(sValue, result)) {
      result = Decimal::Floor(result);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalLog(DbStr *pIn, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal number;
    Decimal result;
    if (parseDecimal(sValue, number)) {
      double d = Math::Log((double)number);
      if (Double::IsNaN(d) || Double::IsInfinity(d)) {
        return ERR_DECIMAL_NAN;
      }
      result = roundDouble(d);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalLog(DbStr *pIn, double base, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal number;
    Decimal result;
    if (parseDecimal(sValue, number)) {
      double d = Math::Log((double)number, base);
      if (Double::IsNaN(d) || Double::IsInfinity(d)) {
        return ERR_DECIMAL_NAN;
      }
      result = roundDouble(d);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalLog10(DbStr *pIn, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal number;
    Decimal result;
    if (parseDecimal(sValue, number)) {
      double d = Math::Log10((double)number);
      if (Double::IsNaN(d) || Double::IsInfinity(d)) {
        return ERR_DECIMAL_NAN;
      }
      result = roundDouble(d);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalMultiply(int argc, DbStr *aValues, DbStr *pResult) {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    bool isWide = aValues[0].isWide;
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues + i);
      Decimal d;
      bool flag = parseDecimal(s, d);
      if (!flag) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    try {
      for (int i = 1; i < argc; i++) {
        result *= dVals[i];
      }
      return Common::SetString(result.ToString(), isWide, pResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalNegate(DbStr *pIn,DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      result = Decimal::Negate(result);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalPower(DbStr *pIn, double exponent, DbStr *pResult) {
    String^ sBase = Common::GetString(pIn);
    Decimal base;
    Decimal result;
    if (parseDecimal(sBase, base)) {
      try {
        double d = Math::Pow((double)base, exponent);
        if (Double::IsNaN(d) || Double::IsInfinity(d)) {
          return ERR_DECIMAL_NAN;
        }
        result = roundDouble(d);
        return Common::SetString(result.ToString(), pIn->isWide, pResult);
      }
      catch (OverflowException^) {
        return ERR_DECIMAL_OVFLOW;
      }
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalRemainder(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    String^ sLeft = Common::GetString(pLeft);
    String^ sRight = Common::GetString(pRight);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = Decimal::Remainder(left, right);
      return Common::SetString(result.ToString(), pLeft->isWide, pResult);
    }
    catch (DivideByZeroException^) {
      return ERR_DECIMAL_DIVZ;
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalRound(DbStr *pIn, int digits, DbStr *pMode, DbStr *pResult) {
    if (digits < 0 || digits > 28) {
      return ERR_DECIMAL_PREC;
    }
    String^ sValue = Common::GetString(pIn);
    String^ sMode = Common::GetString(pMode);
    MidpointRounding mp;
    if (String::Compare(sMode, "even",
                        StringComparison::OrdinalIgnoreCase) == 0)
    {
      mp = MidpointRounding::ToEven;
    }
    else if (String::Compare(sMode, "norm",
                             StringComparison::OrdinalIgnoreCase) == 0)
    {
      mp = MidpointRounding::AwayFromZero;
    }
    else {
      return ERR_DECIMAL_MODE;
    }
    Decimal result;
    if (parseDecimal(sValue, result)) {
      try {
        result = Decimal::Round(result, digits, mp);
        return Common::SetString(result.ToString(), pIn->isWide, pResult);
      }
      catch (OverflowException^) {
        return ERR_DECIMAL_OVFLOW;
      }
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalSubtract(DbStr *pLeft, DbStr *pRight, DbStr *pResult) {
    String^ sLeft = Common::GetString(pLeft);
    String^ sRight = Common::GetString(pRight);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = left - right;
      return Common::SetString(result.ToString(), pLeft->isWide, pResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalTotalFinal(u64 *pAgg, bool isWide, DbStr *pResult) {
    if (pAgg && *pAgg == -1) {
      // xStep() or xValue() returned an error and cleared the hash key; this
      // call is only to dispose of context, which happens on the native
      // side.
      return ERR_AGGREGATE;
    }
    int err = RESULT_OK;
    IntPtr context = (IntPtr)pAgg; /* NULL if no rows */
    String^ result = nullptr;
    if (!_values->ContainsKey(context)) {
      result = L"0.0";
    }
    else {
      try {
        Decimal d = _values[context];
        result = d.ToString();
      }
      catch (OverflowException^) {
        err = ERR_DECIMAL_OVFLOW;
      }
    }
    _values->Remove(context);
    if (err == RESULT_OK) {
      return Common::SetString(result, isWide, pResult);
    }
    return err;
  }

  int DEC::DecimalTotalInverse(DbStr *pIn, void *pAgg) {
    // we're doing the opposite of step(), so subtract the value

    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::Common::GetString(pIn);
    Decimal d;
    bool flag = parseDecimal(input, d);
    assert(flag);
    if (flag) {
      _values[context] = _values[context] - d;
      return RESULT_OK;
    }
    assert(false);
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalTotalStep(DbStr *pIn, void *pAgg) {
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::GetString(pIn);
    Decimal d;
    Decimal sum;
    bool flag = parseDecimal(input, d);
    if (!flag) {
      _values->Remove(context);
      return ERR_DECIMAL_PARSE;
    }
    else {
      if (_values->ContainsKey(context)) {
        sum = _values[context];
        try {
          _values[context] = sum + d;
        }
        catch (OverflowException^) {
          _values->Remove(context);
          return ERR_DECIMAL_OVFLOW;
        }
      }
      else {
        _values[context] = d;
      }
    }
    return RESULT_OK;
  }

  int DEC::DecimalTotalValue(void *pAgg, bool isWide, DbStr *pResult) {
    // step() has already been called with at least one valid number, so
    // we can assert that the key for this context exists.
    IntPtr context = (IntPtr)pAgg;
    assert(_values->ContainsKey(context));
    Decimal result = _values[context];
    return Common::SetString(result.ToString(), isWide, pResult);
  }

  int DEC::DecimalTruncate(DbStr *pIn, DbStr *pResult) {
    String^ sValue = Common::GetString(pIn);
    Decimal result;
    if (parseDecimal(sValue, result)) {
      result = Decimal::Truncate(result);
      return Common::SetString(result.ToString(), pIn->isWide, pResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  // private methods

  bool DEC::parseDecimal(String^ input, Decimal% result) {
    return Decimal::TryParse(input,
                             NumberStyles::Any,
                             Common::Culture,
                             result);
  }

  Decimal DEC::roundDouble(double d) {
    double scale = d * 10000.0;
    BigInteger bi = BigInteger(scale);
    if (scale - (double)bi >= 0.5) {
      bi++;
    }
    return (Decimal)(bi) / Decimal(10000);
  }
}

#endif /* !UTILEXT_OMIT_DECIMAL */
