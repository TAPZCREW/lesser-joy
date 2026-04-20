module;

#include <ntddk.h>
#include <wdf.h>

export module log;

import string_view;

export namespace nsw2ble {
  auto debug_log(StringView str) -> void;
  auto debug_logln(StringView str) -> void;
} // namespace nsw2ble

module :private;

namespace nsw2ble {
  auto debug_log(StringView str) -> void {
    DbgPrint("nsw2ble: ");
    DbgPrint(str.data());
  }

  auto debug_logln(StringView str) -> void {
    debug_log(str);
    DbgPrint("\n");
  }
} // namespace nsw2ble
