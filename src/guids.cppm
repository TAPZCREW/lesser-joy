module;

#define INITGUID
#include <ntddk.h>
#include <wdf.h>
#include <initguid.h> 

export module nsw2ble.guids;

export namespace nsw2ble {

/* fc71b33d-d528-4763-a86c-78777c7bcd7b */
DEFINE_GUID(DEVICE_INTERFACE, 0xfc71b33d, 0xd528, 0x4763, 0xa8, 0x6c, 0x78, 0x77, 0x7c, 0x7b, 0xcd, 0x7b);

/** [Service] Handle=0x0001 Type=00c5af5d-1964-4e30-8f51-1956f96bd280(Service Control) **/
DEFINE_GUID(SERVICE_CONTROL,        0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x80);
extern __declspec(selectany) const PWSTR SERVICE_CONTROL_NAME = L"Service Control";
/**     [Characteristic] Handle=0x0002 ValueHandle=0x0003 Type=00c5af5d-1964-4e30-8f51-1956f96bd281 Properties=(Read) */
// DEFINE_GUID(UNKNOWN_1956f96bd281,   0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x81);
/**     [Characteristic] Handle=0x0004 ValueHandle=0x0005 Type=00c5af5d-1964-4e30-8f51-1956f96bd282(Service Enable) Properties=(Write) */
DEFINE_GUID(SERVICE_ENABLE,         0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x82);
extern __declspec(selectany) const PWSTR SERVICE_ENABLE_NAME  = L"Service Enable";
/**     [Characteristic] Handle=0x0006 ValueHandle=0x0007 Type=00c5af5d-1964-4e30-8f51-1956f96bd283 Properties=(Read) */
// DEFINE_GUID(UNKNOWN_1956f96bd283,   0x00c5af5d, 0x1964, 0x4e30, 0x8f, 0x51, 0x19, 0x56, 0xf9, 0x6b, 0xd2, 0x83);

/** [Service] Handle=0x0008 Type=ab7de9be-89fe-49ad-828f-118f09df7fd0(Nintendo SW2) */
DEFINE_GUID(NINTENDO_SW2,           0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xd0);
extern __declspec(selectany) const PWSTR NINTENDO_SW2_NAME    = L"Nintendo SW2";
/**     [Characteristic] Handle=0x0009 ValueHandle=0x000a Type=ab7de9be-89fe-49ad-828f-118f09df7fd2(Input Report (format 0)) Properties=(Read/Notify) */
DEFINE_GUID(INPUT_REPORT_F0,        0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xd2);
extern __declspec(selectany) const PWSTR INPUT_REPORT_F0_NAME = L"Input report (format 0)";
/**         [Value] [040700000000000000004E78869A987D000000000000000000000000000000D40D000000000000000001000000000000000000000000000000000000000000] */
/**         [Descriptor]  Handle=0x000b Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x000c Type=679d5510-5a24-4dee-9557-95df80486ecb */
// DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
/**     [Characteristic] Handle=0x000d ValueHandle=0x000e Type=7492866c-ec3e-4619-8258-32755ffcc0f8(Input Report (format 3)) Properties=(Read/Notify) */
DEFINE_GUID(INPUT_REPORT_F3,        0x7492866c, 0xec3e, 0x4619, 0x82, 0x58, 0x32, 0x75, 0x5f, 0xfc, 0xc0, 0xf8);
extern __declspec(selectany) const PWSTR INPUT_REPORT_F3_NAME = L"Input report (format 3)";
/**         [Value] [C41C00000046588599A87D30000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000] */
/**         [Descriptor]  Handle=0x000f Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x0010 Type=679d5510-5a24-4dee-9557-95df80486ecb */
// DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
/**     [Characteristic] Handle=0x0011 ValueHandle=0x0012 Type=cc483f51-9258-427d-a939-630c31f72b05(Vibration/rumble output) Properties=(WriteWithoutResponse) */
DEFINE_GUID(RUMBLE_OUTPUT,          0xcc483f51, 0x9258, 0x427d, 0xa9, 0x39, 0x63, 0x0c, 0x31, 0xf7, 0x2b, 0x05);
extern __declspec(selectany) const PWSTR RUMBLE_OUTPUT_NAME = L"Vibration/rumble output";
/**     [Characteristic] Handle=0x0013 ValueHandle=0x0014 Type=649d4ac9-8eb7-4e6c-af44-1ea54fe5f005(Command channel) Properties=(WriteWithoutResponse) */
DEFINE_GUID(COMMAND_CHANNEL,        0x649d4ac9, 0x8eb7, 0x4e6c, 0xaf, 0x44, 0x1e, 0xa5, 0x4f, 0xe5, 0xf0, 0x05);
extern __declspec(selectany) const PWSTR COMMAND_CHANNEL_NAME = L"Command channel";
/**     [Characteristic] Handle=0x0015 ValueHandle=0x0016 Type=3dacbc7e-6955-40b5-8eaf-6f9809e8b379(Command + rumble prefix channel) Properties=(WriteWithoutResponse) */
DEFINE_GUID(COMMAND_RUMBLE_CHANNEL, 0x3dacbc7e, 0x6955, 0x40b5, 0x8e, 0xaf, 0x6f, 0x98, 0x09, 0xe8, 0xb3, 0x79);
extern __declspec(selectany) const PWSTR COMMAND_RUMBLE_CHANNEL_NAME = L"Command + rumble prefix channel";
/**     [Characteristic] Handle=0x0017 ValueHandle=0x0018 Type=4147423d-fdae-4df7-a4f7-d23e5df59f8d Properties=(WriteWithoutResponse) */
// DEFINE_GUID(UNKNOWN_d23e5df59f8d,   0x4147423d, 0xfdae, 0x4df7, 0xa4, 0xf7, 0xd2, 0x3e, 0x5d, 0xf5, 0x9f, 0x8d);
/**     [Characteristic] Handle=0x0019 ValueHandle=0x001a Type=c765a961-d9d8-4d36-a20a-5315b111836a(Command response/ACK) Properties=(Notify) */
DEFINE_GUID(COMMAND_RESPONSE_ACK,   0xc765a961, 0xd9d8, 0x4d36, 0xa2, 0x0a, 0x53, 0x15, 0xb1, 0x11, 0x83, 0x6a);
extern __declspec(selectany) const PWSTR COMMAND_RESPONSE_ACK_NAME = L"Command response/ACK";
/**         [Descriptor]  Handle=0x001b Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x001c Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
/**     [Characteristic] Handle=0x001d ValueHandle=0x001e Type=506d9f7d-4278-4e95-a549-326ba77657e0 Properties=(Notify) */
// DEFINE_GUID(UNKNOWN_326ba77657e0,   0x506d9f7d, 0x4278, 0x4e95, 0xa5, 0x49, 0x32, 0x6b, 0xa7, 0x76, 0x57, 0xe0);
/**         [Descriptor]  Handle=0x001f Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x0020 Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
/**     [Characteristic] Handle=0x0021 ValueHandle=0x0022 Type=d3bd69d2-841c-4241-ab15-f86f406d2a80 Properties=(Notify) */
// DEFINE_GUID(UNKNOWN_f86f406d2a80,   0xd3bd69d2, 0x841c, 0x4241, 0xab, 0x15, 0xf8, 0x6f, 0x40, 0x6d, 0x2a, 0x80);
/**         [Descriptor]  Handle=0x0023 Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x0024 Type=b746df8c-f358-495b-9cd2-e3bbeda4f979 */
// DEFINE_GUID(UNKNOWN_e3bbeda4f979,   0xb746df8c, 0xf358, 0x495b, 0x9c, 0xd2, 0xe3, 0xbb, 0xed, 0xa4, 0xf9, 0x79);
/**     [Characteristic] Handle=0x0025 ValueHandle=0x0026 Type=ab7de9be-89fe-49ad-828f-118f09df7fde Properties=(Read/Notify) */
// DEFINE_GUID(UNKNOWN_118f09df7fde,   0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xde);
/**         [Descriptor]  Handle=0x0027 Type=0x2902(Client Configuration) */
/**             [Value]  IsSubscribeToNotification */
/**         [Descriptor]  Handle=0x0028 Type=679d5510-5a24-4dee-9557-95df80486ecb */
// DEFINE_GUID(UNKNOWN_95df80486ecb,   0x679d5510, 0x5a24, 0x4dee, 0x95, 0x57, 0x95, 0xdf, 0x80, 0x48, 0x6e, 0xcb);
/**     [Characteristic] Handle=0x0029 ValueHandle=0x002a Type=ab7de9be-89fe-49ad-828f-118f09df7fdf Properties=(WriteWithoutResponse) */
// DEFINE_GUID(UNKNOWN_118f09df7fdf,   0xab7de9be, 0x89fe, 0x49ad, 0x82, 0x8f, 0x11, 0x8f, 0x09, 0xdf, 0x7f, 0xdf);

} // namespace nsw2ble
