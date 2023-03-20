/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <ntimage.h>
#include <Ntstrsafe.h>
#include "Wsk.h"

#include "device.h"
#include "queue.h"
#include "Utils.h"





#define POOL_TAG 'eRyS'

#define DbgPrintAndGotoCleanup(Msg, Status)	if (!NT_SUCCESS(Status)){ \
                                                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : %s, Status %d", __func__, Msg, Status); \
                                                goto Cleanup;} \
                              
//
// PDB File Attributes
//
#define MAX_URL_LENGTH 255
#define MAX_PDB_FILENAME_SIZE 255


//
//	Symbol Server Url format
//
#define SYMBOL_SERVER_NODE L"msdl.microsoft.com"
#define SYMBOL_SERVER_REMOTE_PORT RtlUshortByteSwap(443);	// Convert to network byte order
#define SYMBOL_SERVER_BASE L"https://msdl.microsoft.com/download/symbols/"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD SymResKEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP SymResKEvtDriverContextCleanup;

//
//  Functions
//

NTSTATUS WskInit();
NTSTATUS WskInitNpi();
VOID WskCleanup();
NTSTATUS WskCreateSocketAndConnect(
    PADDRINFOEXW AddrInfo,
    OUT PWSK_SOCKET * Socket
);
NTSTATUS WskSendReceiveData(PWSK_SOCKET Socket, PWSK_BUF SendBuf, PWSK_BUF RecvBuf);
NTSTATUS SrkGetAddrInfo(IN PUNICODE_STRING NodeName,
    IN PUNICODE_STRING ServiceName,
    IN PADDRINFOEXW Hints,
    OUT PADDRINFOEXW * Result);
BOOLEAN GetPdbUrl(OUT PUNICODE_STRING PdbUrl, IN PUNICODE_STRING FileName);


