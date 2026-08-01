#ifndef LESSERJOY_WINDOWS_HPP
#define LESSERJOY_WINDOWS_HPP

#define WIN32_NO_STATUS
#include <stormkit/core/platform_macro.hpp>

#include <stormkit/core/platform/windows.hpp>
#undef WIN32_NO_STATUS
#include <devpropdef.h>
#include <ntstatus.h>
#include <wdf.h>

#undef WDF_NO_HANDLE
inline constexpr auto WDF_NO_HANDLE = nullptr;

#undef WDF_NO_OBJECT_ATTRIBUTES
inline constexpr auto WDF_NO_OBJECT_ATTRIBUTES = nullptr;

#undef WDF_NO_SEND_OPTIONS
inline constexpr auto WDF_NO_SEND_OPTIONS = nullptr;

#endif
