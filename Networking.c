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


// WSK application routine that creates a WSK socket
NTSTATUS WskCreateSocket(
    _In_ ADDRESS_FAMILY AddressFamily,
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _Out_ PWSK_SOCKET* Socket
)
{
	NTSTATUS status;
    wskProviderNpi.Dispatch->WskSocket(
		wskProviderNpi.Client,
		AddressFamily,
		SocketType,
		Protocol,
		WSK_FLAG_LISTEN_SOCKET,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		Socket,
		NULL,
		NULL,
		&status
	);

}