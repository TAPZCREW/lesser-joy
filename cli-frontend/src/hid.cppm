module;

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/platform_macro.hpp>

export module lj.cli.hid;

import std;

import hidapi;

import stormkit.core;

import lj.cli.log;

using namespace stormkit;

using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj {
    enum class bus_type : u8 {
        usb,
        ble,
    };

    struct hid_device_entry final {
        bus_type bus;
        u16      vid;
        u16      pid;
        u16      release_number;
        u16      interface_number;
        string   serial_number;
        string   product_string;
        string   path;
    };

    enum class hid_status_code {
        failed_to_initialize,
        failed_to_send_output_report,
        failed_to_open_device,
    };

    // must match lj::hid::output_report_size / output_report_id in driver/src/constants.cppm.
    // the OS sizes every HID output report from HIDP_CAPS::OutputReportByteLength, which it derives from
    // the report descriptor the driver publishes -- so a CLI buffer that disagrees with that number is
    // rejected by its own driver.
    inline constexpr auto output_report_size = 64_usize;
    inline constexpr auto output_report_id   = 0x02_u8;

    class hid_device final {
        using private_tag = private_tag<hid_device>;

      public:
        hid_device(hid_device_entry entry, private_tag) noexcept;
        ~hid_device() noexcept;

        hid_device(const hid_device&)     = delete;
        auto operator=(const hid_device&) = delete;

        hid_device(hid_device&&) noexcept;
        auto operator=(hid_device&&) noexcept -> hid_device&;

        static auto open(const hid_device_entry& entry) noexcept -> system_result<hid_device>;

        auto send_output_report(array_view<const byte, output_report_size> report) const noexcept -> system_result<void>;

        auto handle() const noexcept -> hidapi::hid_device*;

        auto entry() const noexcept -> const hid_device_entry&;
        auto is_open() const noexcept -> bool;

      private:
        hidapi::hid_device* hid_handle_ = nullptr;
        hid_device_entry    entry_;
    };

    auto enumerate_switch2_controllers() noexcept -> system_result<dynarray<hid_device_entry>>;

    /////////////////////////////////////
    /////////////////////////////////////
    STORMKIT_FORCE_INLINE STORMKIT_CONST
    constexpr auto tag_invoke(as_fn<string_view>, bus_type bus, const std::source_location&) noexcept -> string_view {
        switch (bus) {
            case bus_type::usb: return "bus_type::usb";
            case bus_type::ble: return "bus_type::ble";
            default: break;
        }

        std::unreachable();
    }

    /////////////////////////////////////
    /////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto hid_error(std::optional<ref_ptr<const hid_device>> device = std::nullopt) -> string {
        const auto handle = device.has_value() ? (*device)->handle() : raw_ptr<hidapi::hid_device> { nullptr };

        auto werr = hidapi::hid_error(handle);
        auto err  = wide_to_ascii({ werr });

        return err;
    }

    constexpr auto status_code(hid_status_code code) noexcept -> system_error2::quick_status_code_from_enum_code<hid_status_code>;
} // namespace lj

export template<>
struct system_error2::quick_status_code_from_enum<lj::hid_status_code>
    : quick_status_code_from_enum_defaults<lj::hid_status_code> {
    static constexpr auto domain_name = "lesserjoy hid";
    static constexpr auto domain_uuid = "{081670da-d7b3-4300-9d1d-703223b19ea5}";

    static constexpr auto value_mappings() noexcept -> const std::initializer_list<mapping>&;
};

namespace system_error2 {
    /////////////////////////////////////
    /////////////////////////////////////
    inline constexpr auto quick_status_code_from_enum<lj::hid_status_code>::value_mappings() noexcept
      -> const std::initializer_list<mapping>& {
        static constexpr auto mapper = std::initializer_list<mapping> {
            { lj::hid_status_code::failed_to_initialize,         "Failed to initialize",         { errc::unknown } },
            { lj::hid_status_code::failed_to_open_device,        "Failed to open device",        { errc::unknown } },
            { lj::hid_status_code::failed_to_send_output_report, "Failed to send output report", { errc::unknown } },
        };
        return mapper;
    }
} // namespace system_error2

namespace lj {
    /////////////////////////////////////
    /////////////////////////////////////
    STORMKIT_FORCE_INLINE
    constexpr auto status_code(hid_status_code code) noexcept
      -> system_error2::quick_status_code_from_enum_code<hid_status_code> {
        return code;
    }
} // namespace lj

module: private;

namespace lj {
    namespace {
        constexpr auto pid = 0x2069;
        constexpr auto vid = 0x057E;

        const struct hid_initializer_type final {
            hid_initializer_type() {
                if (not initialized_) {
                    if (hidapi::hid_init() == 0) initialized_ = true;
                }

                assert(initialized_, std::format("Failed to init hid, reason: {}", hid_error()));
            }

            ~hid_initializer_type() {
                if (initialized_) hidapi::hid_exit();
            }

            bool initialized_ = false;
        } hidapi_initializer {};

    } // namespace

    /////////////////////////////////////
    /////////////////////////////////////
    hid_device::hid_device(hid_device_entry entry, private_tag) noexcept : entry_ { entry } {
    }

    /////////////////////////////////////
    /////////////////////////////////////
    hid_device::~hid_device() noexcept {
        if (hid_handle_ != nullptr) {
            hidapi::hid_close(hid_handle_);
            hid_handle_ = nullptr;
        }
    }

    /////////////////////////////////////
    /////////////////////////////////////
    hid_device::hid_device(hid_device&& other) noexcept
        : hid_handle_ { std::exchange(other.hid_handle_, nullptr) }, entry_ { std::move(other.entry_) } {
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::operator=(hid_device&& other) noexcept -> hid_device& {
        if (&other == this) [[unlikely]]
            return *this;

        hid_handle_ = std::exchange(other.hid_handle_, nullptr);
        entry_      = std::move(other.entry_);

        return *this;
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::open(const hid_device_entry& entry) noexcept -> system_result<hid_device> {
        auto device = hid_device { entry, private_token<hid_device> };

        const auto wserial = ascii_to_wide(entry.serial_number);
        device.hid_handle_ = hidapi::hid_open(entry.vid, entry.pid, stdr::data(wserial));
        if (device.hid_handle_ == nullptr) return { std::unexpected { status_code(hid_status_code::failed_to_open_device) } };

        return { std::move(device) };
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::send_output_report(array_view<const byte, output_report_size> report) const noexcept
      -> system_result<void> {
        EXPECTS(is_open());
        EXPECTS(report[0] == as<byte>(output_report_id));

        auto result = hidapi::hid_send_output_report(hid_handle_,
                                                     reinterpret_cast<const uchar*>(stdr::data(report)),
                                                     stdr::size(report));
        if (result == -1) return std::unexpected { status_code(hid_status_code::failed_to_send_output_report) };

        return {};
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::handle() const noexcept -> hidapi::hid_device* {
        EXPECTS(is_open());

        return hid_handle_;
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::entry() const noexcept -> const hid_device_entry& {
        return entry_;
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto hid_device::is_open() const noexcept -> bool {
        return hid_handle_ != nullptr;
    }

    /////////////////////////////////////
    /////////////////////////////////////
    auto enumerate_switch2_controllers() noexcept -> system_result<dynarray<hid_device_entry>> {
        auto out                 = dynarray<hid_device_entry> {};
        auto switch2_controllers = raw_ptr<hidapi::hid_device_info> { nullptr };

        switch2_controllers = hidapi::hid_enumerate(vid, pid);

        for (; switch2_controllers != nullptr; switch2_controllers = switch2_controllers->next) {
            const auto bus = switch2_controllers->bus_type == hidapi::hid_bus_type::HID_API_BUS_BLUETOOTH
                               ? bus_type::ble
                               : bus_type::usb;

            auto serial_number  = wide_to_ascii(switch2_controllers->serial_number);
            auto product_string = wide_to_ascii(switch2_controllers->product_string);

            out.emplace_back(bus,
                             switch2_controllers->vendor_id,
                             switch2_controllers->product_id,
                             switch2_controllers->release_number,
                             switch2_controllers->interface_number,
                             std::move(serial_number),
                             std::move(product_string),
                             switch2_controllers->path);
        }
        return { std::move(out) };
    }
} // namespace lj
