module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthddi.h>

module nsw2ble.queue;

import nsw2ble.context;
import nsw2ble.connection;

namespace nsw2ble {
/*++
Description:

    Completion routine for read/write requests

    We receive l2ca transfer BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.
    
Arguments:

    Request - Request that got completed
    Target - Target to which request was sent
    Params - Completion parameters for the request
    Context - We receive BRB as the context

--*/
  auto on_read_write_completion(
    WDFREQUEST                     request,
    WDFIOTARGET                    target,
    PWDF_REQUEST_COMPLETION_PARAMS params,
    WDFCONTEXT                     context
  ) -> void {
    auto brb = (struct _BRB_L2CA_ACL_TRANSFER *) context;
    NT_ASSERT((brb != nullptr));

    //
    // Bytes read/written are contained in brb->BufferSize
    //
    auto information = brb->BufferSize;

    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(
      request,
      params->IoStatus.Status,
      information
    );
  }

/*++
Description:

    This routine is invoked by the framework to deliver a
    Write request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Write request
    Length - Length of write

--*/
  auto on_queue_io_write(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    WDFMEMORY memory;
    auto status = WdfRequestRetrieveInputMemory(request, &memory);
    if (not NT_SUCCESS(status)) {
      WdfRequestComplete(request, status);
      return;
    }

    auto context = get_device_context(WdfIoQueueGetDevice(queue));
    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_ACL_TRANSFER BRB
    //
    auto brb = (struct _BRB_L2CA_ACL_TRANSFER *)get_request_context(request);
    context->header.profile_drv_interface.BthReuseBrb(
      (BRB*)brb,
      BRB_L2CA_ACL_TRANSFER
    );

    //
    // Format the Write request for L2Ca OUT transfer
    //
    // This routine allocates a BRB which is returned to us
    // in brb parameter
    //
    // This BRB is freed by the completion routine if we send the
    // request successfully, else it is freed by this routine.
    //
    auto connection = get_file_context(WdfRequestGetFileObject(request))->connection;
    status = connection->format_request_for_l2ca_transfer(
      request,
      &brb,
      memory,
      ACL_TRANSFER_DIRECTION_OUT
    );
    if (not NT_SUCCESS(status)) {
      WdfRequestComplete(request, status);
      return;
    }

    WdfRequestSetCompletionRoutine(request, on_read_write_completion, brb);
    if (not WdfRequestSend(request, context->header.io_target, nullptr)) {
      status = WdfRequestGetStatus(request);
      WdfRequestComplete(request, status);
      return;
    }
  }

/*++
Description:

    This routine is invoked by the framework to deliver a
    Read request to the driver.

Arguments:

    Queue - Queue delivering the request
    Request - Read request
    Length - Length of Read

--*/
  auto on_queue_io_read(WDFQUEUE queue, WDFREQUEST request, size_t length) -> void {
    WDFMEMORY memory;
    auto status = WdfRequestRetrieveOutputMemory(request, &memory);
    if (not NT_SUCCESS(status)) {
      WdfRequestComplete(request, status);
      return;
    }

    auto context = get_device_context(WdfIoQueueGetDevice(queue));
    auto brb = (struct _BRB_L2CA_ACL_TRANSFER *)get_request_context(request);
    context->header.profile_drv_interface.BthReuseBrb(
      (BRB*)brb,
      BRB_L2CA_ACL_TRANSFER
    );

    auto connection = get_file_context(WdfRequestGetFileObject(request))->connection;
    status = connection->format_request_for_l2ca_transfer(
      request,
      &brb,
      memory,
      ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK
    );
    if (not NT_SUCCESS(status)) {
      WdfRequestComplete(request, status);
      return;
    }

    WdfRequestSetCompletionRoutine(request, on_read_write_completion, brb);
    if (not WdfRequestSend(request, context->header.io_target, nullptr)) {
      status = WdfRequestGetStatus(request);
      WdfRequestComplete(request, status);
      return;
    }
  }

/*++
Description:

    This routine is invoked by Framework when Queue is being stopped

    We implement this routine to cancel any requests owned by our driver.
    
    Without this Queue stop would wait indefinitely for requests to complete
    during surprise remove.

Arguments:

    Queue - Framework queue being stopped
    Request - Request owned by the driver
    ActionFlags - Action flags

--*/
  auto on_queue_io_stop(WDFQUEUE queue, WDFREQUEST request, ULONG action_flags) -> void {
    WdfRequestCancelSentRequest(request);
  }
}
