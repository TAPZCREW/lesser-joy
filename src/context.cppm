module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthioctl.h>
#include <bthddi.h>

export module nsw2ble.context;

import string_view;

export namespace nsw2ble {
// see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-exallocatepool2
  constexpr auto POOLTAG_NSW2BLE = 'htbw';

  struct ContextHeader { 
    // Framework device this context is associated with
    WDFDEVICE   device;
    WDFIOTARGET io_target;
    
    // Profile driver interface which contains profile driver DDI
    BTH_PROFILE_DRIVER_INTERFACE profile_drv_interface;
    BTH_ADDR                     local_bth_addr;
    // Features supported by the local stack
    BTH_HOST_FEATURE_MASK        local_features;

    // Preallocated request to be reused during initialization/deinitialzation phase
    // Access to this reqeust is not synchronized
    WDFREQUEST request;

    auto init(WDFDEVICE device) -> NTSTATUS;

    auto retrieve_local_info() -> NTSTATUS;
    auto get_host_supported_features() -> NTSTATUS;
  };

  struct DeviceContext {
    ContextHeader header;
    BTH_ADDR      server_bth_address;

    auto init(WDFDEVICE device) -> NTSTATUS;
  };
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DeviceContext, get_device_context)

  using RequestContext = BRB;
  WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(RequestContext, get_request_context)

  auto send_brb_async(
    WDFIOTARGET                        io_target,
    WDFREQUEST                         request,
    BRB*                               brb,
    size_t                             brb_size,
    PFN_WDF_REQUEST_COMPLETION_ROUTINE compl_routine,
    WDFCONTEXT                         context
  ) -> NTSTATUS;

  auto send_brb_sync(WDFIOTARGET io_target, WDFREQUEST request, BRB* brb, size_t brb_size) -> NTSTATUS;
}
