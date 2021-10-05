/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * TimeExt class implementation.
 *
 * The managed class only handles a few of the timespan SQL functions. Since a
 * TimeSpan is represented as a 64-bit signed integer, those SQL functions that
 * don't deal with interaction with a date/time value or strings are handled
 * completely in native code.
 *
 *============================================================================*/

#ifndef UTILEXT_OMIT_TIME

#include <assert.h>
#include "TimeExt.h"

using namespace System;

namespace UtilityExtensions {

  int TimeExt::TimespanAddTo(DbDate *pDate, bool isWide, Int64 time) {
    DateTime dt = DateTime::MinValue;
    TimeSpan ts = TimeSpan::MinValue;
    switch (pDate->type)
    {
      case TIMETYPE_UNIX:
        try {
          dt = DateTimeFromUnix(pDate->unix);
          ts = TimeSpan(time);
          dt += ts;
          pDate->unix = ToUnixTime(dt);
        }
        catch (ArgumentOutOfRangeException^) {
          return ERR_TIME_PARSE;
        }
        catch (ArgumentException^) {
          return ERR_TIME_UNIX_RANGE;
        }
        break;
      case TIMETYPE_JULIAN:
        try
        {
          dt = DateTimeFromJulian(pDate->julian);
          ts = TimeSpan(time);
          dt += ts;
          pDate->julian = ToJulianDay(dt);
        }
        catch (ArgumentOutOfRangeException^) {
          return ERR_TIME_PARSE;
        }
        catch (ArgumentException^) {
          return ERR_TIME_JD_RANGE;
        }
        break;
      case TIMETYPE_ISO:
        try
        {
          String^ s = Common::GetString(pDate->iso.pText, pDate->iso.cb, isWide);
          dt = DateTime::Parse(s);
          ts = TimeSpan(time);
          dt += ts;
          Common::SetString(dt.ToString(DATE_FORMAT), isWide, (void**)&pDate->iso.pText);
        }
        catch (Exception^) {
          return ERR_TIME_PARSE;
        }
        break;
      default:
        return ERR_TIME_PARSE;
    }
    return RESULT_OK;
  }

  int TimeExt::TimespanCreate(DbDate *pDate, bool isWide, Int64 *pResult) {
    
    switch (pDate->type) {
      case TIMETYPE_UNIX:
        if (pDate->unix < MIN_TS_SECONDS || pDate->unix > MAX_TS_SECONDS) {
          return ERR_TIME_INVALID;
        }
        try {
          *pResult = TimeSpan(0, 0, (int)pDate->unix).Ticks;
        }
        catch (ArgumentOutOfRangeException^) {
          return ERR_TIME_INVALID;
        }
        break;
      case TIMETYPE_JULIAN: {
        double days = pDate->julian;
        if (days < MIN_TS_DAYS || days > MAX_TS_DAYS) {
          return ERR_TIME_INVALID;
        }
        try {
          int d, h, m, s, ms;
          d = (int)days;
          days -= d;
          int totSec = (int)(days * 86400000); // ms per day
          Decimal seconds = totSec / Decimal(1000);
          totSec = (int)seconds;
          ms = (int)((seconds - totSec) * Decimal(1000));
          seconds -= totSec;
          h = totSec / 3600;
          totSec -= h * 3600;
          m = totSec / 60;
          seconds += totSec - m * 60;
          s = (int)seconds;
          *pResult = TimeSpan(d, h, m, s, ms).Ticks;
        }
        catch (ArgumentOutOfRangeException^) {
          return ERR_TIME_INVALID;
        }
        break;
      } /* case block with initializer */
      case TIMETYPE_ISO:
        try {
          String^ ts = Common::GetString(&pDate->iso, isWide);
          *pResult = TimeSpan::Parse(ts).Ticks;
        }
        catch (FormatException^) {
          return ERR_TIME_PARSE;
        }
        catch (OverflowException^) {
          return ERR_TIME_INVALID;
        }
        break;
      default:
        assert(0);
    }
    return RESULT_OK;
  }

  int TimeExt::TimespanCreate(int argc, int *pArgs, Int64 *pResult) {
    // argc is 3, 4, or 5
    try {
      if (argc == 3) {
        *pResult = TimeSpan(pArgs[0], pArgs[1], pArgs[2]).Ticks;
      }
      else if (argc == 4) {
        *pResult = TimeSpan(pArgs[0], pArgs[1], pArgs[2], pArgs[3]).Ticks;
      }
      else {
        *pResult = TimeSpan(pArgs[0], pArgs[1], pArgs[2], pArgs[3], pArgs[4]).Ticks;
      }
      return RESULT_OK;
    }
    catch (ArgumentOutOfRangeException^) {
      return ERR_TIME_INVALID;
    }
  }

  int TimeExt::TimespanDiff(DbDate *pLeft, DbDate *pRight, bool isWide, Int64 *pResult)
  {
    DateTime dt1 = DateTime::MinValue;
    DateTime dt2 = DateTime::MinValue;

    switch (pLeft->type) {
      case TIMETYPE_UNIX:
        try {
          dt1 = DateTimeFromUnix(pLeft->unix);
        }
        catch (ArgumentException^) {
          return ERR_TIME_UNIX_RANGE;
        }
        break;
      case TIMETYPE_JULIAN:
        try {
          dt1 = DateTimeFromJulian(pLeft->julian);
        }
        catch (ArgumentException^) {
          return ERR_TIME_JD_RANGE;
        }
        break;
      case TIMETYPE_ISO:
        try {
          dt1 = DateTime::Parse(Common::GetString(pLeft->iso.pText, pLeft->iso.cb, isWide));
        }
        catch (Exception^) {
          return ERR_TIME_PARSE;
        }
        break;
    }
    switch (pRight->type) {
      case TIMETYPE_UNIX:
        try {
          dt2 = DateTimeFromUnix(pRight->unix);
        }
        catch (ArgumentException^) {
          return ERR_TIME_UNIX_RANGE;
        }
        break;
      case TIMETYPE_JULIAN:
        try {
          dt2 = DateTimeFromJulian(pRight->julian);
        }
        catch (ArgumentException^) {
          return ERR_TIME_JD_RANGE;
        }
        break;
      case TIMETYPE_ISO:
        try {
          dt2 = DateTime::Parse(Common::GetString(pRight->iso.pText, pRight->iso.cb, isWide));
        }
        catch (Exception^) {
          return ERR_TIME_PARSE;
        }
        break;
    }
    *pResult = (dt1 - dt2).Ticks;
    return RESULT_OK;
  }

  int TimeExt::TimespanStr(Int64 time, bool isWide, void **zResult)
  {
    TimeSpan ts = TimeSpan(time);
    String^ str = ts.ToString(); // default "c" format
    return Common::SetString(str, isWide, zResult);
  }

  Int64 TimeExt::ToUnixTime(DateTime dt) {
    Int64 ticks = dt.ToUniversalTime().Ticks;

    // unix time deals in whole seconds, so strip the milliseconds
    ticks -= dt.Millisecond * TimeSpan::TicksPerMillisecond;

    return (ticks - UNIX_TICKS) / TimeSpan::TicksPerSecond;
  }

  DateTime TimeExt::DateTimeFromUnix(Int64 unixTime) {
    if (unixTime < MIN_UNIX || unixTime > MAX_UNIX) {
      throw gcnew ArgumentException("The specified Unix time is outside the range of valid values.");
    }
    return DateTime(unixTime * TimeSpan::TicksPerSecond + UNIX_TICKS, DateTimeKind::Utc);
  }

  double TimeExt::ToJulianDay(DateTime dt) {
      // the Julian day calculation from the SQLite source.
      int Y, M, D, A, B, X1, X2;
      int h, m, s, ms;
      Int64 iJD;

      dt = dt.ToUniversalTime();
      Y = dt.Year;
      M = dt.Month;
      D = dt.Day;
      h = dt.Hour;
      m = dt.Minute;
      s = dt.Second;
      ms = dt.Millisecond;

      if (M <= 2) {
        Y--;
        M += 12;
      }
      A = Y / 100;
      B = 2 - A + (A / 4);
      X1 = 36525 * (Y + 4716) / 100;
      X2 = 306001 * (M + 1) / 10000;
      iJD = (Int64)((X1 + X2 + D + B - 1524.5) * 86400000);

      //
      // NOTE:  The DateTime struct in the sqlite core stores seconds and
      //        milliseconds together as a fractional double value, which is
      //        extended by the millisecond scale (1000) and then truncated; we
      //        reproduce that operation here to improve consistency with the
      //        core. The way we do it is by taking the fractional part of the
      //        double TotalSeconds and rounding it to 9 digits to clean up any
      //        internal floating-point cruft, then adding that fraction to the
      //        number of seconds and promoting to integer milliseconds. If we
      //        simply extend the seconds from a .NET DateTime and add the
      //        milliseconds, we are sometimes off by 1 or 2 milliseconds
      //        compared to the result from the core. Using 'Decimal' here and
      //        avoiding the rounding also results in occasional inconsistency
      //        in the least 1 or 2 significant digits. If we just use the
      //        TotalSeconds double value and promote the whole thing to integer
      //        milliseconds, that doesn't work either.
      //
      double mantissa = Math::Round(dt.TimeOfDay.TotalSeconds - (int)dt.TimeOfDay.TotalSeconds, 9, MidpointRounding::AwayFromZero);
      int seconds = (int)((mantissa + s) * 1000);

      //iJD += (h * 3600000) + (m * 60000) + (s * 1000) + ms; ** doesn't work
      //iJD += (int)(dt.TimeOfDay.TotalSeconds * 1000); ** doesn't work
      iJD += (h * 3600000) + (m * 60000) + seconds;
      return iJD / 86400000.0;
    }

  DateTime TimeExt::DateTimeFromJulian(double jd) {
      // The Julian to Date conversion from the SQLite source
      int Z, A, B, C, D, E, X1;
      int day, year, mo, h, m, s, ms;

      // NOTE:    We're rounding here to avoid the occasional 'string of 9's
      Int64 iJD = (Int64)Math::Round((jd * 86400000.0));
      if (iJD < MIN_INT_JD || iJD > MAX_INT_JD) {
        throw gcnew ArgumentException("The specified Julian day is outside the range of valid values.");
      }
      Z = (int)((iJD + 43200000) / 86400000);
      A = (int)((Z - 1867216.25) / 36524.25);
      A = Z + 1 + A - (A / 4);
      B = A + 1524;
      C = (int)((B - 122.1) / 365.25);
      D = (36525 * (C & 32767)) / 100;
      E = (int)((B - D) / 30.6001);
      X1 = (int)(30.6001 * E);
      day = B - D - X1;
      mo = E < 14 ? E - 1 : E - 13;
      year = mo > 2 ? C - 4716 : C - 4715;

      int totSec = (int)((iJD + 43200000) % 86400000);

      // we use Decimal here to head off any rounding errors from floating
      // point, which we can do when going from julian day to date time.
      // Doesn't work too well going the other way.
      Decimal seconds = totSec / Decimal(1000);
      totSec = (int)seconds;

      ms = (int)((seconds - totSec) * Decimal(1000));
      seconds -= totSec;

      h = totSec / 3600;
      totSec -= h * 3600;

      m = totSec / 60;
      seconds += totSec - m * 60;

      s = (int)seconds;
      return DateTime(year, mo, day, h, m, s, ms, DateTimeKind::Utc);
    }
}

#endif // !UTILEXT_OMIT_TIME
