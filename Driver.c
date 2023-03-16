/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"

//
// Global WSK Definitions
//

// WSK Client Dispatch table that denotes the WSK version
// that the WSK application wants to use and optionally a pointer
// to the WskClientEvent callback function
const WSK_CLIENT_DISPATCH WskAppDispatch = {
  MAKE_WSK_VERSION(1,0), // Use WSK version 1.0
  0,    // Reserved
  NULL  // WskClientEvent callback not required for WSK version 1.0
};

// WSK Registration object
WSK_REGISTRATION WskRegistration;

// NPI object received from the WSK Provider
BOOLEAN WskProviderNpiInitialized = FALSE;
WSK_PROVIDER_NPI wskProviderNpi;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SymResKEvtDeviceAdd)
#pragma alloc_text (PAGE, SymResKEvtDriverContextCleanup)
#endif

//
//  Symbol and File definitions, for PDB parsing
//
UNICODE_STRING g_FileName = RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\ntdll.dll");
UNICODE_STRING g_Symbol = RTL_CONSTANT_STRING(L"g_CoreUrlCallsDesc");



NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = SymResKEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           SymResKEvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WdfDriverCreate failed %d", status);
        goto Cleanup;
    }

    status = WskInit();

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WskInit failed %d", status);
        goto Cleanup;
    }

    //UNICODE_STRING PdbUrl = { 0 };
    //WCHAR PdbUrlBuffer[MAX_URL_LENGTH];
    //RtlInitEmptyUnicodeString(&PdbUrl, PdbUrlBuffer, MAX_URL_LENGTH);
    PUNICODE_STRING PdbUrl = (PUNICODE_STRING)ExAllocatePool2(PagedPool, sizeof(UNICODE_STRING), POOL_TAG);
    if (PdbUrl == NULL)
    {
        return status;
    }
    PdbUrl->Length = 0;
    PdbUrl->MaximumLength = MAX_URL_LENGTH;
    PdbUrl->Buffer = ExAllocatePool2(PagedPool, MAX_URL_LENGTH, POOL_TAG);
    
    if (!GetPdbUrl(PdbUrl, &g_FileName))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "GetPdbUrl failed");
        status = -1;
        goto Cleanup;
    }


    Cleanup:
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s Exit", __func__);
    return status;

}

NTSTATUS
SymResKEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s Entry", __func__);
    status = SymResKCreateDevice(DeviceInit);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s Exit", __func__);

    return status;
}

VOID
SymResKEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s Entry", __func__);

}
