module;

#include <hidapi/hidapi.h>

export module hidapi;

export namespace hidapi {
    using ::hid_bus_type;
    using ::hid_device;
    using ::hid_device_info;

    using ::hid_close;
    using ::hid_enumerate;
    using ::hid_error;
    using ::hid_exit;
    using ::hid_init;
    using ::hid_open;
    using ::hid_send_feature_report;
    using ::hid_send_output_report;
    using ::hid_write;
} // namespace hidapi
