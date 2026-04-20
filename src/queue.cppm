module;

#include <ntddk.h>
#include <wdf.h>

export module nsw2ble.queue;

export namespace nsw2ble {

  auto on_queue_io_write(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void;
  auto on_queue_io_read(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void;
  auto on_queue_io_stop(WDFQUEUE queue, WDFREQUEST request, ULONG action_flags) -> void;

} // namespace nsw2ble
