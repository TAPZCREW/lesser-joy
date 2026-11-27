module;

#include <stormkit/core/platform_macro.hpp>

#define WIN32_LEAN_AND_MEAN

#include <SDKDDKVer.h>

#include <Windows.h>

#include <wdf.h>

#include <hidport.h>

#include <usb.h>
#include <usbspec.h>
#include <wdfusb.h>

// #define USBD_CLIENT_CONTRACT_VERSION_602 0x602

export module wdk.usb;

import std;

import stormkit.platform.win32;

export namespace wdk {
    using ::WDF_USB_BMREQUEST_DIRECTION;
    using ::WDF_USB_BMREQUEST_RECIPIENT;
    using ::WDF_USB_CONTINUOUS_READER_CONFIG;
    using ::WDF_USB_CONTROL_SETUP_PACKET;
    using ::WDF_USB_DEVICE_CREATE_CONFIG;
    using ::WDF_USB_DEVICE_SELECT_CONFIG_PARAMS;
    using ::WDF_USB_PIPE_INFORMATION;
    using ::WDF_USB_PIPE_TYPE;
    using ::WDFUSBDEVICE;
    using ::WDFUSBINTERFACE;
    using ::WDFUSBPIPE;

    using ::USB_DEVICE_DESCRIPTOR;

    using ::WDF_USB_CONTINUOUS_READER_CONFIG_INIT;
    using ::WDF_USB_CONTROL_SETUP_PACKET_INIT;
    using ::WDF_USB_DEVICE_CREATE_CONFIG_INIT;
    using ::WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES;
    using ::WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE;
    using ::WDF_USB_PIPE_INFORMATION_INIT;
    using ::WdfUsbInterfaceGetConfiguredPipe;
    using ::WdfUsbInterfaceGetNumConfiguredPipes;
    using ::WdfUsbTargetDeviceAllocAndQueryString;
    using ::WdfUsbTargetDeviceCreateWithParameters;
    using ::WdfUsbTargetDeviceGetDeviceDescriptor;
    using ::WdfUsbTargetDeviceGetInterface;
    using ::WdfUsbTargetDeviceGetIoTarget;
    using ::WdfUsbTargetDeviceGetNumInterfaces;
    using ::WdfUsbTargetDeviceQueryString;
    using ::WdfUsbTargetDeviceSelectConfig;
    using ::WdfUsbTargetDeviceSendControlTransferSynchronously;
    using ::WdfUsbTargetPipeConfigContinuousReader;
    using ::WdfUsbTargetPipeFormatRequestForWrite;
    using ::WdfUsbTargetPipeGetIoTarget;
    using ::WdfUsbTargetPipeIsInEndpoint;
    using ::WdfUsbTargetPipeIsOutEndpoint;
    using ::WdfUsbTargetPipeReadSynchronously;
    using ::WdfUsbTargetPipeSetNoMaximumPacketSizeCheck;
    using ::WdfUsbTargetPipeWriteSynchronously;

    inline constexpr auto USBD_CLIENT_CONTRACT_VERSION_602 = 0x602;
} // namespace wdk
