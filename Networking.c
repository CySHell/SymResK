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


NTSTATUS DownloadPdb(PUNICODE_STRING PdbUrl)
{

}