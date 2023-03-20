#include "Driver.h"


NTSTATUS
NTAPI
SrkAsyncContextAllocate(
  OUT PSRK_ASYNC_CONTEXT AsyncContext
  )
{
  //
  // Initialize the completion event.
  //

  KeInitializeEvent(
    &AsyncContext->CompletionEvent,
    SynchronizationEvent,
    FALSE
    );

  //
  // Initialize the IRP.
  //

  AsyncContext->Irp = IoAllocateIrp(1, FALSE);

  if (AsyncContext->Irp == NULL)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  //
  // SrkAsyncContextCompletionRoutine will set
  // the CompletionEvent.
  //

  IoSetCompletionRoutine(
    AsyncContext->Irp,
    &SrkAsyncContextCompletionRoutine,
    &AsyncContext->CompletionEvent,
    TRUE,
    TRUE,
    TRUE
    );

  return STATUS_SUCCESS;
}

VOID
NTAPI
SrkAsyncContextFree(
  IN PSRK_ASYNC_CONTEXT AsyncContext
  )
{
  //
  // Free the IRP.
  //

  IoFreeIrp(AsyncContext->Irp);
}

VOID
NTAPI
SrkAsyncContextReset(
  IN PSRK_ASYNC_CONTEXT AsyncContext
  )
{
  //
  // If the WSK application allocated the IRP, or is reusing an IRP
  // that it previously allocated, then it must set an IoCompletion
  // routine for the IRP before calling a WSK function.  In this
  // situation, the WSK application must specify TRUE for the
  // InvokeOnSuccess, InvokeOnError, and InvokeOnCancel parameters that
  // are passed to the IoSetCompletionRoutine function to ensure that
  // the IoCompletion routine is always called. Furthermore, the IoCompletion
  // routine that is set for the IRP must always return
  // STATUS_MORE_PROCESSING_REQUIRED to terminate the completion processing
  // of the IRP.  If the WSK application is done using the IRP after the
  // IoCompletion routine has been called, then it should call the IoFreeIrp
  // function to free the IRP before returning from the IoCompletion routine.
  // If the WSK application does not free the IRP then it can reuse the IRP
  // for a call to another WSK function.
  //
  // (ref: https://docs.microsoft.com/en-us/windows-hardware/drivers/network/using-irps-with-winsock-kernel-functions)
  //

  //
  // Reset the completion event.
  //

  KeResetEvent(&AsyncContext->CompletionEvent);

  //
  // Reuse the IRP.
  //

  IoReuseIrp(AsyncContext->Irp, STATUS_UNSUCCESSFUL);

  IoSetCompletionRoutine(
    AsyncContext->Irp,
    &SrkAsyncContextCompletionRoutine,
    &AsyncContext->CompletionEvent,
    TRUE,
    TRUE,
    TRUE
    );
}

NTSTATUS
NTAPI
SrkAsyncContextCompletionRoutine(
  IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP Irp,
    IN PKEVENT CompletionEvent
  )
{
  UNREFERENCED_PARAMETER(DeviceObject);
  UNREFERENCED_PARAMETER(Irp);

  KeSetEvent(CompletionEvent, IO_NO_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
SrkAsyncContextWaitForCompletion(
    IN PSRK_ASYNC_CONTEXT AsyncContext,
    [IN,OUT] PNTSTATUS Status
  )
{
  if (*Status == STATUS_PENDING)
  {
    KeWaitForSingleObject(
      &AsyncContext->CompletionEvent,
      Executive,
      KernelMode,
      FALSE,
      NULL
      );

    *Status = AsyncContext->Irp->IoStatus.Status;
  }

  return *Status;
}