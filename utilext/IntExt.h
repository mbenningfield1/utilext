/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static IntExt class.
 *
 *============================================================================*/

#pragma once
#include "Common.h"

using namespace System::Collections::Generic;
using namespace System::Numerics;

namespace UtilityExtensions {

  ref class IntExt abstract sealed {

  internal:
    
    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="isWide"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntCreate(DbStr *pIn, bool isWide, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="iVal"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntCreate(long iVal, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="dVal"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntCreate(double dVal, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntAbs(DbBytes *pIn, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntAdd2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="aValues"></param>
    /// <param name="argc"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntAddAll(DbBytes *aValues, int argc, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntCompare(DbBytes *pLeft, DbBytes *pRight, int *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntDivide(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntGCD(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntLog(DbBytes *pIn, double *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="base"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntLog(DbBytes *pIn, int base, double *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntLog10(DbBytes *pIn, double *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMax2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="aValues"></param>
    /// <param name="argc"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMaxAny(DbBytes *aValues, int argc, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMin2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="aValues"></param>
    /// <param name="argc"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMinAny(DbBytes* aValues, int argc, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="aValues"></param>
    /// <param name="argc"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntModPow(DbBytes *aValues, int argc, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMultiply2(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="aValues"></param>
    /// <param name="argc"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntMultiplyAll(DbBytes *aValues, int argc, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntNegate(DbBytes *pIn, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="exp"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntPow(DbBytes *pIn, int exp, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntRemainder(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntSubtract(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntAnd(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pLeft"></param>
    /// <param name="pRight"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntOr(DbBytes *pLeft, DbBytes *pRight, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="shift"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntLeftShift(DbBytes *pIn, int shift, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntNot(DbBytes *pIn, DbBytes *pResult);

    /// <summary>
    /// 
    /// </summary>
    /// <param name="pIn"></param>
    /// <param name="shift"></param>
    /// <param name="pResult"></param>
    /// <returns></returns>
    static int BigIntRightShift(DbBytes *pIn, int shift, DbBytes *pResult);

  private:
    static int setValue(BigInteger value, DbBytes *pResult);
    static BigInteger getValue(DbBytes *pInput);
    static bool parseBigInt(String^ input, BigInteger% result);
    static Dictionary<IntPtr, BigInteger>^ _values;
    static IntExt() {
      _values = gcnew Dictionary<IntPtr, BigInteger>;
    }
  };
}