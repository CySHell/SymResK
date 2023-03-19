#include "Driver.h"
//#include "wsk.h"


extern WSK_CLIENT_DISPATCH WskAppDispatch;
extern WSK_REGISTRATION WskRegistration;
extern WSK_PROVIDER_NPI wskProviderNpi;
extern BOOLEAN WskProviderNpiInitialized;

NTSTATUS WskInit()
{


    NTSTATUS status;
    WSK_CLIENT_NPI wskClientNpi;

    // Register the WSK application
    wskClientNpi.ClientContext = NULL;
    wskClientNpi.Dispatch = &WskAppDispatch;
    status = WskRegister(&wskClientNpi, &WskRegistration);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WskRegister failed %d", status);
        return status;
    }

    return WskInitNpi();
}

// WSK application routine that waits for WSK subsystem
// to become ready and captures the WSK Provider NPI
NTSTATUS
WskInitNpi()
{
    NTSTATUS status;

    // WskCaptureProviderNPI() must be called at PASSIVE_LEVEL IRQL
    PAGED_CODE()

    // Capture the WSK Provider NPI. If WSK subsystem is not ready yet,
    // wait until it becomes ready.
    status = WskCaptureProviderNPI(
        &WskRegistration, // must have been initialized with WskRegister
        WSK_INFINITE_WAIT,
        &wskProviderNpi
    );

    if (!NT_SUCCESS(status)) {
        // The WSK Provider NPI could not be captured.
        if (status == STATUS_NOINTERFACE) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WSK application's requested version is not supported");
        }
        else if (status == STATUS_DEVICE_NOT_READY) {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WskDeregister was invoked in another thread thereby causing WskCaptureProviderNPI to be canceled.");
        }
        else {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Some other unexpected failure has occurred");
        }
        return status;
    }

    WskProviderNpiInitialized = TRUE;
    return status;
}



// WSK application routine that releases the WSK Provider NPI
// and deregisters the WSK application
VOID WskCleanup()
{
	// Release the WSK Provider NPI
    if (WskProviderNpiInitialized) {
		WskReleaseProviderNPI(&wskProviderNpi);
		WskProviderNpiInitialized = FALSE;
	}
	// Deregister the WSK application
	WskDeregister(&WskRegistration);
}


NTSTATUS SrkGetAddrInfo(IN PUNICODE_STRING NodeName,
    IN PUNICODE_STRING ServiceName,
    IN PADDRINFOEXW Hints,
    OUT PADDRINFOEXW* Result)
{
    NTSTATUS Status;

    //
    // Allocate async context.
    //

    SRK_ASYNC_CONTEXT AsyncContext;
    Status = KspAsyncContextAllocate(&AsyncContext);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Call the WSK API.
    //

    Status = wskProviderNpi.Dispatch->WskGetAddressInfo(
        wskProviderNpi.Client,         // Client
        NodeName,                   // NodeName
        ServiceName,                // ServiceName
        0,                          // NameSpace
        NULL,                       // Provider
        Hints,                      // Hints
        Result,                     // Result
        NULL,                       // OwningProcess
        NULL,                       // OwningThread
        AsyncContext.Irp            // Irp
    );

    SrkAsyncContextWaitForCompletion(&AsyncContext, &Status);

    //
    // Free the async context.
    //

    SrkAsyncContextFree(&AsyncContext);

    return Status;
}



// WSK application routine that creates a WSK socket
NTSTATUS WskCreateSocketAndConnect(
    IN PADDRINFOEXW RemoteAddrInfo,
    OUT PWSK_SOCKET* Socket
)
{
    NTSTATUS Status;
    SOCKADDR_IN LocalAddress, RemoteAddr;

    RemoteAddr = *(struct sockaddr_in*)RemoteAddrInfo->ai_addr;
    RemoteAddr.sin_port = SYMBOL_SERVER_REMOTE_PORT;

    LocalAddress.sin_family = AF_INET;
    LocalAddress.sin_addr.s_addr = INADDR_ANY;

   //
   // Allocate async context.
   //

    SRK_ASYNC_CONTEXT AsyncContext;
    Status = KspAsyncContextAllocate(&AsyncContext);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Call the WSK API.
    //
    wskProviderNpi.Dispatch->WskSocketConnect(
		wskProviderNpi.Client,RemoteAddrInfo->ai_socktype,RemoteAddrInfo->ai_protocol,&LocalAddress, &RemoteAddr, NULL, NULL, NULL, NULL,NULL, NULL,AsyncContext.Irp);
	
    SrkAsyncContextWaitForCompletion(&AsyncContext, &Status);

    if (!NT_SUCCESS(Status))
    {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WskSocketConnect failed %d", Status);
		goto Cleanup;
	}

    Socket = AsyncContext.Irp->IoStatus.Information;

    //
    // Free the async context.
    //

    SrkAsyncContextFree(&AsyncContext);


    Cleanup:
    return Status;

}

NTSTATUS WskSendReceiveData(PWSK_SOCKET Socket, PWSK_BUF SendBuf, PWSK_BUF RecvBuf)
{
    NTSTATUS Status;
    WSK_PROVIDER_CONNECTION_DISPATCH ConnDispatch = *((WSK_PROVIDER_CONNECTION_DISPATCH*)Socket->Dispatch);

    if (ConnDispatch.WskSend == NULL || ConnDispatch.WskReceive == NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s: The WSK_SOCKET does NOT contain the Send\Recv functions.", __func__);
        goto Cleanup;
    }

    //
// Allocate async context.
//

    SRK_ASYNC_CONTEXT AsyncContext;
    Status = KspAsyncContextAllocate(&AsyncContext);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Call the WSK API.
    //
    Status = ConnDispatch.WskSend(Socket, SendBuf, WSK_FLAG_NODELAY, &AsyncContext);

    SrkAsyncContextWaitForCompletion(&AsyncContext, &Status);

    if (!NT_SUCCESS(Status))
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "WskSocketConnect failed %d", Status);
        goto Cleanup;
    }

    Status = ConnDispatch.WskReceive(Socket, RecvBuf, WSK_FLAG_WAITALL, &AsyncContext);


    //
    // Free the async context.
    //

    SrkAsyncContextFree(&AsyncContext);





    Cleanup:
    return Status;
}