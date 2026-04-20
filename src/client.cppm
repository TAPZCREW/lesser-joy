module;

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthsdpddi.h>

export module nsw2ble.client;

import nsw2ble.guids;
import nsw2ble.context;
import nsw2ble.connection;

export namespace nsw2ble {

  auto query_bluetooth_interfaces(DeviceContext* context) -> NTSTATUS;
  auto retrieve_server_bth_address(DeviceContext* context) -> NTSTATUS;
  auto retrieve_server_sdp_record(DeviceContext* context, BTH_SDP_STREAM_RESPONSE** server_sdp_record) -> NTSTATUS;
  auto retrieve_psm_from_sdp_record(
    BTHDDI_SDP_PARSE_INTERFACE* sdp_parse_interface,
    BTH_SDP_STREAM_RESPONSE* server_sdp_record,
    USHORT* psm
  ) -> NTSTATUS;
  auto open_remote_connection(DeviceContext* context, WDFFILEOBJECT file_object, WDFREQUEST request) -> NTSTATUS;

}
