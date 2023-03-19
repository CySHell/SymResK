#pragma once

#include <ntddk.h>
#include <wdf.h>

//
//Structures
//

typedef struct _SRK_ASYNC_CONTEXT
{
	KEVENT CompletionEvent;
	PIRP Irp;
} SRK_ASYNC_CONTEXT, * PSRK_ASYNC_CONTEXT;

// Functions

NTSTATUS
NTAPI
SrkAsyncContextAllocate(
	OUT PSRK_ASYNC_CONTEXT AsyncContext
);

VOID
NTAPI
SrkAsyncContextFree(
	IN PSRK_ASYNC_CONTEXT AsyncContext
);

VOID
NTAPI
SrkAsyncContextReset(
	IN PSRK_ASYNC_CONTEXT AsyncContext
);

NTSTATUS
NTAPI
SrkAsyncContextCompletionRoutine(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP Irp,
	IN PKEVENT CompletionEvent
);

NTSTATUS
NTAPI
SrkAsyncContextWaitForCompletion(
	IN PSRK_ASYNC_CONTEXT AsyncContext,
	[IN,OUT] PNTSTATUS Status
);