module;

#include <ntddk.h>
#include <wdf.h>
#include <bthdef.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthsdpddi.h>

module nsw2ble.device;

import log;
import result;

import nsw2ble.guids;
import nsw2ble.context;
import nsw2ble.connection;
import nsw2ble.client;
import nsw2ble.queue;

namespace nsw2ble {
  auto on_device_self_managed_io_init(WDFDEVICE device) -> NTSTATUS;
  auto on_device_file_create(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file_object) -> void;
  auto on_device_file_close(WDFFILEOBJECT file_object) -> void;

/*++
Routine Description:

    BthEchoCliEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device. All the software resources
    should be allocated in this callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
  auto device_add(WDFDRIVER driver, WDFDEVICE_INIT* device_init) -> NTSTATUS {
    WDF_PNPPOWER_EVENT_CALLBACKS       pnp_power_callbacks;
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnp_power_callbacks);
    pnp_power_callbacks.EvtDeviceSelfManagedIoInit = on_device_self_managed_io_init;
    WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &pnp_power_callbacks);

    WDF_FILEOBJECT_CONFIG       fileobject_config;
    WDF_FILEOBJECT_CONFIG_INIT(
      &fileobject_config,
      on_device_file_create,
      on_device_file_close,
      WDF_NO_EVENT_CALLBACK
    );

    // Inform framework to create context area in every fileobject
    // so that we can track information per open handle by the
    // application.

    WDF_OBJECT_ATTRIBUTES                    file_attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&file_attributes, FileContext);
    WdfDeviceInitSetFileObjectConfig(device_init, &fileobject_config, &file_attributes);
    // Inform framework to create context area in every request object.
    //
    // We make BRB as the context since we need BRB for all the requests
    // we handle (Create, Read, Write).

    WDF_OBJECT_ATTRIBUTES                    request_attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&request_attributes, RequestContext);
    WdfDeviceInitSetRequestAttributes(device_init, &request_attributes);

    WDF_OBJECT_ATTRIBUTES                    device_attributes;
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&device_attributes, DeviceContext);

    WDFDEVICE device;
    auto status = WdfDeviceCreate(&device_init, &device_attributes, &device);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    auto context = get_device_context(device);
    status = context->init(device);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    status = query_bluetooth_interfaces(context);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    WDF_IO_QUEUE_CONFIG                     io_queue_config;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&io_queue_config, WdfIoQueueDispatchParallel);

    io_queue_config.EvtIoRead  = on_queue_io_read;
    io_queue_config.EvtIoWrite = on_queue_io_write;
    io_queue_config.EvtIoStop  = on_queue_io_stop;

    WDFQUEUE queue;
    status = WdfIoQueueCreate(device, &io_queue_config, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    status = WdfDeviceCreateDeviceInterface(device, &DEVICE_INTERFACE, nullptr);
    if (not NT_SUCCESS(status)) {
      return status;
    }

    return STATUS_SUCCESS;
  }

/*++

Description:

    This routine is called by the framework only once
    and hence we use it for our one time initialization.

    Bth addresses for both our local device and server device
    do not change, hence we retrieve them here.

    Features that the local device supports also do not change,
    so checking for local L2cap support is done here and saved in the
    device context header

    Please note that retrieveing server bth address does not
    require presence of the server. It is remembered from
    the installation time when the client gets installed for a
    specific server.

Arguments:

    Device - Framework device object

Return Value:

    NTSTATUS Status code.

--*/
  auto on_device_self_managed_io_init(WDFDEVICE device) -> NTSTATUS {
    auto context = get_device_context(device);

    auto status = context->header.retrieve_local_info();
    if (not NT_SUCCESS(status)) {
        return status;
    }

    status = retrieve_server_bth_address(context);
    if (not NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
  }

/*++
Description:

    This routine is invoked by remote_connect_completion
    function in client.cppm when opening a remote
    connection is completed.

    In this routine we set the file context to the connection passed in.

Arguments:

    FileObject - File object whose open resulted in open connection
    Connection - Our data strucutre to track open connection

Return Value:

    NTSTATUS Status code.
--*/
  auto on_connection_state_connected(WDFFILEOBJECT file_object, ConnectionContext* connection) -> NTSTATUS {
    get_file_context(file_object)->connection = connection;
    return STATUS_SUCCESS;
  }
  
/*++
Description:

    This routine is invoked by Framework when an application opens a handle
    to our device.

    In response we open a remote connection to the server.

Arguments:

    Device - Framework device object
    Request - Create request
    FileObject - File object corresponding to Create
--*/
  auto on_device_file_create(WDFDEVICE device, WDFREQUEST request, WDFFILEOBJECT file_object) -> void {
    auto context = get_device_context(device);
    //
    // We need to obtained PSM from the server
    // For this, first we retrieve the SDP record
    //
    BTH_SDP_STREAM_RESPONSE* server_sdp_record;
    auto status = retrieve_server_sdp_record(context, &server_sdp_record);
    //
    // If we failed we complete the request here
    // If it succeeds remote_connect_completion will complete the request
    //
    if (not NT_SUCCESS(status)) {
      if (server_sdp_record != NULL) {
        ExFreePoolWithTag(server_sdp_record, POOLTAG_NSW2BLE);
      }
      WdfRequestComplete(request, status);
      return;
    }

    BTHDDI_SDP_PARSE_INTERFACE sdp_parse_interface;
    status = WdfFdoQueryForInterface(
      device,
      &GUID_BTHDDI_SDP_PARSE_INTERFACE, 
      (PINTERFACE) (&sdp_parse_interface),
      sizeof(sdp_parse_interface), 
      BTHDDI_SDP_PARSE_INTERFACE_VERSION_FOR_QI, 
      NULL
    );
    if (not NT_SUCCESS(status)) {
      ExFreePoolWithTag(server_sdp_record, POOLTAG_NSW2BLE);
      WdfRequestComplete(request, status);
      return;
    }

    //
    // Once we retrieved the server SDP record, we retrieve
    // PSM from this record.
    //
    // We store this Psm in our file context and use it in open
    // and close channel BRBs.
    //
    status = retrieve_psm_from_sdp_record(
      &sdp_parse_interface,
      server_sdp_record,
      &get_file_context(file_object)->server_psm
    );
    if (NT_SUCCESS(status)) {
      status = open_remote_connection(context, file_object, request);
    }

    if (not NT_SUCCESS(status)) {
      ExFreePoolWithTag(server_sdp_record, POOLTAG_NSW2BLE);
      WdfRequestComplete(request, status);
      return;
    }

    ExFreePoolWithTag(server_sdp_record, POOLTAG_NSW2BLE);
  }

/*++
Description:

    This routine is invoked by Framework when I/O manager sends Close
    IRP for a file.

    In response we close the remote connection to the server.

Arguments:

    FileObject - File object corresponding to Close

--*/
  auto on_device_file_close(WDFFILEOBJECT file_object) -> void {
    // auto context      = get_device_context(WdfFileObjectGetDevice(file_object));
    // auto file_context = get_file_context(file_object);
    get_file_context(file_object)->connection->remote_disconnect_sync();
  }

} // namespace nsw2ble
