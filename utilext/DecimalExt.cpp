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

using DEC = UtilityExtensions::DecimalExt;

namespace UtilityExtensions {

  int DEC::DecimalAbs(DbStr *pIn, bool isWide, void **zResult) {
    String^ sValue = Common::GetString(pIn, isWide);
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      result = Math::Abs(result);
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalAdd(int argc, DbStr *aValues, bool isWide, void **zResult) {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues[i].pText, aValues[i].cb, isWide);
      Decimal d;
      bool flag = parseDecimal(s, d);
      if (!flag) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    try {
      for (int i = 1; i < argc; i++) {
        result += dVals[i];
      }
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalAverageAny(int argc,
                             DbStr *aValues,
                             bool isWide,
                             void **zResult)
  {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues[i].pText, aValues[i].cb, isWide);
      Decimal d;
      bool flag = parseDecimal(s, d);
      if (!flag) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    for (int i = 1; i < argc; i++) {
      result += dVals[i];
    }
    result /= argc;
    return Common::SetString(result.ToString(), isWide, zResult);
  }

  int DEC::DecimalAverageFinal(int *pAgg, bool isWide, void **zResult) {
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
      return Common::SetString(result, isWide, zResult);
    }
    return err;
  }


  int DEC::DecimalAverageInverse(DbStr *pIn, bool isWide, int *pAgg) {
    // opposite of step(), so decrement the count and decrease the sum; we are
    // removing a value from the window that we must have put there in the first
    // place, so assert that this is a good value.

    assert(pIn);
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::Common::GetString(pIn, isWide);
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

  int DEC::DecimalAverageStep(DbStr *pIn, bool isWide, int *pAgg) {
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::GetString(pIn, isWide);
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

  int DEC::DecimalAverageValue(int *pAgg, bool isWide, void **zResult) {
    // step() has already been called with at least one valid number, so
    // there should be a key into the hashtables for this context
    IntPtr context = (IntPtr)pAgg;
    assert(_values->ContainsKey(context));
    Decimal sum = _values[context];
    try
    {
      Decimal result = sum / *pAgg;
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (OverflowException^)
    {
      _values->Remove(context);
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalCeiling(DbStr *pIn, bool isWide, void **zResult) {
    String^ input = Common::GetString(pIn, isWide);
    Decimal d;
    bool flag = parseDecimal(input, d);
    if (flag) {
      return Common::SetString(Decimal::Ceiling(d).ToString(), isWide, zResult);
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
  int DEC::DecimalCollate(DbStr *pLeft, DbStr *pRight, bool isWide) {
    bool nonLhs = false;
    bool nonRhs = false;
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
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

  int DEC::DecimalCompare(DbStr *pLeft,
                          DbStr *pRight,
                          bool isWide,
                          int *pResult)
  {
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    *pResult = Decimal::Compare(left, right);
    return RESULT_OK;
  }

  int DEC::DecimalDivide(DbStr *pLeft,
                         DbStr *pRight,
                         bool isWide,
                         void **zResult)
  {
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = left / right;
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (DivideByZeroException^) {
      return ERR_DECIMAL_DIVZ;
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalFloor(DbStr *pIn, bool isWide, void **zResult) {
    String^ sValue = Common::GetString(pIn, isWide);
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      result = Decimal::Floor(result);
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    return ERR_DECIMAL_PARSE;
  }

  int DEC::DecimalMax2(DbStr *pLeft,
                       DbStr *pRight,
                       bool isWide,
                       void **zResult)
  {
    String^ sLeft = nullptr;
    Decimal left;
    String^ sRight = nullptr;
    Decimal right;
    bool flag = false;
    Decimal result;

    if (pLeft == nullptr && pRight == nullptr) return RESULT_NULL;
    if (pLeft) {
      sLeft = Common::GetString(pLeft, isWide);
      flag = parseDecimal(sLeft, left);
      if (!flag) return ERR_DECIMAL_PARSE;
    }
    if (pRight) {
      sRight = Common::GetString(pRight, isWide);
      flag = parseDecimal(sRight, right);
      if (!flag) return ERR_DECIMAL_PARSE;
    }
    if (pLeft && pRight) {
      result = Math::Max(left, right);
    }
    else if (pLeft) {
      result = left;
    }
    else {
      result = right;
    }
    return Common::SetString(result.ToString(), isWide, zResult);
  }

  int DEC::DecimalMaxAny(int argc, DbStr *aValues, bool isWide, void **zResult) {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues[i].pText, aValues[i].cb, isWide);
      Decimal d;
      bool flag = parseDecimal(s, d);
      if (!flag) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    for (int i = 1; i < argc; i++) {
      if (dVals[i] > result) result = dVals[i];
    }
    return Common::SetString(result.ToString(), isWide, zResult);
  }

  int DEC::DecimalMin2(DbStr *pLeft,
                       DbStr *pRight,
                       bool isWide,
                       void **zResult)
  {
    String^ sLeft = nullptr;
    Decimal left;
    String^ sRight = nullptr;
    Decimal right;
    bool flag = false;
    Decimal result;

    if (pLeft == nullptr && pRight == nullptr) return RESULT_NULL;
    if (pLeft) {
      sLeft = Common::GetString(pLeft, isWide);
      flag = parseDecimal(sLeft, left);
      if (!flag) return ERR_DECIMAL_PARSE;
    }
    if (pRight) {
      sRight = Common::GetString(pRight, isWide);
      flag = parseDecimal(sRight, right);
      if (!flag) return ERR_DECIMAL_PARSE;
    }
    if (pLeft && pRight) {
      result = Math::Min(left, right);
    }
    else if (pLeft) {
      result = left;
    }
    else {
      result = right;
    }
    return Common::SetString(result.ToString(), isWide, zResult);
  }

  int DEC::DecimalMinAny(int argc, DbStr *aValues, bool isWide, void **zResult) {
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues[i].pText, aValues[i].cb, isWide);
      Decimal d;
      bool flag = parseDecimal(s, d);
      if (!flag) return ERR_DECIMAL_PARSE;
      dVals[i] = d;
    }
    Decimal result = dVals[0];
    for (int i = 1; i < argc; i++) {
      if (dVals[i] < result) result = dVals[i];
    }
    return Common::SetString(result.ToString(), isWide, zResult);
  }

  int DEC::DecimalMultiply(int argc, DbStr *aValues, bool isWide, void **zResult) {
    assert(argc > 0);
    array<Decimal>^ dVals = gcnew array<Decimal>(argc);
    for (int i = 0; i < argc; i++) {
      String^ s = Common::GetString(aValues[i].pText, aValues[i].cb, isWide);
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
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }

  int DEC::DecimalNegate(DbStr *pIn, bool isWide, void **zResult) {
    String^ sValue = Common::GetString(pIn, isWide);
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      result = Decimal::Negate(result);
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    return ERR_DECIMAL_PARSE;
  }


  int DEC::DecimalRemainder(DbStr *pLeft,
                            DbStr *pRight,
                            bool isWide,
                            void **zResult)
  {
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = Decimal::Remainder(left, right);
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (DivideByZeroException^) {
      return ERR_DECIMAL_DIVZ;
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }


  int DEC::DecimalRound(DbStr *pIn,
                        bool isWide,
                        int digits,
                        const void *zMode,
                        int cbMode,
                        void **zResult)
  {
    if (digits < 0 || digits > 28) {
      return ERR_DECIMAL_PREC;
    }
    String^ sValue = Common::GetString(pIn, isWide);
    String^ sMode = Common::GetString(zMode, cbMode, isWide);
    MidpointRounding mp;
    if (String::Compare(sMode,
                        "even",
                        StringComparison::OrdinalIgnoreCase) == 0)
    {
      mp = MidpointRounding::ToEven;
    }
    else if (String::Compare(sMode,
                             "norm",
                             StringComparison::OrdinalIgnoreCase) == 0)
    {
      mp = MidpointRounding::AwayFromZero;
    }
    else {
      return ERR_DECIMAL_MODE;
    }
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      try {
        result = Decimal::Round(result, digits, mp);
        return Common::SetString(result.ToString(), isWide, zResult);
      }
      catch (OverflowException^) {
        return ERR_DECIMAL_OVFLOW;
      }
    }
    return ERR_DECIMAL_PARSE;
  }


  int DEC::DecimalSubtract(DbStr *pLeft,
                           DbStr *pRight,
                           bool isWide,
                           void **zResult)
  {
    String^ sLeft = Common::GetString(pLeft, isWide);
    String^ sRight = Common::GetString(pRight, isWide);
    Decimal left;
    bool flag = parseDecimal(sLeft, left);
    if (!flag) return ERR_DECIMAL_PARSE;
    Decimal right;
    flag = parseDecimal(sRight, right);
    if (!flag) return ERR_DECIMAL_PARSE;
    try {
      Decimal result = left - right;
      return Common::SetString(result.ToString(), isWide, zResult);
    }
    catch (OverflowException^) {
      return ERR_DECIMAL_OVFLOW;
    }
  }


  int DEC::DecimalSumFinal(int *pAgg, bool isWide, void **zResult) {
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
      return Common::SetString(result, isWide, zResult);
    }
    return err;
  }


  int DEC::DecimalSumInverse(DbStr *pIn, bool isWide, void *pAgg) {
    // we're doing the opposite of step(), so subtract the value

    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::Common::GetString(pIn, isWide);
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


  int DEC::DecimalSumStep(DbStr *pIn, bool isWide, void *pAgg) {
    IntPtr context = (IntPtr)pAgg;
    String^ input = Common::GetString(pIn, isWide);
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


  int DEC::DecimalSumValue(void *pAgg, bool isWide, void **zResult) {
    // step() has already been called with at least one valid number, so
    // we can assert that the key for this context exists.
    IntPtr context = (IntPtr)pAgg;
    assert(_values->ContainsKey(context));
    Decimal result = _values[context];
    return Common::SetString(result.ToString(), isWide, zResult);
  }


  int DEC::DecimalTruncate(DbStr *pIn, bool isWide, void **zResult) {
    String^ sValue = Common::GetString(pIn, isWide);
    Decimal result;
    bool flag = parseDecimal(sValue, result);
    if (flag) {
      result = Decimal::Truncate(result);
      return Common::SetString(result.ToString(), isWide, zResult);
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
}

#endif /* !UTILEXT_OMIT_DECIMAL */
