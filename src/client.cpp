module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>

module nsw2ble.client;

import nsw2ble.guids;
import nsw2ble.context;
import nsw2ble.connection;

namespace nsw2ble {
/*++

Description:

    Query profile driver interface

Arguments:

    DevCtx - Client context where we store the interface

Return Value:

    NTSTATUS Status code.

--*/
  auto query_bluetooth_interfaces(DeviceContext* context) -> NTSTATUS {
    auto status = WdfFdoQueryForInterface(
      context->header.device,
      &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE, 
      (PINTERFACE) (&context->header.profile_drv_interface),
      sizeof(context->header.profile_drv_interface), 
      BTHDDI_PROFILE_DRIVER_INTERFACE_VERSION_FOR_QI, 
      nullptr
    );
    return status;
  }

/*++

Description:

    Retrieve server Bth address

Arguments:

    DevCtx - Client context where we store bth address

Return Value:

    NTSTATUS Status code.

--*/
  auto retrieve_server_bth_address(DeviceContext* context) -> NTSTATUS {
    WDF_REQUEST_REUSE_PARAMS       reuse_params;
    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    NT_ASSERT(NT_SUCCESS(
      WdfRequestReuse(context->header.request, &reuse_params)
    ));

    BTH_DEVICE_INFO server_device_info;
    RtlZeroMemory(&server_device_info, sizeof(server_device_info));

    WDF_MEMORY_DESCRIPTOR memory_desc;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
      &memory_desc,
      &server_device_info,
      sizeof(server_device_info)
    );

    auto status = WdfIoTargetSendInternalIoctlSynchronously(
      context->header.io_target,
      context->header.request,
      IOCTL_INTERNAL_BTHENUM_GET_DEVINFO,
      nullptr,
      &memory_desc,
      nullptr,
      nullptr
    );
    if (not NT_SUCCESS(status)) {
      return status;
    }

    context->server_bth_address = server_device_info.address;

    return STATUS_SUCCESS;
  }

/*++

Description:

    Retrive server SDP record.
    We call this function on every file open to get the PSM

Arguments:

    DevCtx - Client context
    ServerSdpRecord - SDP record retrieved

Return Value:

    NTSTATUS Status code.

--*/
  auto retrieve_server_sdp_record(DeviceContext* context, BTH_SDP_STREAM_RESPONSE** server_sdp_record) -> NTSTATUS {
    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    WDFREQUEST request;
    auto status = WdfRequestCreate(&attributes, context->header.io_target, &request);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    BTH_SDP_CONNECT connect;
    connect.bthAddress = context->server_bth_address;
    connect.requestTimeout = SDP_REQUEST_TO_DEFAULT;
    connect.fSdpConnect = 0;

    WDF_MEMORY_DESCRIPTOR              in_memory_desc;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &connect, sizeof(connect));

    WDF_MEMORY_DESCRIPTOR              out_memory_desc;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, &connect, sizeof(connect));

    status = WdfIoTargetSendIoctlSynchronously(
      context->header.io_target,
      request,
      IOCTL_BTH_SDP_CONNECT,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,   //sendOptions
      nullptr    //bytesReturned
    );
    if (not NT_SUCCESS(status)) {
      WdfObjectDelete(request);
      return status;
    }

    WDF_REQUEST_REUSE_PARAMS  reuse_params;
    const auto disconnect = [&reuse_params, context, &request, &in_memory_desc, &out_memory_desc, &connect] {
      WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
      NT_ASSERT(NT_SUCCESS(
        WdfRequestReuse(request, &reuse_params)
      ));

      BTH_SDP_CONNECT disconnect;
      disconnect.hConnection = connect.hConnection;
      WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &disconnect, sizeof(disconnect));

      return WdfIoTargetSendIoctlSynchronously(
        context->header.io_target,
        request,
        IOCTL_BTH_SDP_DISCONNECT,
        &in_memory_desc,
        nullptr,
        nullptr,
        nullptr
      );
    };

    BTH_SDP_SERVICE_ATTRIBUTE_SEARCH_REQUEST request_sdp;
    request_sdp.hConnection = connect.hConnection;
    request_sdp.uuids[0].u.uuid128 = NINTENDO_SW2;
    request_sdp.uuids[0].uuidType = SDP_ST_UUID128;
    request_sdp.range[0].minAttribute = 0;
    request_sdp.range[0].maxAttribute = 0xFFFF;

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    NT_ASSERT(NT_SUCCESS(
      WdfRequestReuse(request, &reuse_params)
    ));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &request_sdp, sizeof(request_sdp));
    BTH_SDP_STREAM_RESPONSE response_sdp;
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, &response_sdp, sizeof(response_sdp));

    status = WdfIoTargetSendIoctlSynchronously(
      context->header.io_target,
      request,
      IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,   //sendOptions
      nullptr    //bytesReturned
    );
    if (not NT_SUCCESS(status)) {
      NT_ASSERT(NT_SUCCESS(
        disconnect()
      ));
      WdfObjectDelete(request);
      return status;
    }

    ULONG request_size;
    status = RtlULongAdd(response_sdp.requiredSize, sizeof(BTH_SDP_STREAM_RESPONSE), &request_size);
    if (not NT_SUCCESS(status)) {
      NT_ASSERT(NT_SUCCESS(
        disconnect()
      ));
      WdfObjectDelete(request);
      return status;
    }

    *server_sdp_record = (BTH_SDP_STREAM_RESPONSE*)ExAllocatePoolZero(
      (enum _POOL_TYPE)POOL_FLAG_NON_PAGED,
      request_size,
      POOLTAG_NSW2BLE
    );
    // *server_sdp_record = ExAllocatePool2(POOL_FLAG_NON_PAGED, request_size, POOLTAG_NSW2BLE);
    if (server_sdp_record == nullptr) {
      NT_ASSERT(NT_SUCCESS(
        disconnect()
      ));
      WdfObjectDelete(request);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    WDF_REQUEST_REUSE_PARAMS_INIT(&reuse_params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    NT_ASSERT(NT_SUCCESS(
      WdfRequestReuse(request, &reuse_params)
    ));

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&in_memory_desc, &request_sdp, sizeof(request_sdp));
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&out_memory_desc, &server_sdp_record, request_size);

    status = WdfIoTargetSendIoctlSynchronously(
      context->header.io_target,
      request,
      IOCTL_BTH_SDP_SERVICE_ATTRIBUTE_SEARCH,
      &in_memory_desc,
      &out_memory_desc,
      nullptr,
      nullptr
    );
    if (not NT_SUCCESS(status)) {
      NT_ASSERT(NT_SUCCESS(
        disconnect()
      ));
      WdfObjectDelete(request);
      return status;
    }

    NT_ASSERT(NT_SUCCESS(
      disconnect()
    ));
    WdfObjectDelete(request);
    return STATUS_SUCCESS;
  }

/*++

Description:

    Retrieve PSM from the SDP record

Arguments:

    sdpParseInterface - Parse interface used for sdp parse functions
    ServerSdpRecord - SDP record to obtain Psm from
    Psm - Psm retrieved

Return Value:

    NTSTATUS Status code.

--*/
  auto retrieve_psm_from_sdp_record(
    BTHDDI_SDP_PARSE_INTERFACE* sdp_parse_interface,
    BTH_SDP_STREAM_RESPONSE* server_sdp_record,
    USHORT* psm
  ) -> NTSTATUS {
    // https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/host/service-discovery-protocol--sdp--specification.html
    
    UCHAR* next_element;
    ULONG  next_element_size;
    sdp_parse_interface->SdpGetNextElement(
      &(server_sdp_record->response[0]),
      server_sdp_record->responseSize,
      nullptr,
      &next_element,
      &next_element_size
    );
    if (next_element_size == 0) {
      return STATUS_DEVICE_DATA_ERROR;
    }

    SDP_TREE_ROOT_NODE* sdp_tree;
    auto status = sdp_parse_interface->SdpConvertStreamToTree(
      next_element,
      next_element_size,
      &sdp_tree,
      POOLTAG_NSW2BLE
    );
    if (not NT_SUCCESS(status)) {
      if (sdp_tree != nullptr) {
        sdp_parse_interface->SdpFreeTree(sdp_tree);
      }
      return status;
    }

    SDP_NODE* node_proto_desc_list;
    status = sdp_parse_interface->SdpFindAttributeInTree(
      sdp_tree,
      (USHORT)SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST,
      &node_proto_desc_list
    );
    if (not NT_SUCCESS(status)) {
      sdp_parse_interface->SdpFreeTree(sdp_tree);
      return status;
    }

    if (node_proto_desc_list->hdr.Type != SDP_TYPE_SEQUENCE
     or node_proto_desc_list->u.sequence.Link.Flink == nullptr) {
      sdp_parse_interface->SdpFreeTree(sdp_tree);
      return STATUS_DEVICE_DATA_ERROR;
    }

    auto node_proto_0 = CONTAINING_RECORD(node_proto_desc_list->u.sequence.Link.Flink, SDP_NODE, hdr.Link);
    if (node_proto_0->hdr.Type != SDP_TYPE_SEQUENCE
     or node_proto_0->u.sequence.Link.Flink == nullptr) {
      sdp_parse_interface->SdpFreeTree(sdp_tree);
      return STATUS_DEVICE_DATA_ERROR;
    }

    auto node_proto_0_UUID = CONTAINING_RECORD(node_proto_0->u.sequence.Link.Flink, SDP_NODE, hdr.Link);
    if (node_proto_0_UUID->hdr.Type != SDP_TYPE_UUID
     or node_proto_0_UUID->hdr.Link.Flink == nullptr) {
      sdp_parse_interface->SdpFreeTree(sdp_tree);
      return STATUS_DEVICE_DATA_ERROR;
    }

    auto node_proto_0_s_param_0 = CONTAINING_RECORD(node_proto_0_UUID->hdr.Link.Flink, SDP_NODE, hdr.Link);
    if (node_proto_0_s_param_0->hdr.SpecificType != SDP_ST_UINT16) {
      sdp_parse_interface->SdpFreeTree(sdp_tree);
      return STATUS_DEVICE_DATA_ERROR;
    }

    *psm = node_proto_0_s_param_0->u.uint16;

    sdp_parse_interface->SdpFreeTree(sdp_tree);
    return STATUS_SUCCESS;
  }

/*++

Description:

    Indication callback passed to bth stack while sending open channel BRB
    Bth stack sends notification related to the connection.
    
Arguments:

    Context - We receive data connection as the context
    Indication - Type of indication
    Parameters - Parameters of indication

--*/
  auto indication_callback(
    VOID* void_connection,
    INDICATION_CODE indication,
    INDICATION_PARAMETERS_ENHANCED* parameters
  ) -> void {
    auto connection = (ConnectionContext*) void_connection;

    //
    // Only supporting connect and disconnect
    //
    
    switch(indication) {
      //
      // We don't add/release reference to anything because our connection
      // is scoped within file object lifetime
      //
      case IndicationAddReference:
      case IndicationReleaseReference:
        break;
      case IndicationRemoteConnect:
        //
        // We don't expect connection
        //
        NT_ASSERT(false);
        break;
      case IndicationRemoteDisconnect:
        //
        // This is an indication that server has disconnected
        // In response we disconnect from our end
        //
        
        connection->remote_disconnect();

        break;
      default:
        break;
    }
  }

  auto on_connection_state_connected(WDFFILEOBJECT file_object, ConnectionContext* connection) -> NTSTATUS;

/*++
Description:

    Completion routine for Create request which we format as open
    channel BRB and send down the stack. We complete the Create request 
    in this routine.

    We receive open channel BRB as the context. This BRB
    is part of the request context and doesn't need to be freed
    explicitly.

    Connection is part of the context in the BRB.

Arguments:

    Request - Create request that we formatted with open channel BRB
    Target - Target to which we sent the request
    Params - Completion params
    Context - We receive BRB as the context          

Return Value:

    NTSTATUS Status code.
--*/
  auto remote_connect_completion(
    WDFREQUEST request,
    WDFIOTARGET target,
    WDF_REQUEST_COMPLETION_PARAMS* params,
    WDFCONTEXT brb_context
  ) -> void {
    auto context = get_device_context(WdfIoTargetGetDevice(target));
    auto status = params->IoStatus.Status;
    // TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
    //     "Connection completion, status: %!STATUS!", status);        

    auto brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *) brb_context;
    auto connection = (ConnectionContext*) brb->Hdr.ClientContext[0];

    //
    // In the client we don't check for ConnectionStateDisconnecting state 
    // because only file close generates disconnect which
    // cannot happen before create completes. And we complete Create
    // only after we process this completion.
    //
    if (not NT_SUCCESS(status)) {
        connection->state = ConnectionContext::ConnectionState::CONNECTFAILED;
        WdfRequestComplete(request, status);
        return;
    }

    connection->out_mtu = brb->OutResults.Params.Mtu;
    connection->in_mtu = brb->InResults.Params.Mtu;
    connection->channel_handle = brb->ChannelHandle;
    connection->remote_address = brb->BtAddress;

    connection->state = ConnectionContext::ConnectionState::CONNECTED;

    // TraceEvents(TRACE_LEVEL_INFORMATION, DBG_CONNECT, 
    //     "Connection (0x%x) established to server", brb->OutResults.Params.RetransmissionAndFlow.Mode); 

    //
    // Call the function in device.c (BthEchoCliConnectionStateConnected)
    // for any post processing after connection has been established
    //
    status = on_connection_state_connected(WdfRequestGetFileObject(request), connection);
    if (not NT_SUCCESS(status)) {
        connection->remote_disconnect();
        WdfRequestComplete(request, status);
        return;
    }

    //
    // Complete the Create request
    //
    WdfRequestComplete(request, status);

    return;    
  }

/*++

Description:

    This routine is invoked by BthEchoCliEvtDeviceFileCreate.
    In this routine we send down open channel BRB.

    This routine allocates open channel BRB. If the request
    is sent down successfully completion routine needs to free
    this BRB.
    
Arguments:

    _In_ PBTHECHOSAMPLE_CLIENT_CONTEXT DevCtx - 
    _In_ WDFFILEOBJECT FileObject - 
    _In_ WDFREQUEST Request - 

Return Value:

    NTSTATUS Status code.

--*/
  auto open_remote_connection(DeviceContext* context, WDFFILEOBJECT file_object, WDFREQUEST request) -> NTSTATUS {
    //
    // Create the connection object that would store information
    // about the open channel
    //
    // Set file object as the parent for this connection object
    //
    WDFOBJECT connection_object;
    auto status = ConnectionContext::object_create(
      &context->header,
      file_object, //parent
      &connection_object
    );

    if (not NT_SUCCESS(status)) {
      return status;
    }

    auto connection = get_connection(connection_object);
    connection->state = ConnectionContext::ConnectionState::CONNECTING;

    //
    // Get the BRB from request context and initialize it as
    // BRB_L2CA_OPEN_CHANNEL BRB
    //
    auto brb = (struct _BRB_L2CA_OPEN_ENHANCED_CHANNEL *)get_request_context(request);

    context->header.profile_drv_interface.BthReuseBrb(
      (BRB*)brb,
      BRB_L2CA_OPEN_ENHANCED_CHANNEL
    );
    
    brb->Hdr.ClientContext[0] = connection;
    brb->BtAddress = context->server_bth_address;
    brb->Psm = get_file_context(file_object)->server_psm;

    brb->ChannelFlags = CF_ROLE_EITHER;

    brb->ConfigOut.Flags = CFG_ENHANCED;
    //
    // Open an ERTM channel if the local host supports it
    //
    auto mode_config_flag = CM_BASIC;
    if (context->header.local_features.Mask & BTH_HOST_FEATURE_ENHANCED_RETRANSMISSION_MODE) {
      mode_config_flag |= CM_RETRANSMISSION_AND_FLOW;
      brb->ConfigOut.ModeConfig.Flags = mode_config_flag;

      //
      // Mode is specified using Flags above and this should be 0.
      //
      brb->ConfigOut.ModeConfig.RetransmissionAndFlow.Mode = 0;
      brb->ConfigOut.ModeConfig.RetransmissionAndFlow.MaxTransmit = L2CAP_RAF_DEFAULT_MAXTRANSMIT;
      brb->ConfigOut.ModeConfig.RetransmissionAndFlow.MaxPDUSize = L2CAP_RAF_DEFAULT_MAX_PDU_SIZE;
      brb->ConfigOut.ModeConfig.RetransmissionAndFlow.TxWindowSize = L2CAP_RAF_DEFAULT_TX_WINDOW_SIZE;
    }

    brb->ConfigOut.Flags |= CFG_MTU;
    brb->ConfigOut.Mtu.Max = L2CAP_DEFAULT_MTU;
    brb->ConfigOut.Mtu.Min = L2CAP_MIN_MTU;
    brb->ConfigOut.Mtu.Preferred = L2CAP_DEFAULT_MTU;

    brb->ConfigIn.Flags = CFG_MTU;
    brb->ConfigIn.Mtu.Max = brb->ConfigOut.Mtu.Max;
    brb->ConfigIn.Mtu.Min = brb->ConfigOut.Mtu.Min;
    brb->ConfigIn.Mtu.Preferred = brb->ConfigOut.Mtu.Max;

    //
    // Get notificaiton about remote disconnect 
    //
    brb->CallbackFlags = CALLBACK_DISCONNECT;                                                   

    brb->Callback = &indication_callback;
    brb->CallbackContext = connection;
    brb->ReferenceObject = (PVOID) WdfDeviceWdmGetDeviceObject(context->header.device);
    brb->IncomingQueueDepth = 50;

    status = send_brb_async(
      context->header.io_target,
      request,
      (BRB*) brb,
      sizeof(*brb),
      remote_connect_completion,
      brb    //Context
    );
    if (not NT_SUCCESS(status)) {
      connection->state = ConnectionContext::ConnectionState::CONNECTFAILED;
      return status;
    }

    return STATUS_SUCCESS;
  }

}
