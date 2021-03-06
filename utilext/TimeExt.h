/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static TimeExt class.
 *
 *============================================================================*/

#pragma once

#include "Common.h"

using namespace System;

namespace UtilityExtensions {

  ref class TimeExt abstract sealed {

  internal:
    /// <summary>
    /// Adds a specified TimeSpan value to a specified DateTime.
    /// </summary>
    /// <param name="time">Tick count for a TimeSpan value</param>
    /// <param name="pDate">Pointer to an encapsulated SQLite date/time value</param>
    /// <returns>
    /// An integer result code. If successful, the result is written into the
    /// appropriate member of <paramref name="pResult"/>.
    /// </returns>
    static int TimespanAddTo(i64 time, DbDate *pDate, DbDate *pResult);

    /// <summary>
    /// Creates a TimeSpan value from the specified date/time interval.
    /// </summary>
    /// <param name="pDate">Pointer to an encapsulated SQLite date/time value</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the resulting TimeSpan is written
    /// into <paramref name="pResult"/>.
    /// </returns>
    static int TimespanCreate(DbDate *pDate, i64 *pResult);

    /// <summary>
    /// Creates a TimeSpan value from the specified number of days, hours,
    /// minutes, seconds, and milliseconds.
    /// </summary>
    /// <param name="argc">The number of args in 'pArgs'</param>
    /// <param name="pArgs">An array of integer values</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code; if successful, the resulting TimeSpan is written
    /// into <paramref name="pResult"/>.
    /// </returns>
    static int TimespanCreate(int argc, int *pArgs, i64 *pResult);

    /// <summary>
    /// Calculates the difference between 2 DateTime values.
    /// </summary>
    /// <param name="pLeft">Pointer to an encapsulated SQLite date/time value</param>
    /// <param name="pRight">Pointer to an encapsulated SQLite date/time value</param>
    /// <param name="pResult">Pointer to hold the resulting TimeSpan tick count</param>
    /// <returns>
    /// An integer result code. If successful, the result is written into
    /// <paramref name="pResult"/>.
    /// </returns>
    static int TimespanDiff(DbDate *pLeft, DbDate *pRight, i64 *pResult);

    /// <summary>
    /// Converts a TimeSpan tick count into a formatted string.
    /// </summary>
    /// <param name="time">Tick count of a TimeSpan value</param>
    /// <param name="isWide">True if the text is desired in UTF-16</param>
    /// <param name="pResult">Pointer to hold the result</param>
    /// <returns>
    /// An integer result code. If successful, the formatted string is allocated
    /// and written into <paramref name="pResult"/>.
    /// </returns>
    static int TimespanStr(i64 time, bool isWide, DbStr *pResult);

  private:
    // 100-nanosecond tick count for the Unix epoch
    static const i64 UNIX_TICKS = 621355968000000000;
    static const i64 MIN_UNIX = -62135596800; // { 1/1/0001 00:00:00 }
    static const i64 MAX_UNIX = 253402300799; // { 12/31/9999 23:59:59 }

    // min & max seconds for a TimeSpan
    static const i64 MAX_TS_SECONDS = 922337203685;
    static const i64 MIN_TS_SECONDS = -922337203685;

    // min & max days for a TimeSpan
    static const double MAX_TS_DAYS = Int64::MaxValue / 864000000000.0; // ticks/day
    static const double MIN_TS_DAYS = Int64::MinValue / 864000000000.0; 

    // min & max JulianDay promoted to long integer
    static const i64 MIN_INT_JD = 148731163200000; // { 1/1/0001 00:00:00.000 }
    static const i64 MAX_INT_JD = 464269060799999; // { 12/31/9999 23:59:59.999 }

    // ISO-8601 with optional ms and time zone info
    static String^ DATE_FORMAT = "yyyy-MM-ddTHH:mm:ss.FFFK";

    static i64 ToUnixTime(DateTime dt);

    static DateTime DateTimeFromUnix(i64 unixTime);

    static double TimeExt::ToJulianDay(DateTime dt);

    static DateTime DateTimeFromJulian(double jd);
  };
}

