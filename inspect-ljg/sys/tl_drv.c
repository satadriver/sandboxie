
#include <ntifs.h>
#include <ntddk.h>

#include <wdf.h>

#pragma warning(push)
#pragma warning(disable:4201) /* Unnamed struct/union. */

#include <fwpsk.h>
#pragma warning(pop)

#include <fwpmk.h>
#include "inspect.h"
#include "worker_thread.h"
#include "packet_pool.h"
#include "dnscache.h"
#include "logfile.h"

#define INITGUID
#include <guiddef.h>

#include "mypacket.h"

#include "tl_drv.h"
#include "list.h"
#include "configfile.h"


static HANDLE hEngine;
static UINT32 layerStreamV4, layerStreamV6;
static UINT32 layerDatagramV4, layerDatagramV6;
static UINT32 layerAleClosureV4, layerAleClosureV6;


static callout_t callouts[] = {
  {
    &FWPM_LAYER_STREAM_V4,
    &TL_LAYER_STREAM_V4,
    StreamNotify,
    StreamClassify,
    L"StreamLayerV4",
    L"Intercepts the first ipv4 outbound packet with payload of each connection.",
    &layerStreamV4
  },
  {
    &FWPM_LAYER_STREAM_V6,
    &TL_LAYER_STREAM_V6,
    StreamNotify,
    StreamClassify,
    L"StreamLayerV6",
    L"Intercepts the first ipv6 outbound packet with payload of each connection.",
    &layerStreamV6
  },
  {
    &FWPM_LAYER_DATAGRAM_DATA_V4,
    &TL_LAYER_DATAGRAM_V4,
    DatagramNotify,
    DatagramClassify,
    L"DatagramLayerV4",
    L"Intercepts inbound ipv4 UDP data.",
    &layerDatagramV4
  },
  {
    &FWPM_LAYER_DATAGRAM_DATA_V6,
    &TL_LAYER_DATAGRAM_V6,
    DatagramNotify,
    DatagramClassify,
    L"DatagramLayerV6",
    L"Intercepts inbound ipv6 UDP data.",
    &layerDatagramV6
  },
  {
      //This filtering layer allows for tracking de-activation of connected TCP flow or UDP sockets
    &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
    &TL_LAYER_ALE_EP_CLOSURE_V4,
    AleClosureNotify,
    AleClosureClassify,
    L"AleLayerEndpointClosureV4",
    L"Intercepts ipv4 connection close",
    &layerAleClosureV4
  },
  {
    &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
    &TL_LAYER_ALE_EP_CLOSURE_V6,
    AleClosureNotify,
    AleClosureClassify,
    L"AleLayerEndpointClosureV6",
    L"Intercepts ipv6 connection close",
    &layerAleClosureV6
  }
};


DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

EVT_WDF_DEVICE_FILE_CREATE EvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE EvtFileClose;

EVT_WDF_DRIVER_DEVICE_ADD EvtWdfDriverDeviceAdd;
#pragma alloc_text(PAGE, EvtWdfDriverDeviceAdd)

static NTSTATUS AddFilter(_In_ const wchar_t* filterName,
                          _In_ const wchar_t* filterDesc,
                          _In_ UINT64 context,
                          _In_ const GUID* layerKey,
                          _In_ const GUID* calloutKey)
{
    FWPM_FILTER filter = {0};

    filter.displayData.name = (wchar_t*) filterName;
    filter.displayData.description = (wchar_t*) filterDesc;

    filter.rawContext = context;

    filter.layerKey = *layerKey;
    filter.action.calloutKey = *calloutKey;

    //filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;

	FWPM_FILTER_CONDITION FilterCondition[1] = { 0 };

	filter.filterCondition = FilterCondition;
	filter.numFilterConditions = ARRAYSIZE(FilterCondition);

//     FilterCondition[1].fieldKey = FWPM_CONDITION_ICMP_TYPE;
//     FilterCondition[1].matchType = FWP_MATCH_EQUAL;
//     FilterCondition[1].conditionValue.type = FWP_UINT16;
//     FilterCondition[1].conditionValue.uint16 = 3;

    if (memcmp(layerKey , &FWPM_LAYER_STREAM_V4,sizeof(GUID)) == 0 || 
        memcmp(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V4, sizeof(GUID)) == 0||
        memcmp(layerKey, &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4, sizeof(GUID)) == 0 )
    {
		FilterCondition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		FilterCondition[0].matchType = FWP_MATCH_EQUAL;
        FilterCondition[0].conditionValue.type = FWP_V4_ADDR_MASK;
        FWP_V4_ADDR_AND_MASK AddrAndMask = { 0 };
        FilterCondition[0].conditionValue.v4AddrMask = &AddrAndMask;

		filter.filterCondition = FilterCondition;
		filter.numFilterConditions = ARRAYSIZE(FilterCondition);

    }
	else if (memcmp(layerKey, &FWPM_LAYER_STREAM_V6, sizeof(GUID)) == 0 || 
        memcmp(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V6, sizeof(GUID)) == 0 ||
		memcmp(layerKey, &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6, sizeof(GUID)) == 0)
    {
		FilterCondition[0].fieldKey = FWPM_CONDITION_IP_REMOTE_ADDRESS;
		FilterCondition[0].matchType = FWP_MATCH_EQUAL;
        FilterCondition[0].conditionValue.type = FWP_V6_ADDR_MASK;
        FWP_V6_ADDR_AND_MASK AddrAndMask = { 0 };
        FilterCondition[0].conditionValue.v6AddrMask = &AddrAndMask;

		filter.filterCondition = FilterCondition;
		filter.numFilterConditions = ARRAYSIZE(FilterCondition);

    }
//     else if (memcmp(layerKey, &FWPM_LAYER_DATAGRAM_DATA_V6, sizeof(GUID)) == 0 )
//     {
// 		static const UINT16 PORTS[] = {80, 443, 53,8080,8000};
//         FWPM_FILTER_CONDITION filterConditions[ARRAYSIZE(PORTS)];
// 		for (size_t i = 0; i < ARRAYSIZE(PORTS); i++) {
// 		    filterConditions[i].fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
// 		    filterConditions[i].matchType = FWP_MATCH_EQUAL;
// 		    filterConditions[i].conditionValue.type = FWP_UINT16;
// 		    filterConditions[i].conditionValue.uint16 = PORTS[i];
//         }
//        filter.filterCondition = filterConditions;
//        filter.numFilterConditions = ARRAYSIZE(PORTS);
//     }
    else {
        return -1;
    }

    filter.subLayerKey = TL_INSPECT_SUBLAYER;
    filter.weight.type = FWP_EMPTY; /* Auto-weight. */

    return FwpmFilterAdd(hEngine, &filter, NULL, NULL);
}



NTSTATUS icmpfilter() {
    NTSTATUS status;
    FWPM_FILTER filter = { 0 };

	filter.displayData.name = (wchar_t*)L"icmp";
	filter.displayData.description = (wchar_t*)L"icmp";

	filter.rawContext = 0;

    FWPM_FILTER_CONDITION FilterCondition[1] = { 0 };

	FilterCondition[0].fieldKey = FWPM_CONDITION_ICMP_TYPE;
	FilterCondition[0].matchType = FWP_MATCH_EQUAL;
	FilterCondition[0].conditionValue.type = FWP_UINT16;
	FilterCondition[0].conditionValue.uint16 = 3;

	filter.action.type = FWP_ACTION_BLOCK;

	filter.filterCondition = FilterCondition;
	filter.numFilterConditions = ARRAYSIZE(FilterCondition);

	// Add the IPv4 filter.
	filter.layerKey = FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4;
	status = FwpmFilterAdd0(hEngine, &filter, NULL, NULL);

	// Add the IPv6 filter.
	filter.layerKey = FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6;
	status = FwpmFilterAdd0(hEngine, &filter, NULL, NULL);

	return status;
}

static NTSTATUS RegisterCallout(_In_ const GUID* layerKey,
                                _In_ const GUID* calloutKey,
                                _Inout_ void* deviceObject,
                                _In_ FWPS_CALLOUT_NOTIFY_FN notifyFn,
                                _In_ FWPS_CALLOUT_CLASSIFY_FN classifyFn,
                                _In_ wchar_t* calloutName,
                                _In_ wchar_t* calloutDescription,
                                _Out_ UINT32* calloutId)
{
    FWPS_CALLOUT sCallout = {0};
    FWPM_CALLOUT mCallout = {0};
    NTSTATUS status;

    /* Register callout with the filter engine. */
    sCallout.calloutKey = *calloutKey;
    sCallout.notifyFn = notifyFn;
    sCallout.classifyFn = classifyFn;

    status = FwpsCalloutRegister(deviceObject, &sCallout, calloutId);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Add callout. */
    mCallout.applicableLayer = *layerKey;
    mCallout.calloutKey = *calloutKey;
    mCallout.displayData.name = calloutName;
    mCallout.displayData.description = calloutDescription;

    status = FwpmCalloutAdd(hEngine, &mCallout, NULL, NULL);
    if (!NT_SUCCESS(status)) {
		FwpsCalloutUnregisterById(*calloutId);
		*calloutId = 0;

		return status;
    }

    /* Add filter. */
    status = AddFilter(L"HTTP/HTTPS/DNS",
                        L"Filter HTTP/HTTPS/DNS",
                        0,
                        layerKey,
                        calloutKey);

    if (!NT_SUCCESS(status)) {
		FwpsCalloutUnregisterById(*calloutId);
		*calloutId = 0;

		return status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS RegisterCallouts(_Inout_ void* deviceObject)
{
    FWPM_SESSION session = {0};
    FWPM_SUBLAYER TLInspectSubLayer;
    NTSTATUS status;

    /* If session.flags is set to FWPM_SESSION_FLAG_DYNAMIC, any WFP objects
    * added during the session are automatically deleted when the session ends.
    */
    session.flags = FWPM_SESSION_FLAG_DYNAMIC;

    /* Open session to the filter engine. */
    status = FwpmEngineOpen(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &hEngine);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Begin transaction with the current session. */
    status = FwpmTransactionBegin(hEngine, 0);
    if (!NT_SUCCESS(status)) {
        FwpmEngineClose(hEngine);
        hEngine = NULL;

        return status;
    }

    /* Add sublayer to the system. */
    RtlZeroMemory(&TLInspectSubLayer, sizeof(FWPM_SUBLAYER));

    TLInspectSubLayer.subLayerKey = TL_INSPECT_SUBLAYER;
    TLInspectSubLayer.displayData.name = L"Transport Inspect Sub-Layer";
    TLInspectSubLayer.displayData.description =
    L"Sub-Layer for use by Transport Inspect callouts";

    TLInspectSubLayer.flags = 0;
    TLInspectSubLayer.weight = 0;

    status = FwpmSubLayerAdd(hEngine, &TLInspectSubLayer, NULL);
    if (!NT_SUCCESS(status)) {
		FwpmTransactionAbort(hEngine);
		_Analysis_assume_lock_not_held_(hEngine);

		FwpmEngineClose(hEngine);
		hEngine = NULL;

		return status;
    }

    /* Register callouts. */
    for (size_t i = 0; i < ARRAYSIZE(callouts); i++) {
		status = RegisterCallout(callouts[i].layerKey,
			callouts[i].calloutKey,
			deviceObject,
			callouts[i].notifyFn,
			callouts[i].classifyFn,
			callouts[i].name,
			callouts[i].description,
			callouts[i].calloutId);

		if (!NT_SUCCESS(status)) {
			FwpmTransactionAbort(hEngine);
			_Analysis_assume_lock_not_held_(hEngine);

			FwpmEngineClose(hEngine);
			hEngine = NULL;

			return status;
		}
    }

    icmpfilter();

    status = FwpmTransactionCommit(hEngine);

    if (!NT_SUCCESS(status)) {
		FwpmTransactionAbort(hEngine);
		_Analysis_assume_lock_not_held_(hEngine);

		FwpmEngineClose(hEngine);
		hEngine = NULL;

		return status;
    }

    return STATUS_SUCCESS;
}

static void UnregisterCallouts()
{
	FwpsCalloutUnregisterById(layerStreamV4);
	FwpsCalloutUnregisterById(layerStreamV6);
	FwpsCalloutUnregisterById(layerDatagramV4);
	FwpsCalloutUnregisterById(layerDatagramV6);
	FwpsCalloutUnregisterById(layerAleClosureV4);
	FwpsCalloutUnregisterById(layerAleClosureV6);

	FwpmEngineClose(hEngine);
	hEngine = NULL;
}

_Function_class_(EVT_WDF_DRIVER_UNLOAD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
void EvtDriverUnload(_In_ WDFDRIVER driverObject)
{
	UNREFERENCED_PARAMETER(driverObject);

	UnregisterCallouts();
// 	StopWorkerThread();
// 	FreeWorkerThread();
// 	CloseLogFile();
// 	FreeDnsCache();
// 	FreePacketPool();

// 	IoDeleteDevice(g_ctrldev);
// 	UNICODE_STRING symbol;
// 	RtlInitUnicodeString(&symbol, WFP_DEVICE_SYMBOL);
// 	IoDeleteSymbolicLink(&symbol);
}


VOID EvtDeviceFileCreate(__in WDFDEVICE Device, __in WDFREQUEST Request, __in WDFFILEOBJECT FileObject)
{
	KdPrint(("EvtDeviceFileCreate"));

	WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID EvtFileClose(__in  WDFFILEOBJECT FileObject)
{
	KdPrint(("EvtFileClose"));
}

NTSTATUS EvtWdfdeviceWdmIrpDispatch(
	WDFDEVICE Device,
	UCHAR MajorFunction,
	UCHAR MinorFunction,
	ULONG Code,
	WDFCONTEXT DriverContext,
	PIRP Irp,
	WDFCONTEXT DispatchContext
)
{
    return 0;
}

//EVT_WDFDEVICE_WDM_IRP_DISPATCH EvtWdfdeviceWdmIrpDispatch;



 

NTSTATUS WfpCtrlIRPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    //__debugbreak();

	PIO_STACK_LOCATION	IrpStack = 0;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	if (Irp == NULL)
	{
		return nStatus;
	}

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	if (IrpStack == NULL)
	{
		return nStatus;
	}

	if (IrpStack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
	{
		Irp->IoStatus.Information = 0;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
	}

	UNREFERENCED_PARAMETER(DeviceObject);

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_WFP_SDWS_ADD_DNS:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG uInLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            nStatus = addDnsRule(pSystemBuffer, uInLen);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_URL:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG uInLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
            nStatus = addDnsRule(pSystemBuffer, uInLen);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_SERVER:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            DWORD ipv4 = *(DWORD*)pSystemBuffer;
            setDnsTarget(ipv4);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_PORT:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            addIPPortRule(pSystemBuffer, IOCTL_WFP_SDWS_ADD_PORT);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_IPV4:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            addIPPortRule(pSystemBuffer, IOCTL_WFP_SDWS_ADD_IPV4);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_IPV6:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            addIPPortRule(pSystemBuffer, IOCTL_WFP_SDWS_ADD_IPV6);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_PROCESS:
        {
            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            addProcessRule(pSystemBuffer, IOCTL_WFP_SDWS_ADD_PROCESS);
            nStatus = STATUS_SUCCESS;
            break;
        }
        case IOCTL_WFP_SDWS_ADD_INTRANET:
        {
            //__debugbreak();

            PVOID pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
            BOOLEAN enable = *(BOOLEAN*)pSystemBuffer;
            setIntranet(enable);
            nStatus = STATUS_SUCCESS;
            break;
        }

        case IOCTL_WFP_SDWS_CLEAR_DNS: 
        {
            clearDnsList();
            break;
        }
        case IOCTL_WFP_SDWS_CLEAR_IPPORT:
        {
            clearIPPortList();
        }
        case IOCTL_WFP_SDWS_CLEAR_PROCESS:
        {
            clearProcessList();
        }
	    default:
	    {
		    nStatus = STATUS_UNSUCCESSFUL;
	    }
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = nStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return nStatus;
}



NTSTATUS EvtWdfDriverDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {

    return STATUS_SUCCESS;
}

static NTSTATUS InitDriverObjects(_Inout_ DRIVER_OBJECT* driverObject, _In_ const UNICODE_STRING* registryPath,_Out_ WDFDRIVER* pDriver,_Out_ WDFDEVICE* pDevice)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_FILEOBJECT_CONFIG f_cfg;

    //KdBreakPoint();

    /* Initialize 'config'. */
    WDF_DRIVER_CONFIG_INIT(&config, 0);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = EvtDriverUnload;

    /* Create framework driver object. */
    status = WdfDriverCreate(driverObject, registryPath,WDF_NO_OBJECT_ATTRIBUTES, &config, pDriver);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    /* Allocate a WDFDEVICE_INIT structure. */
    PWDFDEVICE_INIT pInit =WdfControlDeviceInitAllocate(*pDriver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);
    if (!pInit) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UNICODE_STRING devname;
	RtlInitUnicodeString(&devname, WFP_DEVICE_NAME);
    
	status = WdfDeviceInitAssignName(pInit, &devname);  //status = WdfDeviceInitUpdateName(pInit, &devname);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("WdfDeviceInitAssignName %Z error", &devname);
        return status;
    }

    WdfDeviceInitSetDeviceType(pInit, FILE_DEVICE_NETWORK);
    WdfDeviceInitSetCharacteristics(pInit, FILE_DEVICE_SECURE_OPEN, FALSE);
    //WdfDeviceInitSetCharacteristics(pInit, FILE_AUTOGENERATED_DEVICE_NAME, TRUE);

    WDF_OBJECT_ATTRIBUTES object_attribs;
	WDF_OBJECT_ATTRIBUTES_INIT(&object_attribs);

    WDF_FILEOBJECT_CONFIG_INIT(&f_cfg, EvtDeviceFileCreate, EvtFileClose, NULL);
    WdfDeviceInitSetFileObjectConfig(pInit, &f_cfg, WDF_NO_OBJECT_ATTRIBUTES);

    /* Create framework device object. */
    status = WdfDeviceCreate(&pInit, &object_attribs, pDevice);
    if (!NT_SUCCESS(status)) {
		WdfDeviceInitFree(pInit);
		return status;
    }

// MessageId: STATUS_OBJECT_NAME_COLLISION
// MessageText:
// Object Name already exists.
// #define STATUS_OBJECT_NAME_COLLISION     ((NTSTATUS)0xC0000035L)

	//WDM drivers do not name device objectsand therefore should not use this routine.Instead, a WDM driver should call IoRegisterDeviceInterface to set up a symbolic link.
	//A named device object has a name of the form \Device\DeviceName. This is known as the NT device name of the device object.
	// WDF drivers do not need to name their PnP device in order to create a symbolic link using WdfDeviceCreateSymbolicLink.

    //__debugbreak();
	UNICODE_STRING symbol;
	RtlInitUnicodeString(&symbol, WFP_DEVICE_SYMBOL );
	status = WdfDeviceCreateSymbolicLink(*pDevice, &symbol);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("WdfDeviceCreateSymbolicLink %Z error\r\n", &symbol);
	}

    DRIVER_OBJECT * wfpdriver = WdfDriverWdmGetDriverObject(*pDriver);
    wfpdriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WfpCtrlIRPDispatch;

    //status = WdfDeviceConfigureWdmIrpDispatchCallback(*pDevice, *pDriver, IRP_MJ_DEVICE_CONTROL, WfpCtrlIRPDispatch, 0);

    /* Inform framework that we have finished initializing the device object. */
    WdfControlFinishInitializing(*pDevice);

    return STATUS_SUCCESS;
}



NTSTATUS DriverEntry(DRIVER_OBJECT* driverObject, UNICODE_STRING* registryPath)
{
	WDFDRIVER driver;
	WDFDEVICE device;
	DEVICE_OBJECT* wdmDevice;
	WDFKEY parametersKey;
    NTSTATUS status;

    //KdBreakPoint();
    //DbgBreakPoint();

    initAttackList();

    /* Request NX Non-Paged Pool when available. */
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    /* Initialize packet pool. */
//     if (!InitPacketPool(MAX_PACKETS, MAX_PACKET_SIZE)) {
// 		DbgPrint("Error initializing packet pool.");
// 		return STATUS_NO_MEMORY;
//     }

    /* Initialize DNS cache. */
//     if (!InitDnsCache(NUMBER_BUCKETS, MAX_DNS_ENTRIES)) {
// 		DbgPrint("Error initializing DNS cache.");
// 
// 		FreePacketPool();
// 		return STATUS_NO_MEMORY;
//     }

    /* Open log file. */
//   status = OpenLogFile(LOG_BUFFER_SIZE);
//   if (!NT_SUCCESS(status)) {
//     DbgPrint("Error opening log file.");
// 
//     FreeDnsCache();
//     FreePacketPool();
// 
//     return status;
//   }

    /* Initialize worker thread. */
//     if (!InitWorkerThread(MAX_PACKETS)) {
//         DbgPrint("Error initializing worker thread.");
// 
//         CloseLogFile();
//         FreeDnsCache();
//         FreePacketPool();
// 
//         return STATUS_NO_MEMORY;
//     }

    /* Start worker thread. */
//     status = StartWorkerThread();
//     if (!NT_SUCCESS(status)) {
// 		DbgPrint("Error starting worker thread.");
// 
// 		FreeWorkerThread();
// 		CloseLogFile();
// 		FreeDnsCache();
// 		FreePacketPool();
// 
// 		return status;
//     }

    /* Initialize driver objects. */
    status = InitDriverObjects(driverObject, registryPath, &driver, &device);
    if (!NT_SUCCESS(status)) {
//         StopWorkerThread();
//         FreeWorkerThread();
//         CloseLogFile();
//         FreeDnsCache();
//         FreePacketPool();

        return status;
    }

    /* Open 'Parameters' registry key and retrieve a handle to the registry-key
    * object.
    */
    status = WdfDriverOpenParametersRegistryKey(driver,KEY_READ,WDF_NO_OBJECT_ATTRIBUTES, &parametersKey);
    if (!NT_SUCCESS(status)) {
// 		StopWorkerThread();
// 		FreeWorkerThread();
// 		CloseLogFile();
// 		FreeDnsCache();
// 		FreePacketPool();

		return status;
    }

    
    /* Get the Windows Driver Model (WDM) device object. */
    wdmDevice = WdfDeviceWdmGetDeviceObject(device);

    WCHAR wdmname[1024];
    ULONG outlen = 0;
    status = ObQueryNameString(wdmDevice, (POBJECT_NAME_INFORMATION)wdmname, sizeof(wdmname),&outlen);
    PUNICODE_STRING devicename = (PUNICODE_STRING)wdmname;

// 	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
// 	{
// 		driverObject->MajorFunction[i] = WfpCtrlIRPDispatch;
// 	}
// 	//driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WfpCtrlIRPDispatch;
//      __debugbreak();
//     GUID mydevguid = { 0x01234567,0x89ab,0xcdef,0,1,2,3,4,5,6,7 };
//     UNICODE_STRING devname;
//     RtlInitUnicodeString(&devname, L"\\device\\inspectnt");
//     UNICODE_STRING sddl;
//     RtlInitUnicodeString(&sddl, L"D:P(A;;GA;;;WD)");
//     status = IoCreateDeviceSecure(driverObject, 0, &devname, FILE_DEVICE_UNKNOWN, 0, 0,&sddl, & mydevguid, &g_ctrldev);
//     //status = IoCreateDevice(driverObject, 0, &devname, FILE_DEVICE_UNKNOWN, OBJ_CASE_INSENSITIVE, 0,  &g_pctrldev);
//     if (NT_SUCCESS(status))
//     {
// 		UNICODE_STRING symbol;
// 		RtlInitUnicodeString(&symbol, L"\\DosDevices\\inspectnt");
// 		status = IoCreateSymbolicLink(&symbol, &devname);
// 		if (NT_SUCCESS(status))
// 		{
// 			g_ctrldev->Flags |= DO_BUFFERED_IO;
// 			g_ctrldev->Flags |= DO_DIRECT_IO;
// 			g_ctrldev->Flags &= (~DO_DEVICE_INITIALIZING);		
// 		}
//         else {
//             DbgPrint("IoCreateSymbolicLink %Z error\r\n", &symbol);
//         }
//     }
//     else {
//         DbgPrint("IoCreateDevice error\r\n");
//     }


    /* Register callouts. */
    status = RegisterCallouts(wdmDevice);
    if (!NT_SUCCESS(status)) {
		UnregisterCallouts();
// 		StopWorkerThread();
// 		FreeWorkerThread();
// 		CloseLogFile();
// 		FreeDnsCache();
// 		FreePacketPool();

		return status;
    }

    //__debugbreak();
//     HANDLE thread_handle;
// 	status = PsCreateSystemThread(
// 		&thread_handle,
// 		(ACCESS_MASK)0L,
// 		NULL,
// 		NULL,
// 		NULL,
// 		(PKSTART_ROUTINE)mainloop,
// 		0);
// 
// 	if (NT_SUCCESS(status)) {
//         ZwClose(thread_handle);
// 	}

    return STATUS_SUCCESS;
}
