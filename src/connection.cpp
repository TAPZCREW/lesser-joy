module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <bthddi.h>

module nsw2ble.connection;

// import nsw2ble.context;

namespace nsw2ble {
  auto resubmit_read_dpc(
    struct _KDPC  *dpc,
    PVOID  deferred_context,
    PVOID  system_argument_1,
    PVOID  system_argument_2
  ) -> void {
    auto reader = (RepeatReader*) system_argument_1;
    if (deferred_context != nullptr and reader != nullptr) {
      reader->submit((ContextHeader*)deferred_context);
    }
  }

  auto RepeatReader::uninitialize() -> void {
    if (pending_read_request != nullptr) {
      pending_read_request = nullptr;
      stopping = 0;
    }
  }

  auto RepeatReader::init(ConnectionContext* _connection, size_t buffer_size) -> NTSTATUS {
    connection = _connection;
    if (buffer_size <= 0) {
      return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = WdfObjectContextGetObject(connection);

    auto status = WdfRequestCreate(&attributes, connection->header->io_target, &pending_read_request);
    if (not NT_SUCCESS(status)) {
      uninitialize();
      return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = pending_read_request;

    status = WdfMemoryCreate(
      &attributes,
      NonPagedPoolNx,
      POOLTAG_NSW2BLE,
      buffer_size,
      &pending_read_memory,
      nullptr
    );
    if (not NT_SUCCESS(status)) {
      uninitialize();
      return status;
    }

    KeInitializeDpc(&resubmit, resubmit_read_dpc, connection->header);
    KeInitializeEvent(&on_stop, NotificationEvent, true);

    return STATUS_SUCCESS;
  }

  auto pending_read_completion(
    WDFREQUEST                      request,
    WDFIOTARGET                     target,
    WDF_REQUEST_COMPLETION_PARAMS*  params,
    WDFCONTEXT                      context
  ) -> void {
    auto reader = (RepeatReader*) context;
    NT_ASSERT(reader != nullptr);

    auto status = params->IoStatus.Status;
    if (not NT_SUCCESS(status)) {
      if (status != STATUS_CANCELLED) {
        reader->connection->continuous_reader.on_failed();
      }
      KeSetEvent(&reader->on_stop, 0, false);
      return;
    }

    reader->connection->continuous_reader.on_read_complete(
      reader->transfer_request.Buffer,
      reader->transfer_request.BufferSize
    );
    NT_ASSERT(
      KeInsertQueueDpc(&resubmit, reader, nullptr)
    );
  }
  
  auto RepeatReader::submit(ContextHeader* header) -> NTSTATUS {
    auto brb = (struct _BRB_L2CA_ACL_TRANSFER *)&transfer_request;
    header->profile_drv_interface.BthReuseBrb((BRB*)brb, BRB_L2CA_ACL_TRANSFER);

    WDF_REQUEST_REUSE_PARAMS       reuse_params;
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_UNSUCCESSFUL);
    NT_ASSERT(NT_SUCCESS(
      WdfRequestReuse(pending_read_request, &reuse_params);
    ));

    if (stopping) {
      KeSetEvent(&on_stop, 0, false);
      return STATUS_SUCCESS;
    }

    auto status = connection->format_request_for_l2ca_transfer(
      pending_read_request,
      &brb,
      pending_read_memory,
      ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK
    );
    if (not NT_SUCCESS(status)) {
      connection->continuous_reader.on_failed();
      KeSetEvent(&on_stop, 0, false);
      return status;
    }

    WdfRequestSetCompletionRoutine(pending_read_request, pending_read_completion, this);
    KeClearEvent(&on_stop);
    if (not WdfRequestSend(pending_read_request, header->io_target, nullptr)) {
      auto status = WdfRequestGetStatus(pending_read_request);
      connection->continuous_reader.on_failed();
      KeSetEvent(&on_stop, 0, false);
      return status;
    }

    return STATUS_SUCCESS;
  }

  auto RepeatReader::cancel() -> void {
    InterlockedIncrement(&stopping);
    WdfRequestCancelSentRequest(pending_read_request);
  }

  auto RepeatReader::wait_for_stop() -> void {
    KeWaitForSingleObject(&on_stop, Executive, KernelMode, false, nullptr);
  }

  auto ContinuousReader::init(
      ConnectionContext* connection,
      ReadCompleteCallback on_read_complete_cb,
      FailedCallback on_failed_cb,
      size_t buffer_size
  ) -> NTSTATUS {
    on_read_complete = on_read_complete_cb;
    on_failed        = on_failed_cb;
    for (auto r = repeat_readers; r < repeat_readers + initialized_count; r++) {
      auto status = r->init(connection, buffer_size);
      if (not NT_SUCCESS(status)) {
        cancel();
        return status;
      }
    }

    return STATUS_SUCCESS;
  }

  auto ContinuousReader::submit(ContextHeader* header) -> NTSTATUS {
    NT_ASSERT(
      initialized_count <= AMOUNT
    );

    for (auto r = repeat_readers; r < repeat_readers + initialized_count; r++) {
      auto status = r->submit(header);
      if (not NT_SUCCESS(status)) {
        cancel();
        return status;
      }
    }

    return STATUS_SUCCESS;
  }

  auto ContinuousReader::cancel() -> void {
    NT_ASSERT(
      initialized_count <= AMOUNT
    );

    for (auto r = repeat_readers; r < repeat_readers + initialized_count; r++) {
      r->cancel();
    }
  }

  auto ContinuousReader::join() -> void {
    NT_ASSERT(
      initialized_count <= AMOUNT
    );

    for (auto r = repeat_readers; r < repeat_readers + initialized_count; r++) {
      r->wait_for_stop();
      r->uninitialize();
    }
  }

  auto ConnectionContext::object_cleanup(WDFOBJECT connection) -> void {
    auto context = get_connection(connection);
    context->continuous_reader.join();
    KeWaitForSingleObject(&context->disconnect_event, Executive, KernelMode, false, nullptr);
    WdfObjectDelete(context->connect_disconnect_request);
  }

  auto ConnectionContext::object_init(WDFOBJECT connection, ContextHeader* header) -> NTSTATUS {
    auto context = get_connection(connection);
    context->header = header;
    context->state = ConnectionState::INITIALIZED; // <- INITIALIZING ?

    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);    
    auto status = WdfSpinLockCreate(&attributes, &context->state_lock);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    status = WdfRequestCreate(&attributes, header->io_target, &context->connect_disconnect_request);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    KeInitializeEvent(&context->disconnect_event, NotificationEvent, true);
    InitializeListHead(&context->list_entry);

    context->state = ConnectionState::INITIALIZED;
    return STATUS_SUCCESS;
  }

  auto ConnectionContext::object_create(ContextHeader* header, WDFOBJECT parent, WDFOBJECT* connection) -> NTSTATUS {
    WDF_OBJECT_ATTRIBUTES                    attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ConnectionContext);
    attributes.ParentObject       = parent;
    attributes.EvtCleanupCallback = object_cleanup;
    
    WDFOBJECT local_connection;
    auto status = WdfObjectCreate(&attributes, &local_connection);
    
    if (not NT_SUCCESS(status)) {
      if (local_connection != nullptr) {
        WdfObjectDelete(local_connection);
      }
      return status;
    }

    status = object_init(local_connection, header);
    
    if (not NT_SUCCESS(status)) {
      WdfObjectDelete(local_connection);
      return status;
    }

    *connection = local_connection;

    return STATUS_SUCCESS;
  }

  auto ConnectionContext::disconnect_completion(
    WDFREQUEST request,
    WDFIOTARGET io_target,
    WDF_REQUEST_COMPLETION_PARAMS*  params,
    WDFCONTEXT context
  ) -> void {
    auto connection = (ConnectionContext*) context;

    WdfSpinLockAcquire(connection->state_lock);
    connection->state = ConnectionState::DISCONNECTED;
    WdfSpinLockRelease(connection->state_lock);

    KeSetEvent(&connection->disconnect_event, 0, false);
  }
/*++

Description:

    This routine sends a disconnect BRB for the connection

Arguments:

    DevCtxHdr - Device context header
    Connection - Connection which is to be disconnected

Return Value:

    TRUE is this call initiates the disconnect.
    FALSE if the connection was already disconnected.

--*/
  auto ConnectionContext::remote_disconnect() -> bool {
    WdfSpinLockAcquire(state_lock);

    switch (state) {
      case ConnectionState::CONNECTING:
        state = ConnectionState::DISCONNECTING;
        KeClearEvent(&disconnect_event);
        WdfSpinLockRelease(state_lock);
        return true;

      case ConnectionState::CONNECTED:
        state = ConnectionState::DISCONNECTING;
        KeClearEvent(&disconnect_event);
        WdfSpinLockRelease(state_lock);
        break;

      default:
        WdfSpinLockRelease(state_lock);
        return false;
    }

    header->profile_drv_interface.BthReuseBrb(&connect_disconnect_brb, BRB_L2CA_CLOSE_CHANNEL);

    auto disconnect_brb = (struct _BRB_L2CA_CLOSE_CHANNEL *) &connect_disconnect_brb;
    disconnect_brb->BtAddress = remote_address;
    disconnect_brb->ChannelHandle = channel_handle;

    auto status = send_brb_async(
      header->io_target,
      connect_disconnect_request,
      (BRB*)disconnect_brb,
      sizeof(*disconnect_brb),
      disconnect_completion,
      this
    );

    // NT_ASSERT(
    //   NT_SUCCESS(status) or status == STATUS_DEVICE_DISCONNECT)
    // );

    return true;
  }

  auto ConnectionContext::remote_disconnect_sync() -> void {
    remote_disconnect();
    KeWaitForSingleObject(&disconnect_event, Executive, KernelMode, false, nullptr);
  }
/*++

Description:

    This routine formats are WDFREQUEST with the passed in BRB

Arguments:

    IoTarget - Target to which request will be sent
    Request - Request to be formattted
    Brb - BRB to format the request with
    BrbSize - size of the BRB

--*/
  auto ConnectionContext::format_request_with_brb(
    WDFIOTARGET io_target,
    WDFREQUEST request,
    BRB* brb,
    size_t brb_size
  ) -> NTSTATUS {
    if (brb_size <= 0) {
      return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    WDFMEMORY memory;
    auto status = WdfMemoryCreatePreallocated(&attributes, brb, brb_size, &memory);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    status = WdfIoTargetFormatRequestForInternalIoctlOthers(
      io_target,
      request,
      IOCTL_INTERNAL_BTH_SUBMIT_BRB,
      memory,
      nullptr, //OtherArg1Offset
      nullptr, //OtherArg2
      nullptr, //OtherArg2Offset
      nullptr, //OtherArg4
      nullptr  //OtherArg4Offset
    );
    if (not NT_SUCCESS(status)) {
      return status;
    }

    return STATUS_SUCCESS;
  }
/*++

Description:

    Formats a request for L2Ca transfer

Arguments:

    Connection - Connection on which L2Ca transfer will be made
    Request - Request to be formatted
    Brb - If a Brb is passed in, it will be used, otherwise
          this routine will allocate the Brb and return in this parameter
    Memory - Memory object which has the buffer for transfer
    TransferFlags - Transfer flags which include direction of the transfer

Return Value:

    NTSTATUS Status code.

--*/
  auto ConnectionContext::format_request_for_l2ca_transfer(
    WDFREQUEST request,
    struct _BRB_L2CA_ACL_TRANSFER ** brb,
    WDFMEMORY memory,
    ULONG transfer_flags //flags include direction of transfer
  ) -> NTSTATUS {
    WdfSpinLockAcquire(state_lock);
    auto c_state = state;
    WdfSpinLockRelease(state_lock);

    if (state != ConnectionState::CONNECTED) {
      return STATUS_CONNECTION_DISCONNECTED;
    }

    BRB* local_brb = nullptr;
    if (*brb != nullptr) {
      header->profile_drv_interface.BthReuseBrb((BRB*)*brb, BRB_L2CA_ACL_TRANSFER);
    }
    else {
      local_brb = header->profile_drv_interface.BthAllocateBrb(
        BRB_L2CA_ACL_TRANSFER, 
        POOLTAG_NSW2BLE
      );

      if (local_brb == nullptr) {
        return STATUS_INSUFFICIENT_RESOURCES;
      }

      *brb = (struct _BRB_L2CA_ACL_TRANSFER *) local_brb;
    }

    (*brb)->BtAddress = remote_address;
    (*brb)->BufferMDL = nullptr;

    size_t buffer_size;
    (*brb)->Buffer = WdfMemoryGetBuffer(memory, &buffer_size);
    if (buffer_size > -1) {
        return STATUS_BUFFER_OVERFLOW;
    }
    (*brb)->BufferSize = buffer_size;

    (*brb)->ChannelHandle = channel_handle;
    (*brb)->TransferFlags = transfer_flags;

    auto status = format_request_with_brb(
      header->io_target,
      request,
      (BRB*) brb,
      sizeof(*brb)
    );
    if (not NT_SUCCESS(status)) {
      if (local_brb != nullptr) {
        header->profile_drv_interface.BthFreeBrb(local_brb);
      }
      return status;
    }

    return STATUS_SUCCESS;
  }
}
