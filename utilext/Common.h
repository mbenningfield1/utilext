/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Class definition for the static Common class.
 *
 *============================================================================*/

#pragma once

#include "constants.h"

using namespace System;
using namespace System::Text;
using namespace System::Globalization;

namespace UtilityExtensions {

#pragma warning( push )
#pragma warning( disable: 4820 ) /* struct padding added */

  /* Native struct that represents a zero-terminated string from the database,
  ** encoded in UTF-8 or UTF-16.
  */
  struct DbStr {
    const void *pText;  /* pointer to the string bytes       */
    int cb;             /* count of bytes in pText (less \0) */
  };

  /* Native struct that represents a heap-allocated array of UTF-8 strings that
  ** is used for the regsplit() table-valued SQL function.
  */
  struct DbStrArr {
    char **pArr;  /* pointer to an array of encoded strings */
    int n;        /* number of strings in pArr              */
  };


  struct DbBytes {
    u8 *pData;
    int cb;
  };

#pragma warning ( pop )

  ref class Common abstract sealed {

  internal:

    /// <summary>
    /// Gets the culture rules to use for formatting and case comparisons.
    /// </summary>
    static property CultureInfo^ Culture {
      CultureInfo^ get(void) {
        return _culture;
      }
    };

    /// <summary>
    /// Converts a native pointer into a managed string, using the specified
    /// encoding.
    /// </summary>
    /// <param name="zInput">Pointer to a native string</param>
    /// <param name="cbIn">Count of bytes in 'zInput'</param>
    /// <param name="isWide">True if the encoding is UTF-16</param>
    /// <returns>
    /// A managed string.
    /// </returns>
    /// <remarks>
    /// This overload is used when we have an 'DbStr' struct instance in
    /// hand instead of a pointer to a struct instance. That struct is not very
    /// large (but it is padded on 64-bit systems). Rather than delving into
    /// a pointless analysis to determine if the compiler can -- or will --
    /// optimize passing the entire struct as a parameter, we just flatten out
    /// and pass the members.
    /// </remarks>
    static String^ GetString(const void *zInput, int cbIn, bool isWide);

    /// <summary>
    /// Converts a native pointer into a managed string, using the specified
    /// encoding.
    /// </summary>
    /// <param name="pInput">A 'DbStr' object that contains a pointer to a
    /// native string and the count of bytes in that string.</param>
    /// <param name="isWide">True if the encoding is UTF-16</param>
    /// <returns>
    /// A managed string.
    /// </returns>
    static String^ GetString(DbStr *pInput, bool isWide);

    /// <summary>
    /// Sets the CultureInfo to use for the current application session.
    /// </summary>
    /// <param name="lcName">A locale identifier for the desired culture</param>
    /// <param name="prev">Pointer to hold the previous culture identifier</param>
    /// <returns>
    /// An integer result code. If successful, the integer LCID of the culture
    /// that was in use prior to the call is written into <paramref name="prev"/>.
    /// </returns>
    /// <remarks>
    /// <paramref name="lcName"/> can be either a recognized NLS culture name or
    /// an integer string that represents a valid LCID.
    /// <para>
    /// If <paramref name="lcName"/> is an empty string, the
    /// <see cref="CultureInfo::InvariantCulture"/> is used.
    /// If <paramref name="lcName"/> is NULL, the LCID of the current
    /// culture is returned. If <paramref name="lcName"/> is not a recognized
    /// identifier, an error is returned.
    /// </para>
    /// </remarks>
    static int SetCultureInfo(String^ lcName, int *prev);

    /// <summary>
    /// Converts a managed string into a heap-allocated native string with the
    /// specified encoding.
    /// </summary>
    /// <param name="output">The managed string to convert</param>
    /// <param name="isWide">True if the encoding should be UTF16</param>
    /// <param name="ppResult">Pointer to the allocated string</param>
    /// <returns>
    /// An integer result code. If not successful, <paramref name="ppResult"/>
    /// is set to point to a NULL pointer. This happens if memory allocation
    /// fails.
    /// </returns>
    static int SetString(String^ output, bool isWide, void **ppResult);

    /// <summary>
    /// Converts a managed string array into a heap-allocated UTF-8 native
    /// string array.
    /// </summary>
    /// <param name="input">The managed string array to convert</param>
    /// <param name="pResult">A 'DbStrArr' object to hold the native array</param>
    /// <returns>
    /// An integer result code. If successful, an allocated array of char*'s is
    /// assigned to 'pResult', along with the length of the array.
    /// </returns>
    static int SetStringArray(array<String^>^ input, DbStrArr *pResult);

    static array<u8>^ GetBytes(DbBytes *pBuffer);

    static int SetBytes(array<u8>^ bytes, DbBytes *pResult);

  private:
    static CultureInfo^ _culture;
    static Encoding^ _encoding8;
    static Encoding^ _encoding16;
    static Common() {
      _encoding8 = gcnew UTF8Encoding;
      _encoding16 = gcnew UnicodeEncoding;
      _culture = CultureInfo::CurrentCulture;
    }
  };
}
