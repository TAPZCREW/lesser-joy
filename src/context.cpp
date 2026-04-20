module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthioctl.h>
#include <bthddi.h>

module nsw2ble.context;

namespace nsw2ble {
  auto DeviceContext::init(WDFDEVICE device) -> NTSTATUS {
    return header.init(device);
  }

/*++

Description:

    Initializes the common context header between server and client

Arguments:

    Header - Contex header
    Device - Framework device object

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::init(WDFDEVICE device) -> NTSTATUS {
    device   = device;
    io_target = WdfDeviceGetIoTarget(device);

    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    auto status = WdfRequestCreate(&attributes, io_target, &request);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    return STATUS_SUCCESS;
  }

/*++

Description:

    Retrieves the local bth address.
    This address is burnt into device hence doesn't change.

    It also retrieves the local host supported features if available.

Arguments:

    Header - Contex header

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::retrieve_local_info() -> NTSTATUS {
    auto brb = (struct _BRB_GET_LOCAL_BD_ADDR *)profile_drv_interface.BthAllocateBrb(
      BRB_HCI_GET_LOCAL_BD_ADDR, 
      POOLTAG_NSW2BLE
    );
    if (brb == nullptr) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    auto status = send_brb_sync(io_target, request, (BRB*)brb, sizeof(*brb));
    if (not NT_SUCCESS(status)) {
      profile_drv_interface.BthFreeBrb((BRB*)brb);
      return status;
    }

    local_bth_addr = brb->BtAddress;

    status = get_host_supported_features();
    if (not NT_SUCCESS(status)) {
      profile_drv_interface.BthFreeBrb((BRB*)brb);
      return status;
    }

    profile_drv_interface.BthFreeBrb((BRB*)brb);
    return STATUS_SUCCESS;
  }

/*++

Routine Description:

    This routine synchronously checks the local stack's supported features

Arguments:

    DevCtxHdr - Information about the local device

Return Value:

    NTSTATUS Status code.

--*/
  auto ContextHeader::get_host_supported_features() -> NTSTATUS {
    local_features.Mask = 0;

    WDF_MEMORY_DESCRIPTOR out_mem_desc;
    BTH_HOST_FEATURE_MASK features;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_mem_desc, &features, sizeof(features));

    auto status = WdfIoTargetSendIoctlSynchronously(
      io_target,
      nullptr,
      IOCTL_BTH_GET_HOST_SUPPORTED_FEATURES,
      nullptr,
      &out_mem_desc,
      nullptr,
      nullptr
    );

    if (not NT_SUCCESS(status)) {
        return status;
    }

    local_features = features;

    return STATUS_SUCCESS;
  }

/*++

Routine Description:

    This routine formats a request with brb and sends it asynchronously

Arguments:

    IoTarget - Target to send the brb to
    Request - request object to be formatted with brb                
    Brb - Brb to be sent
    BrbSize - size of the Brb data structure
    ComplRoutine - WDF completion routine for the request
                   This must be specified because we are formatting the request
                   and hence not using SEND_AND_FORGET flag
    Context - (optional) context to be passed in to the completion routine

Return Value:

    Success implies that request was sent correctly and completion routine will be called
    for it,
    failure implies it was not sent and caller should complete the request

Notes:

    This routine does not call WdfRequestReuse on the Request passed in.
    Caller must do so before passing in the request, if it is reusing the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.

--*/
  auto send_brb_async(
    WDFIOTARGET                        io_target,
    WDFREQUEST                         request,
    BRB*                               brb,
    size_t                             brb_size,
    PFN_WDF_REQUEST_COMPLETION_ROUTINE compl_routine,
    WDFCONTEXT                         context
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

    WdfRequestSetCompletionRoutine(request, compl_routine, context);
    if (not WdfRequestSend(request, io_target, nullptr)) {
      status = WdfRequestGetStatus(request);
      return status;
    }

    return STATUS_SUCCESS;
  }

/*++

Routine Description:

    This routine formats a request with brb and sends it synchronously

Arguments:

    IoTarget - Target to send the brb to
    Request - request object to be formatted with brb
    Brb - Brb to be sent
    BrbSize - size of the Brb data structure

Return Value:

    NTSTATUS Status code.

Notes:

    This routine does calls WdfRequestReuse on the Request passed in.
    Caller need not do so before passing in the request.

    This routine does not complete the request in case of failure.
    Caller must complete the request in case of failure.

--*/
  auto send_brb_sync(WDFIOTARGET io_target, WDFREQUEST request, BRB* brb, size_t brb_size) -> NTSTATUS {

    WDF_REQUEST_REUSE_PARAMS       reuse_params;
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);

    auto status = WdfRequestReuse(request, &reuse_params);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    WDF_MEMORY_DESCRIPTOR              memory_desc;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_desc, brb, brb_size);

    status = WdfIoTargetSendInternalIoctlOthersSynchronously(
      io_target,
      request,
      IOCTL_INTERNAL_BTH_SUBMIT_BRB,
      &memory_desc,
      nullptr, //OtherArg2
      nullptr, //OtherArg4
      nullptr, //RequestOptions
      nullptr  //BytesReturned
    );
    if (not NT_SUCCESS(status)) {
      return status;
    }

    return STATUS_SUCCESS;
  }
}
