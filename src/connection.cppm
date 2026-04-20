module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <bthddi.h>

export module nsw2ble.connection;

import nsw2ble.context;

export namespace nsw2ble {
  struct ConnectionContext;

  struct RepeatReader {
    struct _BRB_L2CA_ACL_TRANSFER transfer_request;

    WDFREQUEST  pending_read_request;
    WDFMEMORY   pending_read_memory;

    KDPC resubmit;

    LONG   stopping;
    KEVENT on_stop;

    ConnectionContext* connection;

    auto submit(ContextHeader* header) -> NTSTATUS;
    auto uninitialize() -> void;
    auto init(ConnectionContext* connection, size_t buffer_size) -> NTSTATUS;
    auto cancel() -> void;
    auto wait_for_stop() -> void;
  };

  struct  ContinuousReader {
    static constexpr auto AMOUNT = 2uz;

    RepeatReader repeat_readers[AMOUNT];
    DWORD initialized_count;

    using ReadCompleteCallback = void (*)(void* buffer, size_t buffer_size);
    ReadCompleteCallback on_read_complete;
    using FailedCallback = void (*)();
    FailedCallback on_failed;

    auto init(
      ConnectionContext* connection,
      ReadCompleteCallback on_read_complete_cb,
      FailedCallback on_failed_cb,
      size_t buffer_size
    ) -> NTSTATUS;
    auto submit(ContextHeader* header) -> NTSTATUS;

    auto join() -> void;
    auto cancel() -> void;
  };

  // L2Ca connection
  struct ConnectionContext {
    // List entry for connection list maintained at device level
    LIST_ENTRY      list_entry;

    ContextHeader*   header;

    enum struct ConnectionState {
      UNITIALIZED = 0,
      INITIALIZED,
      CONNECTING,
      CONNECTED,
      CONNECTFAILED,
      DISCONNECTING,
      DISCONNECTED
    };
    ConnectionState state;
    WDFSPINLOCK     state_lock;

    USHORT out_mtu, in_mtu;

    L2CAP_CHANNEL_HANDLE channel_handle;
    BTH_ADDR             remote_address;

    struct _BRB connect_disconnect_brb;
    WDFREQUEST  connect_disconnect_request;
    KEVENT      disconnect_event; // active when connection_state != ConnectionState::DISCONNECTING

    // Continuous readers (used only by server)
    // PLEASE NOTE that KMDF USB Pipe Target uses a single continuous reader
    ContinuousReader continuous_reader;

    // auto on_reader_read_complete(void* buffer, size_t buffer_size) -> void;
    // auto on_reader_failed() -> void;

    static auto object_cleanup(WDFOBJECT connection) -> void;
    static auto object_init(WDFOBJECT connection, ContextHeader* header) -> NTSTATUS;
    static auto object_create(ContextHeader* header, WDFOBJECT parent, WDFOBJECT* connection) -> NTSTATUS;

    static auto disconnect_completion(
      WDFREQUEST request,
      WDFIOTARGET io_target,
      WDF_REQUEST_COMPLETION_PARAMS*  params,
      WDFCONTEXT context
    ) -> void;

    auto remote_disconnect() -> bool;
    auto remote_disconnect_sync() -> void;

    auto format_request_with_brb(
      WDFIOTARGET io_target,
      WDFREQUEST request,
      BRB* brb,
      size_t brb_size
    ) -> NTSTATUS;

    auto format_request_for_l2ca_transfer(
      WDFREQUEST request,
      struct _BRB_L2CA_ACL_TRANSFER ** brb,
      WDFMEMORY memory,
      ULONG transfer_flags //flags include direction of transfer
    ) -> NTSTATUS;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ConnectionContext, get_connection)

  struct FileContext {
    // Connection to server opened for this file
    ConnectionContext* connection;

    USHORT server_psm;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FileContext, get_file_context)

}
