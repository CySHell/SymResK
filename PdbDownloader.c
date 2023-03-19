#include "Driver.h"

extern WSK_CLIENT_DISPATCH WskAppDispatch;
extern WSK_REGISTRATION WskRegistration;
extern WSK_PROVIDER_NPI wskProviderNpi;
extern BOOLEAN WskProviderNpiInitialized;


NTSTATUS DownloadPdb(PUNICODE_STRING PdbUrl)
{
	NTSTATUS Status;
	WSK_SOCKET Socket;
	PADDRINFOEXW res;
	ADDRINFOEXW hints = { 0 };
	UNICODE_STRING SymbolServerNode;

	RtlInitUnicodeString(&SymbolServerNode, SYMBOL_SERVER_NODE);

	if (!WskProviderNpiInitialized)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s: WskProviderNpi is NOT INITIALIZED!", __func__);
		return STATUS_UNSUCCESSFUL;
	}

	hints.ai_flags |= AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	Status = SrkGetAddrInfo(&SymbolServerNode, NULL, &hints, &res);
	if (Status != STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s: SrkGetAddrInfo failed with error: %d", __func__, Status);
		return Status;
	}

	Status = WskCreateSocketAndConnect(res, &Socket);

	if (!NT_SUCCESS(Status))
	{
		goto Cleanup;
	}
	


Cleanup:
	return Status;
}


