#ifndef LESSERJOY_WDFOBJECT_MACROS_HPP
#define LESSERJOY_WDFOBJECT_MACROS_HPP

import stormkit.platform.win32;

import wdk.wdf;
import wdk.ntstatus;

using wdk::NTSTATUS;
using wdk::PWDF_DRIVER_GLOBALS;
using wdk::WDFOBJECT;
using win32::GUID;
using win32::LONG;
using win32::PCHAR;
using win32::PVOID;
using win32::ULONG;
using win32::VOID;

#include <sal.h>

#include <Driverspecs.h>

#define FORCEINLINE __forceinline
#define FASTCALL    __fastcall
#define WDFAPI      __stdcall
#define CONST       const
#ifndef NULL
    #define NULL nullptr
#endif

#include <wdfobject.h>

#endif
