// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The GYP based build ends up defining USING_V8_SHARED when compiling this
// file.
#undef USING_V8_SHARED
#include "include/v8.h"
#include "base/allocator/features.h"
#include "build/build_config.h"

#if V8_OS_WIN
#include <locale.h>
#include "src/base/win32-headers.h"

extern "C" {
BOOL WINAPI DllMain(HANDLE hinstDLL,
                    DWORD dwReason,
                    LPVOID lpvReserved) {

  if (dwReason == DLL_PROCESS_ATTACH) {
    setlocale(LC_ALL, NULL);
  }

  return TRUE;
}
}

#if BUILDFLAG(USE_EXPERIMENTAL_ALLOCATOR_SHIM)
extern __int64 allocator_shim_counter;
extern "C" {
__declspec(dllexport) __int64 GetAllocatorShimCounter()
{
    return allocator_shim_counter;
}
}
#endif

#endif  // V8_OS_WIN
