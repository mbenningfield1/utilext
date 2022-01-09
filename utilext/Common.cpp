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

  String^ Common::GetString(DbStr *pInput) {
    assert(pInput);
    String^ result = String::Empty;
    int cbIn = pInput->cb;
    if (cbIn > 0) {
      array<u8>^ arr = gcnew array<u8>(cbIn);
      pin_ptr<u8> pIn = &arr[0];
      memcpy(pIn, pInput->pText, (size_t)cbIn);
      if (pInput->isWide) {
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

  int Common::SetErrorString(String^ output, char **pzResult) {
    assert(output != nullptr);
    assert(output->Length > 0);

    array<u8>^ arrOut = _encoding8->GetBytes(output);
    *pzResult = (char*)calloc((size_t)(arrOut->Length + 1), 1);
    if (*pzResult) {
      pin_ptr<u8> pOut = &arrOut[0];
      memcpy(*pzResult, pOut, (size_t)(arrOut->Length));
    }
    else {
      return ERR_NOMEM;
    }
    return RESULT_OK;
  }

  int Common::SetString(String^ output, bool isWide, DbStr *pResult) {
    assert(output != nullptr);
    array<u8>^ arrOut = nullptr;
    if (isWide) {
      arrOut = _encoding16->GetBytes(output);
      pResult->isWide = true;
      pResult->pText = calloc((size_t)(arrOut->Length + 2), 1);
    }
    else {
      arrOut = _encoding8->GetBytes(output);
      pResult->isWide = false;
      pResult->pText = calloc((size_t)(arrOut->Length + 1), 1);
    }
    if (pResult->pText) {
      if (arrOut->Length > 0) {
        pin_ptr<u8> pOut = &arrOut[0];
        memcpy((void*)pResult->pText, pOut, (size_t)(arrOut->Length));
      }
      return RESULT_OK;
    }
    return ERR_NOMEM;
  }

  int Common::SetStringArray(array<String^>^ input, DbStrArr *pResult) {
    assert(pResult != nullptr);
    void *pTemp = nullptr;
    int n = input->Length;
    int i = 0;
    array<u8>^ bytes = nullptr;

    pResult->pArr = (char**)malloc(n * sizeof(void*));
    if (pResult->pArr == nullptr) return ERR_NOMEM;
    for (i = 0; i < n; i++) {
      bytes = _encoding8->GetBytes(input[i]);
      int len = bytes->Length;
      pTemp = calloc((size_t)(len + 1), 1);
      if (pTemp) {
        // don't bother to copy an empty string; calloc has zeroed the slot
        if (len > 0) {
          pin_ptr<u8> pOut = &bytes[0];
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
}
