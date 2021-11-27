/*==============================================================================
 *
 * Written by: Mark Benningfield
 *
 * LICENSE: Public Domain -- see the file LICENSE.txt
 *
 *==============================================================================
 *
 * Common class implementation.
 *
 * The only thing this class does is provide a couple of static helper functions
 * that convert managed Strings to pointers and native pointers to managed
 * Strings, along with converting a managed string array to its heap-allocated
 * equivalent for use by native code.
 *
 * The two static Encoding variables are not assigned to the static default
 * Encoding.UTF8 and Encoding.Unicode objects, because those may have been
 * monkeyed with by other managed applications running on the host machine.
 *
 *============================================================================*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "Common.h"

using namespace System;
using namespace System::Globalization;
using namespace System::Runtime::InteropServices;
using namespace System::Text::RegularExpressions;
using namespace System::Threading;

namespace UtilityExtensions {

  String^ Common::GetString(const void *zInput, int cbIn, bool isWide) {
    assert(zInput);
    String^ result = String::Empty;
    if (cbIn > 0) {
      array<unsigned char>^ arr = gcnew array<unsigned char>(cbIn);
      pin_ptr<unsigned char> pIn = &arr[0];
      memcpy(pIn, zInput, (size_t)cbIn);
      if (isWide) {
        result = _encoding16->GetString(arr);
      }
      else {
        result = _encoding8->GetString(arr);
      }
    }
    return result;
  }

  String^ Common::GetString(DbStr *pInput, bool isWide) {
    assert(pInput);
    String^ result = String::Empty;
    int cbIn = pInput->cb;
    if (cbIn > 0) {
      array<unsigned char>^ arr = gcnew array<unsigned char>(cbIn);
      pin_ptr<unsigned char> pIn = &arr[0];
      memcpy(pIn, pInput->pText, (size_t)cbIn);
      if (isWide) {
        result = _encoding16->GetString(arr);
      }
      else {
        result = _encoding8->GetString(arr);
      }
    }
    return result;
  }

  int Common::SetCultureInfo(String^ lcName, int *prev) {
    int lcid = -1;
    bool isNumber = false;
    *prev = 0;
    CultureInfo^ culture = nullptr;
    
    if (lcName->StartsWith("0x", StringComparison::OrdinalIgnoreCase)) {
      isNumber = Int32::TryParse(lcName->Substring(2),
                                 NumberStyles::HexNumber,
                                 CultureInfo::InvariantCulture,
                                 lcid);
    }
    else {
      isNumber = Int32::TryParse(lcName, lcid);
    }
    try {
      if (isNumber) {
        culture = gcnew CultureInfo(lcid, true);
      }
      else {
        culture = gcnew CultureInfo(lcName, true);
      }
      if (culture != nullptr) {
        *prev = _culture->LCID;
        _culture = culture;
      }
      return RESULT_OK;
    }
    catch (ArgumentException^) {
      return ERR_CULTURE;
    }
  }

  int Common::SetString(String^ output, bool isWide, void **ppResult) {
    assert(output != nullptr);

    *ppResult = nullptr;
    array<unsigned char>^ arrOut = nullptr;
    if (isWide) {
      arrOut = _encoding16->GetBytes(output);
      *ppResult = calloc((size_t)(arrOut->Length + 2), 1);
    }
    else {
      arrOut = _encoding8->GetBytes(output);
      *ppResult = calloc((size_t)(arrOut->Length + 1), 1);
    }
    if (*ppResult) {
      if (arrOut->Length > 0) {
        pin_ptr<unsigned char> pOut = &arrOut[0];
        memcpy(*ppResult, pOut, (size_t)(arrOut->Length));
      }
    }
    else {
      return ERR_NOMEM;
    }
    return RESULT_OK;
  }

  int Common::SetStringArray(array<String^>^ input, DbStrArr *pResult) {
    assert(pResult != nullptr);
    void *pTemp = nullptr;
    int n = input->Length;
    int i = 0;
    array<unsigned char>^ bytes = nullptr;

    pResult->pArr = (char**)malloc(n * sizeof(void*));
    if (pResult->pArr == nullptr) return ERR_NOMEM;
    for (i = 0; i < n; i++) {
      bytes = _encoding8->GetBytes(input[i]);
      int len = bytes->Length;
      pTemp = calloc((size_t)(len + 1), 1);
      if (pTemp) {
        // don't bother to copy an empty string; calloc has zeroed the slot
        if (len > 0) {
          pin_ptr<unsigned char> pOut = &bytes[0];
          memcpy(pTemp, pOut, (size_t)len);
        }
        pResult->pArr[i] = (char*)pTemp;
      }
      else {
        goto CLEANUP;
      }
    }
    pResult->n = n;
    return RESULT_OK;

CLEANUP:
    for (int j = 0; j < i; j++) {
      free(pResult->pArr[j]);
    }
    free(pResult->pArr);
    return ERR_NOMEM;
  }

  int Common::SetBytes(array<u8>^ bytes, DbBytes *pResult) {
    assert(bytes != nullptr);
    DbBytes data;
    data.cb = bytes->Length;
    data.pData = (u8*)malloc((size_t)data.cb);
    if (data.pData) {
      if (data.cb > 0) {
        pin_ptr<u8> pStart = &bytes[0];
        memcpy(data.pData, pStart, (size_t)data.cb);
      }
      *pResult = data;
    }
    else {
      return ERR_NOMEM;
    }
    return RESULT_OK;
  }

  array<u8>^ Common::GetBytes(DbBytes *pBuffer) {
    assert(pBuffer);
    DbBytes raw = *pBuffer;
    array<u8>^ result =gcnew array<u8>(raw.cb);
    pin_ptr<u8> pStart = &result[0];
    memcpy(pStart, raw.pData, (size_t)raw.cb);
    return result;
  }
}
