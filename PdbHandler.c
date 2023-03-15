#include "Driver.h"

#define RSDS_MAGIC 0x53445352  // "RSDS"
typedef struct _RSDS_PDB_HEADER
{
	UINT32 Magic;
	GUID PdbGuid;
	UINT32 PdbAge;
} RSDS_PDB_HEADER, * PRSDS_PDB_HEADER;



//
//	Symbol Server Url format
//
#define SYMBOL_SERVER_BASE L"https://msdl.microsoft.com/download/symbols/"


NTSTATUS ReadFileContent(IN PHANDLE phFile, LARGE_INTEGER Offset, PVOID Buffer, ULONG BufferSize)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

	Status = ZwReadFile(*phFile, NULL, NULL, NULL, &IoStatus,
		Buffer, BufferSize, &Offset, NULL);

	if (!NT_SUCCESS(Status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read from file at offset %I64d. Status: %d\n", __func__, Offset.QuadPart, Status);
		return Status;
	}

	return Status;
}

NTSTATUS GetFileHandle(OUT PHANDLE phFile, IN PUNICODE_STRING FileName)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatus;

	InitializeObjectAttributes(&ObjectAttributes,
		FileName,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = ZwOpenFile(phFile,
		FILE_GENERIC_READ,
		&ObjectAttributes,
		&IoStatus,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

	return Status;
}

BOOLEAN ParseRsdsHeader(IN PHANDLE phFile, IN ULONG RsdsHeaderOffset, IN ULONG SizeOfHeader, OUT PRSDS_PDB_HEADER pRsdsHeader, OUT PCHAR pPdbFileName)
{
	NTSTATUS Status;
	LARGE_INTEGER Offset;

	Offset.QuadPart = RsdsHeaderOffset;

	Status = ReadFileContent(phFile, Offset, pRsdsHeader, sizeof(RSDS_PDB_HEADER));

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read RSDS Header", __func__);
		return FALSE;
	}

	Offset.QuadPart += sizeof(RSDS_PDB_HEADER);


	if (!(pRsdsHeader->Magic == RSDS_MAGIC))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : RSDS Header magic is invalid", __func__);
		return FALSE;
	}

	Status = ReadFileContent(phFile, Offset, pPdbFileName, SizeOfHeader - sizeof(RSDS_PDB_HEADER));

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read PdbFileName", __func__);
		return FALSE;
	}

	//pPdbFileName[SizeOfHeader - sizeof(RSDS_PDB_HEADER) + 1] = '\x0';

	return TRUE;
}

BOOLEAN GetRsdsPdbEntry(IN PHANDLE phFile, IN IMAGE_NT_HEADERS* pNTHeader, OUT RSDS_PDB_HEADER* pRsdsHeader, OUT PCHAR pPdbFileName)
{

	LARGE_INTEGER DbgDirOffset;
	UINT32 DbgDirCount;
	NTSTATUS Status;
	IMAGE_DEBUG_DIRECTORY DbgDir;
	BOOLEAN result = FALSE;


	DbgDirOffset.QuadPart = pNTHeader->OptionalHeader.DataDirectory[6].VirtualAddress;
	DbgDirCount = pNTHeader->OptionalHeader.DataDirectory[6].Size / sizeof(IMAGE_DEBUG_DIRECTORY);

	/*
	DbgDir = ExAllocatePool2(PagedPool, DbgDirCount * sizeof(IMAGE_DEBUG_DIRECTORY), POOL_TAG);
	*/

	//Status = ReadFileContent(phFile, DbgDirOffset, DbgDir, DbgDirCount * sizeof(IMAGE_DEBUG_DIRECTORY));


	for (UINT32 i = 0; i < DbgDirCount; i++)
	{
		DbgDirOffset.QuadPart += (LONGLONG)(sizeof(IMAGE_DEBUG_DIRECTORY) * i);
		Status = ReadFileContent(phFile, DbgDirOffset, &DbgDir, sizeof(IMAGE_DEBUG_DIRECTORY));

		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read Debug Directory Number %d", __func__, i);
			goto Cleanup;
		}

		if (DbgDir.Type == IMAGE_DEBUG_TYPE_CODEVIEW)
		{
			result = ParseRsdsHeader(phFile, DbgDir.AddressOfRawData,DbgDir.SizeOfData, pRsdsHeader, pPdbFileName);
			goto Cleanup;
		}
	}

Cleanup:
	return result;
}

BOOLEAN GetNtHeader(IN PHANDLE phFile, IMAGE_NT_HEADERS* pNTHeader)
{
	IMAGE_DOS_HEADER	  DOSHeader;
	LARGE_INTEGER DOSHeaderOffset;

	LARGE_INTEGER NTHeaderOffset;

	NTSTATUS Status;
	

	DOSHeaderOffset.QuadPart = 0;
	Status = ReadFileContent(phFile, DOSHeaderOffset, &DOSHeader, sizeof(DOSHeader));

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read Dos Header", __func__);
		return FALSE;
	}

	if (DOSHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : IMAGE_DOS_SIGNATURE is invalid", __func__);
		return FALSE;
	}

	NTHeaderOffset.QuadPart = DOSHeader.e_lfanew;
	Status = ReadFileContent(phFile, NTHeaderOffset, pNTHeader, sizeof(IMAGE_NT_HEADERS));

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to read Nt Header", __func__);
		return FALSE;
	}

	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : IMAGE_NT_SIGNATURE is invalid", __func__);
		return FALSE;
	}

	if (pNTHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : IMAGE_NT_OPTIONAL_HDR_MAGIC is invalid", __func__);
		return FALSE;
	}

	return TRUE;
}

BOOLEAN ConvertGuidToWString(GUID Guid, CHAR *GuidString)
{
	if (sprintf_s(GuidString, (sizeof(GUID) * 2) + 2, "%8X%4X%4X%2X%2X%2X%2X%2X%2X%2X%2X",
		(UINT32)Guid.Data1, Guid.Data2, Guid.Data3,
		Guid.Data4[0], Guid.Data4[1], Guid.Data4[2], Guid.Data4[3],
		Guid.Data4[4], Guid.Data4[5], Guid.Data4[6], Guid.Data4[7]))
	{
		return TRUE;
	}
	return FALSE;
}

BOOLEAN ComposeFullPdbUrl(OUT PUNICODE_STRING PdbUrl, IN RSDS_PDB_HEADER* pRsdsHeader, IN PCHAR pPdbFileName)
{
	NTSTATUS Status;
	ANSI_STRING aPdbFileName;
	UNICODE_STRING uPdbFileName;
	UNICODE_STRING GuidString;

	// https://msdl.microsoft.com/download/symbols/
	Status = RtlAppendUnicodeToString(PdbUrl, SYMBOL_SERVER_BASE);
	
	if (NT_SUCCESS(Status))
	{
		// https://msdl.microsoft.com/download/symbols/<PdbFileName>
		if (strrchr(pPdbFileName, '/') == NULL)
		{
			RtlInitAnsiString(&aPdbFileName, pPdbFileName);
		}
		else
		{
			RtlInitAnsiString(&aPdbFileName,strrchr(pPdbFileName, '/'));
		}

		RtlInitUnicodeString(&uPdbFileName, NULL);
		RtlAnsiStringToUnicodeString(&uPdbFileName, &aPdbFileName, TRUE);
		Status = RtlAppendUnicodeStringToString(PdbUrl, &uPdbFileName);

		if (NT_SUCCESS(Status))
		{
			// https://msdl.microsoft.com/download/symbols/<PdbFileName>/
			RtlAppendUnicodeToString(PdbUrl, L"/");

			// https://msdl.microsoft.com/download/symbols/<PdbFileName>/<PdbGUID>
			Status = RtlStringFromGUID(&pRsdsHeader->PdbGuid, &GuidString);

			CHAR GuidWString[0x20];
			ConvertGuidToWString(pRsdsHeader->PdbGuid, GuidWString);

			if (NT_SUCCESS(Status))
			{
				RtlAppendUnicodeStringToString(PdbUrl, &GuidString);

				// https://msdl.microsoft.com/download/symbols/<PdbFileName>/<PdbGUID>/
				RtlAppendUnicodeToString(PdbUrl, L"/");

				// https://msdl.microsoft.com/download/symbols/<PdbFileName>/<PdbGUID>/<PdbFileName>
				Status = RtlAppendUnicodeStringToString(PdbUrl, &uPdbFileName);
				goto Cleanup;
			}
		}
	}

	DbgPrintAndGotoCleanup("Failed to create the Full Url", Status);

	Cleanup:
	return Status == STATUS_SUCCESS;
}

BOOLEAN GetPdbUrl(OUT PUNICODE_STRING PdbUrl, IN PUNICODE_STRING FileName)
{
	BOOLEAN Result = FALSE;
	NTSTATUS Status;
	HANDLE hFile = { 0 };
	IMAGE_NT_HEADERS NTHeader = { 0 };
	RSDS_PDB_HEADER RsdsHeader = { 0 };
	CHAR PdbFileName[MAX_PDB_FILENAME_SIZE];

	Status = GetFileHandle(&hFile, FileName);
	if (!NT_SUCCESS(Status)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to obtain a file handle %d", __func__, Status);
		goto Cleanup;
	}

	if (!GetNtHeader(&hFile, &NTHeader))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to parse Nt Header", __func__);
		goto Cleanup;
	}

	if (!GetRsdsPdbEntry(&hFile, &NTHeader,&RsdsHeader, PdbFileName))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to parse RSDS Header", __func__);
		goto Cleanup;
	}

	if (!ComposeFullPdbUrl(PdbUrl, &RsdsHeader, PdbFileName))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "%s : Failed to Compose full URL", __func__);
		goto Cleanup;
	}

	Result = TRUE;

	Cleanup: 
	ZwClose(hFile);
	return Result;
}