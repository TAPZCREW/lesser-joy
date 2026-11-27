module;

#include <stormkit/core/platform_macro.hpp>

#define WIN32_LEAN_AND_MEAN

#include <SDKDDKVer.h>

#include <Windows.h>

#include <devpropdef.h>
#include <wdf.h>

export module wdk.wdf;

import std;

export namespace wdk {
    using ::EVT_WDF_OBJECT_CONTEXT_CLEANUP;

    using ::DEVPROPKEY;
    using ::DRIVER_INITIALIZE;
    using ::NonPagedPoolNx;
    using ::PDRIVER_OBJECT;
    using ::PUNICODE_STRING;
    using ::PWDF_DRIVER_GLOBALS;
    using ::PWDF_REQUEST_COMPLETION_PARAMS;
    using ::PWDF_TRI_STATE;
    using ::PWDFDEVICE_INIT;
    using ::UNICODE_STRING;
    using ::WDF_DEVICE_PROPERTY_DATA;
    using ::WDF_DRIVER_CONFIG;
    using ::WDF_DRIVER_GLOBALS;
    using ::WDF_IO_QUEUE_CONFIG;
    using ::WDF_IO_QUEUE_DISPATCH_TYPE;
    using ::WDF_MEMORY_DESCRIPTOR;
    using ::WDF_OBJECT_ATTRIBUTES;
    using ::WDF_PNPPOWER_EVENT_CALLBACKS;
    using ::WDF_POWER_DEVICE_STATE;
    using ::WDF_REQUEST_COMPLETION_PARAMS;
    using ::WDF_REQUEST_SEND_OPTIONS;
    using ::WDF_REQUEST_SEND_OPTIONS_FLAGS;
    using ::WDF_TRI_STATE;
    using ::WDFCMRESLIST;
    using ::WDFCONTEXT;
    using ::WDFDEVICE;
    using ::WDFDEVICE_INIT;
    using ::WDFDRIVER;
    using ::WDFIOTARGET;
    using ::WDFMEMORY;
    using ::WDFOBJECT;
    using ::WDFQUEUE;
    using ::WDFREQUEST;

    using ::WDF_DEVICE_PROPERTY_DATA_INIT;
    using ::WDF_DRIVER_CONFIG_INIT;
    using ::WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE;
    using ::WDF_MEMORY_DESCRIPTOR_INIT_BUFFER;
    using ::WDF_MEMORY_DESCRIPTOR_INIT_HANDLE;
    using ::WDF_OBJECT_ATTRIBUTES_INIT;
    using ::WDF_PNPPOWER_EVENT_CALLBACKS_INIT;
    using ::WDF_REL_TIMEOUT_IN_SEC;
    using ::WDF_REL_TIMEOUT_IN_MS;
    using ::WDF_REQUEST_SEND_OPTIONS_INIT;
    using ::WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT;
    using ::WdfDeviceAssignProperty;
    using ::WdfDeviceCreate;
    using ::WdfDeviceCreateDeviceInterface;
    using ::WdfDeviceInitSetPnpPowerEventCallbacks;
    using ::WdfDriverCreate;
    using ::WdfFdoInitSetFilter;
    using ::WdfIoQueueCreate;
    using ::WdfIoQueueDispatchParallel;
    using ::WdfIoTargetCancelSentIo;
    using ::WdfIoTargetStart;
    using ::WdfIoTargetStop;
    using ::WdfMemoryCopyFromBuffer;
    using ::WdfMemoryCopyToBuffer;
    using ::WdfMemoryCreate;
    using ::WdfMemoryGetBuffer;
    using ::WdfObjectAllocateContext;
    using ::WdfObjectDelete;
    using ::WdfRequestComplete;
    using ::WdfRequestCreate;
    using ::WdfRequestGetStatus;
    using ::WdfRequestRetrieveInputBuffer;
    using ::WdfRequestRetrieveInputMemory;
    using ::WdfRequestRetrieveOutputMemory;
    using ::WdfRequestSend;
    using ::WdfRequestSetCompletionRoutine;
    using ::WdfRequestSetInformation;

#pragma push_macro("WDF_NO_HANDLE")
#undef WDF_NO_HANDLE
    inline constexpr auto WDF_NO_HANDLE
#pragma pop_macro("WDF_NO_HANDLE")
      = nullptr;

#pragma push_macro("WDF_NO_OBJECT_ATTRIBUTES")
#undef WDF_NO_OBJECT_ATTRIBUTES
    inline constexpr auto WDF_NO_OBJECT_ATTRIBUTES
#pragma pop_macro("WDF_NO_OBJECT_ATTRIBUTES")
      = nullptr;

#pragma push_macro("WDF_NO_SEND_OPTIONS")
#undef WDF_NO_SEND_OPTIONS
    inline constexpr auto WDF_NO_SEND_OPTIONS
#pragma pop_macro("WDF_NO_SEND_OPTIONS")
      = nullptr;

#pragma push_macro("DEVPROP_MASK_TYPE")
#undef DEVPROP_MASK_TYPE
    inline constexpr auto DEVPROP_MASK_TYPE =
#pragma pop_macro("DEVPROP_MASK_TYPE")
      DEVPROP_MASK_TYPE;

#pragma push_macro("DEVPROP_MASK_TYPEMOD")
#undef DEVPROP_MASK_TYPEMOD
    inline constexpr auto DEVPROP_MASK_TYPEMOD =
#pragma pop_macro("DEVPROP_MASK_TYPEMOD")
      DEVPROP_MASK_TYPEMOD;

#pragma push_macro("DEVPROP_TYPE_BINARY")
#undef DEVPROP_TYPE_BINARY
    inline constexpr auto DEVPROP_TYPE_BINARY =
#pragma pop_macro("DEVPROP_TYPE_BINARY")
      DEVPROP_TYPE_BINARY;

#pragma push_macro("DEVPROP_TYPE_BOOLEAN")
#undef DEVPROP_TYPE_BOOLEAN
    inline constexpr auto DEVPROP_TYPE_BOOLEAN =
#pragma pop_macro("DEVPROP_TYPE_BOOLEAN")
      DEVPROP_TYPE_BOOLEAN;

#pragma push_macro("DEVPROP_TYPE_BYTE")
#undef DEVPROP_TYPE_BYTE
    inline constexpr auto DEVPROP_TYPE_BYTE =
#pragma pop_macro("DEVPROP_TYPE_BYTE")
      DEVPROP_TYPE_BYTE;

#pragma push_macro("DEVPROP_TYPE_CURRENCY")
#undef DEVPROP_TYPE_CURRENCY
    inline constexpr auto DEVPROP_TYPE_CURRENCY =
#pragma pop_macro("DEVPROP_TYPE_CURRENCY")
      DEVPROP_TYPE_CURRENCY;

#pragma push_macro("DEVPROP_TYPE_DATE")
#undef DEVPROP_TYPE_DATE
    inline constexpr auto DEVPROP_TYPE_DATE =
#pragma pop_macro("DEVPROP_TYPE_DATE")
      DEVPROP_TYPE_DATE;

#pragma push_macro("DEVPROP_TYPE_DECIMAL")
#undef DEVPROP_TYPE_DECIMAL
    inline constexpr auto DEVPROP_TYPE_DECIMAL =
#pragma pop_macro("DEVPROP_TYPE_DECIMAL")
      DEVPROP_TYPE_DECIMAL;

#pragma push_macro("DEVPROP_TYPE_DEVPROPKEY")
#undef DEVPROP_TYPE_DEVPROPKEY
    inline constexpr auto DEVPROP_TYPE_DEVPROPKEY =
#pragma pop_macro("DEVPROP_TYPE_DEVPROPKEY")
      DEVPROP_TYPE_DEVPROPKEY;

#pragma push_macro("DEVPROP_TYPE_DEVPROPTYPE")
#undef DEVPROP_TYPE_DEVPROPTYPE
    inline constexpr auto DEVPROP_TYPE_DEVPROPTYPE =
#pragma pop_macro("DEVPROP_TYPE_DEVPROPTYPE")
      DEVPROP_TYPE_DEVPROPTYPE;

#pragma push_macro("DEVPROP_TYPE_DOUBLE")
#undef DEVPROP_TYPE_DOUBLE
    inline constexpr auto DEVPROP_TYPE_DOUBLE =
#pragma pop_macro("DEVPROP_TYPE_DOUBLE")
      DEVPROP_TYPE_DOUBLE;

#pragma push_macro("DEVPROP_TYPE_EMPTY")
#undef DEVPROP_TYPE_EMPTY
    inline constexpr auto DEVPROP_TYPE_EMPTY =
#pragma pop_macro("DEVPROP_TYPE_EMPTY")
      DEVPROP_TYPE_EMPTY;

#pragma push_macro("DEVPROP_TYPE_ERROR")
#undef DEVPROP_TYPE_ERROR
    inline constexpr auto DEVPROP_TYPE_ERROR =
#pragma pop_macro("DEVPROP_TYPE_ERROR")
      DEVPROP_TYPE_ERROR;

#pragma push_macro("DEVPROP_TYPE_FILETIME")
#undef DEVPROP_TYPE_FILETIME
    inline constexpr auto DEVPROP_TYPE_FILETIME =
#pragma pop_macro("DEVPROP_TYPE_FILETIME")
      DEVPROP_TYPE_FILETIME;

#pragma push_macro("DEVPROP_TYPE_FLOAT")
#undef DEVPROP_TYPE_FLOAT
    inline constexpr auto DEVPROP_TYPE_FLOAT =
#pragma pop_macro("DEVPROP_TYPE_FLOAT")
      DEVPROP_TYPE_FLOAT;

#pragma push_macro("DEVPROP_TYPE_GUID")
#undef DEVPROP_TYPE_GUID
    inline constexpr auto DEVPROP_TYPE_GUID =
#pragma pop_macro("DEVPROP_TYPE_GUID")
      DEVPROP_TYPE_GUID;

#pragma push_macro("DEVPROP_TYPE_INT16")
#undef DEVPROP_TYPE_INT16
    inline constexpr auto DEVPROP_TYPE_INT16 =
#pragma pop_macro("DEVPROP_TYPE_INT16")
      DEVPROP_TYPE_INT16;

#pragma push_macro("DEVPROP_TYPE_INT32")
#undef DEVPROP_TYPE_INT32
    inline constexpr auto DEVPROP_TYPE_INT32 =
#pragma pop_macro("DEVPROP_TYPE_INT32")
      DEVPROP_TYPE_INT32;

#pragma push_macro("DEVPROP_TYPE_INT64")
#undef DEVPROP_TYPE_INT64
    inline constexpr auto DEVPROP_TYPE_INT64 =
#pragma pop_macro("DEVPROP_TYPE_INT64")
      DEVPROP_TYPE_INT64;

#pragma push_macro("DEVPROP_TYPE_NTSTATUS")
#undef DEVPROP_TYPE_NTSTATUS
    inline constexpr auto DEVPROP_TYPE_NTSTATUS =
#pragma pop_macro("DEVPROP_TYPE_NTSTATUS")
      DEVPROP_TYPE_NTSTATUS;

#pragma push_macro("DEVPROP_TYPE_NULL")
#undef DEVPROP_TYPE_NULL
    inline constexpr auto DEVPROP_TYPE_NULL =
#pragma pop_macro("DEVPROP_TYPE_NULL")
      DEVPROP_TYPE_NULL;

#pragma push_macro("DEVPROP_TYPE_SBYTE")
#undef DEVPROP_TYPE_SBYTE
    inline constexpr auto DEVPROP_TYPE_SBYTE =
#pragma pop_macro("DEVPROP_TYPE_SBYTE")
      DEVPROP_TYPE_SBYTE;

#pragma push_macro("DEVPROP_TYPE_SECURITY_DESCRIPTOR")
#undef DEVPROP_TYPE_SECURITY_DESCRIPTOR
    inline constexpr auto DEVPROP_TYPE_SECURITY_DESCRIPTOR =
#pragma pop_macro("DEVPROP_TYPE_SECURITY_DESCRIPTOR")
      DEVPROP_TYPE_SECURITY_DESCRIPTOR;

#pragma push_macro("DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING")
#undef DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING
    inline constexpr auto DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING =
#pragma pop_macro("DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING")
      DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING;

#pragma push_macro("DEVPROP_TYPE_STRING")
#undef DEVPROP_TYPE_STRING
    inline constexpr auto DEVPROP_TYPE_STRING =
#pragma pop_macro("DEVPROP_TYPE_STRING")
      DEVPROP_TYPE_STRING;

#pragma push_macro("DEVPROP_TYPE_STRING_INDIRECT")
#undef DEVPROP_TYPE_STRING_INDIRECT
    inline constexpr auto DEVPROP_TYPE_STRING_INDIRECT =
#pragma pop_macro("DEVPROP_TYPE_STRING_INDIRECT")
      DEVPROP_TYPE_STRING_INDIRECT;

#pragma push_macro("DEVPROP_TYPE_STRING_LIST")
#undef DEVPROP_TYPE_STRING_LIST
    inline constexpr auto DEVPROP_TYPE_STRING_LIST =
#pragma pop_macro("DEVPROP_TYPE_STRING_LIST")
      DEVPROP_TYPE_STRING_LIST;

#pragma push_macro("DEVPROP_TYPE_UINT16")
#undef DEVPROP_TYPE_UINT16
    inline constexpr auto DEVPROP_TYPE_UINT16 =
#pragma pop_macro("DEVPROP_TYPE_UINT16")
      DEVPROP_TYPE_UINT16;

#pragma push_macro("DEVPROP_TYPE_UINT32")
#undef DEVPROP_TYPE_UINT32
    inline constexpr auto DEVPROP_TYPE_UINT32 =
#pragma pop_macro("DEVPROP_TYPE_UINT32")
      DEVPROP_TYPE_UINT32;

#pragma push_macro("DEVPROP_TYPE_UINT64")
#undef DEVPROP_TYPE_UINT64
    inline constexpr auto DEVPROP_TYPE_UINT64 =
#pragma pop_macro("DEVPROP_TYPE_UINT64")
      DEVPROP_TYPE_UINT64;

#pragma push_macro("DEVPROP_TYPEMOD_ARRAY")
#undef DEVPROP_TYPEMOD_ARRAY
    inline constexpr auto DEVPROP_TYPEMOD_ARRAY =
#pragma pop_macro("DEVPROP_TYPEMOD_ARRAY")
      DEVPROP_TYPEMOD_ARRAY;

#pragma push_macro("DEVPROP_TYPEMOD_LIST")
#undef DEVPROP_TYPEMOD_LIST
    inline constexpr auto DEVPROP_TYPEMOD_LIST =
#pragma pop_macro("DEVPROP_TYPEMOD_LIST")
      DEVPROP_TYPEMOD_LIST;

    auto WdfObjectAttributesInitContextType(WDF_OBJECT_ATTRIBUTES&, const auto&) noexcept -> void;
} // namespace wdk

namespace wdk {
    /////////////////////////////////////
    /////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline auto WdfObjectAttributesInitContextType(WDF_OBJECT_ATTRIBUTES& attributes, const auto& context_type_info) noexcept
      -> void {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        attributes.ContextTypeInfo = context_type_info.UniqueType;
    }
} // namespace wdk
