#include "Driver.h"

extern WSK_CLIENT_DISPATCH WskAppDispatch;
extern WSK_REGISTRATION WskRegistration;
extern WSK_PROVIDER_NPI wskProviderNpi;
extern BOOLEAN WskProviderNpiInitialized;

NTSTATUS DownloadPdb(PUNICODE_STRING Url)
{
	NTSTATUS Status;
	WSK_SOCKET Socket;

	if (WskProviderNpiInitialized)
	{
		Status = WskCreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &Socket);
		if (NT_SUCCESS(Status))
		{
			goto Cleanup;
		}
	}

	Cleanup:
}