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
#include "utilext.h"

using namespace System;
using namespace System::Text;
using namespace System::Globalization;

namespace UtilityExtensions {

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
    /// <param name="pInput">A DbStr object that contains a pointer to a
    /// native string and the count of bytes in that string.</param>
    /// <returns>
    /// A managed string.
    /// </returns>
    static String^ GetString(DbStr *pInput);

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
    /// Converts a managed string into a heap-allocated UTF-8 error message.
    /// </summary>
    /// <param name="output">The managed string to convert</param>
    /// <param name="pzResult">Pointer to receive the allocated string</param>
    /// <returns>
    /// An integer result code. If successful, a string is allocated as assigned
    /// to <paramref name="pzResult"/>.
    /// </returns>
    static int SetErrorString(String^ output, char **pzResult);

    static int SetString(String^ output, bool isWide, DbStr *pResult);
    /// <summary>
    /// Converts a managed string array into a heap-allocated UTF-8 native
    /// string array.
    /// </summary>
    /// <param name="input">The managed string array to convert</param>
    /// <param name="pResult">A DbStrArr object to hold the native array</param>
    /// <returns>
    /// An integer result code. If successful, an allocated string array is
    /// assigned to <paramref name="pResult"/>, along with the length of the
    /// array.
    /// </returns>
    static int SetStringArray(array<String^>^ input, DbStrArr *pResult);

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
