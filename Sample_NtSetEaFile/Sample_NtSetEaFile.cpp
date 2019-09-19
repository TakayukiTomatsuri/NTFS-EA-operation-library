#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID    Pointer;
	} DUMMYUNIONNAME;
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;


typedef struct _FILE_FULL_EA_INFORMATION {
	ULONG  NextEntryOffset;
	UCHAR  Flags;
	UCHAR  EaNameLength;
	USHORT EaValueLength;
	CHAR   EaName[1];
} FILE_FULL_EA_INFORMATION, * PFILE_FULL_EA_INFORMATION;

typedef NTSTATUS(__stdcall* pNtSetEaFile)(
	HANDLE           FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID            Buffer,
	ULONG            Length
	);

ULONG calcEaEntryLength(
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength)
{
	ULONG eaEntryLength = sizeof(ULONG) + sizeof(UCHAR) * 2 + sizeof(USHORT) + (EaNameLength + 1) + (EaValueLength + 0);
	//4bytes align. 
	ULONG alignmentBoundry = sizeof(ULONG);
	if(eaEntryLength % alignmentBoundry != 0)  eaEntryLength = eaEntryLength + (alignmentBoundry - (eaEntryLength % alignmentBoundry));
	/*eaEntryLength = eaEntryLength + (eaEntryLength % 4);*/
	return eaEntryLength;
}

FILE_FULL_EA_INFORMATION* makeEaEntry(
	IN  ULONG   NextEntryOffset,
	IN  UCHAR   Flags,
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength,
	IN  char*   EaName,            // containing terminator 0x00.
	IN  char*   EaValue,           // EaValue isn't restlicted to Ascii format. So EaValueLength should be contain terminator 0x00 if EaValue is Ascii.
	//IN CHAR   EaName[1];
	OUT ULONG*  EaEntryLength
)
{
	FILE_FULL_EA_INFORMATION* eaEntryBuffer = NULL;
	ULONG eaEntryLength = calcEaEntryLength(EaNameLength, EaValueLength);
	//4bytes align. 
	/*eaEntryLength = eaEntryLength + (eaEntryLength % 4);*/

	eaEntryBuffer = (FILE_FULL_EA_INFORMATION*)malloc(eaEntryLength);
	if (eaEntryBuffer == NULL) return NULL;

	memset(eaEntryBuffer, 0, eaEntryLength);

	eaEntryBuffer->NextEntryOffset = NextEntryOffset;
	eaEntryBuffer->Flags = 0x00;          // Flags can be zero or 0x80(FILE_NEED_EA)
	eaEntryBuffer->EaNameLength = EaNameLength;
	eaEntryBuffer->EaValueLength = EaValueLength;


	// EaName and EaValue are concatenated. Both of them are terminated with 1byte 0x00.
	memcpy(eaEntryBuffer->EaName, EaName, EaNameLength);
	memcpy(eaEntryBuffer->EaName + eaEntryBuffer->EaNameLength + 1, EaValue, EaValueLength);

	*EaEntryLength = eaEntryLength;
	return eaEntryBuffer;
}

PVOID addEaEntryAtTopOfEaBuffer(
	IN  FILE_FULL_EA_INFORMATION * EaEntry,
	IN  PVOID EaBuffer,
	IN  ULONG EaLength,
	OUT ULONG *ReturnedEaLength
){
	ULONG eaEntryLength = calcEaEntryLength(EaEntry->EaNameLength, EaEntry->EaValueLength);
	char* concatenatedEaBuffer = (char*)malloc(eaEntryLength + EaLength);
	memset(concatenatedEaBuffer, 0, eaEntryLength + EaLength);

	memcpy(concatenatedEaBuffer, EaEntry, eaEntryLength);
	memcpy(concatenatedEaBuffer + eaEntryLength, EaBuffer, EaLength);

	// change firest EA entry's NextEntryOffset.
	((FILE_FULL_EA_INFORMATION*)concatenatedEaBuffer)->NextEntryOffset = eaEntryLength;

	*ReturnedEaLength = eaEntryLength + EaLength;
	return concatenatedEaBuffer;
}



int showAllEaEntriesInEaBuffer(PVOID EaBuffer) {
	FILE_FULL_EA_INFORMATION* currentEaEntry = (FILE_FULL_EA_INFORMATION*)EaBuffer;
	ULONG totalOffset = 0;
	ULONG eaEntryIndex = 0;
	while (true) {
		OutputDebugString(L"hogehoge!");

		//char* detailOfEaEntry = (char*)malloc(5000);
		//sprintf(
		//	detailOfEaEntry,
		//	" EaEntry->NextEntryOffset: %d\n EaEntry->Flags: %d\n EaEntry->EaNameLength: %d\n EaEntry->EaValueLength: %d\n EaName: %s\n EaValue: %s\n\n",
		//	currentEaEntry->NextEntryOffset,
		//	currentEaEntry->Flags,
		//	currentEaEntry->EaNameLength,
		//	currentEaEntry->EaValueLength,
		//	&currentEaEntry->EaName[0],
		//	&currentEaEntry->EaName[currentEaEntry->EaNameLength + 1]
		//);

		//printf(detailOfEaEntry);
		/*std::cout << detailOfEaEntry;*/

		printf("Index: %d\ntotalOffset: %d\n EaEntry->NextEntryOffset: %d\n EaEntry->Flags: %d\n EaEntry->EaNameLength: %d\n EaEntry->EaValueLength: %d\n EaName: %s\n EaValue: %s\n\n", 
			    eaEntryIndex,
				totalOffset,
				currentEaEntry->NextEntryOffset,
				currentEaEntry->Flags,
				currentEaEntry->EaNameLength,
				currentEaEntry->EaValueLength,
				&currentEaEntry->EaName[0],
				&currentEaEntry->EaName[currentEaEntry->EaNameLength + 1]
);

		// The last entry has 0 in NextEntryOffset.
		if (currentEaEntry->NextEntryOffset == 0) break;

		totalOffset = totalOffset + currentEaEntry->NextEntryOffset;
		eaEntryIndex = eaEntryIndex + 1;
		currentEaEntry = (FILE_FULL_EA_INFORMATION*)((char*)currentEaEntry + currentEaEntry->NextEntryOffset);
	
	}

	return 0;
}

LPWSTR getFilePathWithCurrentDirectory( IN LPWSTR FileName) {
	WCHAR szModulePath[MAX_PATH];

	//GetModuleFileNameW(NULL,
	//	szModulePath,
	//	sizeof(szModulePath) / sizeof(szModulePath[0]));


	GetCurrentDirectory(
		sizeof(szModulePath) / sizeof(szModulePath[0]),
		szModulePath
	);

	WCHAR filePath[MAX_PATH];
	memset(filePath, 0, sizeof(WCHAR) * MAX_PATH);

	/*wcscat_s(filePath, "\\??\\");*/
	wcscat_s(filePath, szModulePath);
	wcscat_s(filePath, L"\\");
	wcscat_s(filePath, MAX_PATH, FileName);

	WCHAR* returnFilePath = (WCHAR *)malloc(sizeof(WCHAR) * MAX_PATH);
	memcpy(returnFilePath, filePath, MAX_PATH);
	return returnFilePath;
}


// First. 
// Simply open file with CrateFile().
// Then, write $EA with NtSetEaFile().
NTSTATUS writeSingleEaEntry() {
	//HANDLE victimeFile = CreateFileW(L"hoge");
	LPWSTR victimFilePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	HANDLE hVictimFile = CreateFile(
		  victimFilePath
		, GENERIC_WRITE
		, 0
		, NULL
		, CREATE_ALWAYS
		, FILE_ATTRIBUTE_NORMAL
		, NULL
	);


	DWORD dwWriteSize = 0;
	 WriteFile(hVictimFile, L"helloworld", sizeof(L"helloworld"), &dwWriteSize, NULL);



	pNtSetEaFile NtSetEaFile = (pNtSetEaFile) GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetEaFile");
	ULONG eaLength = -1;
	char name[] = "ea1";
	char val[] = "val1";
	PVOID eaBuffer = makeEaEntry(
		0,
		0,
		strlen(name),
		strlen(val)+1,
		name,
		val,
		&eaLength
	);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength);


	CloseHandle(hVictimFile);
	return status;
}

#include<stdio.h>

NTSTATUS writeMultipleEaEntry() {
	//HANDLE victimeFile = CreateFileW(L"hoge");
	LPWSTR victimFilePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	HANDLE hVictimFile = CreateFile(
		victimFilePath
		, GENERIC_WRITE
		, 0
		, NULL
		, CREATE_ALWAYS
		, FILE_ATTRIBUTE_NORMAL
		, NULL
	);


	DWORD dwWriteSize = 0;
	WriteFile(hVictimFile, L"helloworld", sizeof(L"helloworld"), &dwWriteSize, NULL);




	ULONG eaLength_1 = -1;
	char name_1[] = "e1aaa"; //短いとInconsistentEaなんとかになる？
	char val_1[] = "v1";
	FILE_FULL_EA_INFORMATION *eaBuffer_1 = makeEaEntry(
		0,
		0,
		strlen(name_1),
		strlen(val_1)+1,
		name_1,
		val_1,
		&eaLength_1
	);

	ULONG eaLength_2 = -1;
	char name_2[] = "e2";
	char val_2[] = "v2";
	FILE_FULL_EA_INFORMATION *eaBuffer_2 = makeEaEntry(
		0,
		0,
		strlen(name_2),
		strlen(val_2)+1,
		name_2,
		val_2,
		&eaLength_2
	);

	ULONG allEaLength = -1;
	PVOID allEaBuffer = addEaEntryAtTopOfEaBuffer(eaBuffer_1, eaBuffer_2, calcEaEntryLength(eaBuffer_2->EaNameLength, eaBuffer_2->EaValueLength), &allEaLength);
	
	showAllEaEntriesInEaBuffer(allEaBuffer);

	//eaBuffer_1->NextEntryOffset = eaLength_1;
	//ULONG allEaLength = eaLength_1 + eaLength_2;
	//PVOID allEaBuffer = (PVOID)malloc(allEaLength);
	//memset(allEaBuffer, 0, allEaLength);
	//memcpy(allEaBuffer, eaBuffer_1, eaLength_1);

	//memcpy((char*)allEaBuffer + eaLength_1, eaBuffer_2, eaLength_2);


	////test
	//(FILE_FULL_EA_INFORMATION *)allEaBuffer
	////test

	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	pNtSetEaFile NtSetEaFile = (pNtSetEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetEaFile");
	NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, allEaBuffer, allEaLength);
	//NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, eaBuffer_1, eaLength_1);

	CloseHandle(hVictimFile);
	return status;
}





int main()
{
    std::cout << "Hello World!\n";

	// write single EA entry.
	//return writeSingleEaEntry();

	// write multiple EA entries.
	NTSTATUS status = writeMultipleEaEntry();
	return status;

	// 
}