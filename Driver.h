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


#define POOL_TAG 'eRyS'
#define MAX_URL_LENGTH 255
#define MAX_PDB_FILENAME_SIZE 255

#define DbgPrintAndGotoCleanup(Msg, Status)	if (!NT_SUCCESS(Status)){ \
                                                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : %s, Status %d", __func__, Msg, Status); \
                                                goto Cleanup;} \
                                            


EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD SymResKEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP SymResKEvtDriverContextCleanup;
EXTERN_C_END

//
//  Functions
//

NTSTATUS WskInit();
NTSTATUS WskInitNpi();
BOOLEAN GetPdbUrl(OUT PUNICODE_STRING PdbUrl, IN PUNICODE_STRING FileName);


