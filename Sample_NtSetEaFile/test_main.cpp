#include"ealib.h"


// First. 
// Simply open file with CrateFile().
// Then, write $EA with NtSetEaFile().
NTSTATUS test_writeSingleEaEntry() {
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



	pNtSetEaFile NtSetEaFile = (pNtSetEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetEaFile");
	ULONG eaLength = -1;
	char name[] = "ea1";
	char val[] = "val1";
	PVOID eaBuffer = makeEaEntry(
		0,
		0,
		strlen(name),
		strlen(val) + 1,
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

NTSTATUS test_writeMultipleEaEntry() {
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
	char name_1[] = "e1aaa"; //’Z‚¢‚ÆInconsistentEa‚È‚ñ‚Æ‚©‚É‚È‚éH
	char val_1[] = "v1";
	FILE_FULL_EA_INFORMATION* eaBuffer_1 = makeEaEntry(
		0,
		0,
		strlen(name_1),
		strlen(val_1) + 1,
		name_1,
		val_1,
		&eaLength_1
	);

	ULONG eaLength_2 = -1;
	char name_2[] = "e2";
	char val_2[] = "v2";
	FILE_FULL_EA_INFORMATION* eaBuffer_2 = makeEaEntry(
		0,
		0,
		strlen(name_2),
		strlen(val_2) + 1,
		name_2,
		val_2,
		&eaLength_2
	);

	ULONG allEaLength = -1;
	PVOID allEaBuffer = appendEaEntryAtTopOfEaBuffer(eaBuffer_1, eaBuffer_2, calcEaEntryLength(eaBuffer_2->EaNameLength, eaBuffer_2->EaValueLength), &allEaLength);

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

//************

NTSTATUS test_writeMultipleEaEntryWithNtCreateFile() {
	_NtCreateFile NtCreateFile = (_NtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtCreateFile");
	_RtlInitUnicodeString RtlInitUnicodeString = (_RtlInitUnicodeString)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlInitUnicodeString");

	HANDLE hFile;
	OBJECT_ATTRIBUTES objAttribs = { 0 };

	//std::cout << "Initializing unicode string..." << std::endl;
	//PCWSTR filePath = L"\\??\\Z:\\NtCreateFileHook.output";
	PCWSTR filePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	UNICODE_STRING unicodeString;
	RtlInitUnicodeString(&unicodeString, filePath);

	//std::cout << "Call to InitializeObjectAttributes for OBJECT_ATTRIBUTES data structure..." << std::endl;
	InitializeObjectAttributes(&objAttribs, &unicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

	//std::cout << "Initializing LARGE_INTEGER for allocation size..." << std::endl;
	const int allocSize = 2048;
	LARGE_INTEGER largeInteger;
	largeInteger.QuadPart = allocSize;

	//std::cout << "Calling NtCreateFile..." << std::endl;

	//---- create EA entry list in EaBuffer----


	ULONG eaLength_1 = -1;
	char name_1[] = "e1aaa"; //’Z‚¢‚ÆInconsistentEa‚È‚ñ‚Æ‚©‚É‚È‚éH
	char val_1[] = "v1";
	FILE_FULL_EA_INFORMATION* eaBuffer_1 = makeEaEntry(
		0,
		0,
		strlen(name_1),
		strlen(val_1) + 1,
		name_1,
		val_1,
		&eaLength_1
	);

	ULONG eaLength_2 = -1;
	char name_2[] = "e2";
	char val_2[] = "v2";
	FILE_FULL_EA_INFORMATION* eaBuffer_2 = makeEaEntry(
		0,
		0,
		strlen(name_2),
		strlen(val_2) + 1,
		name_2,
		val_2,
		&eaLength_2
	);

	ULONG allEaLength = -1;
	PVOID allEaBuffer = appendEaEntryAtTopOfEaBuffer(eaBuffer_1, eaBuffer_2, calcEaEntryLength(eaBuffer_2->EaNameLength, eaBuffer_2->EaValueLength), &allEaLength);

	showAllEaEntriesInEaBuffer(allEaBuffer);
	//-------------------


	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtCreateFile(&hFile, STANDARD_RIGHTS_ALL, &objAttribs, &ioStatusBlock, &largeInteger,
		FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE, allEaBuffer, allEaLength);
	if (hFile != NULL) CloseHandle(hFile);

	return status;
}

//******************

NTSTATUS test_readEaEntryWithNtQueryEaFile() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or Create the victim file.
	LPWSTR victimFilePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	HANDLE hVictimFile = CreateFile(
		victimFilePath
		, GENERIC_WRITE | GENERIC_READ
		, 0
		, NULL
		, OPEN_ALWAYS
		, FILE_ATTRIBUTE_NORMAL
		, NULL
	);

	ULONG eaLength = 5000;
	PVOID eaBuffer = malloc(eaLength);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, FALSE, NULL, NULL, NULL, FALSE);
	showAllEaEntriesInEaBuffer(eaBuffer);

	//// Repeated querying
	//NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);
	//status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);

	return status;
}

NTSTATUS test_readEaEntryWithNtQueryEaFileWithSpecifyingEaName() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or Create the victim file.
	LPWSTR victimFilePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	HANDLE hVictimFile = CreateFile(
		victimFilePath
		, GENERIC_WRITE | GENERIC_READ
		, 0
		, NULL
		, OPEN_ALWAYS
		, FILE_ATTRIBUTE_NORMAL
		, NULL
	);

	// make trget list
	const CHAR* targetEaNames[2];
	targetEaNames[0] = "E2";
	targetEaNames[1] = "E1AAA";

	ULONG eaSearchTargetEntryListBufferLength = -1;
	PVOID eaSearchTargetEntryListBuffer = makeEaSearchTargetEntryListBuffer((CHAR * *)targetEaNames, 2, &eaSearchTargetEntryListBufferLength);
	showAllEaSearchTargetEntriesInBuffer(eaSearchTargetEntryListBuffer);


	// read
	ULONG eaLength = 5000;
	PVOID eaBuffer = malloc(eaLength);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, FALSE, eaSearchTargetEntryListBuffer, eaSearchTargetEntryListBufferLength, NULL, FALSE);
	showAllEaEntriesInEaBuffer(eaBuffer);

	//NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);
	//status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);

	return status;
}

NTSTATUS test_readEaEntryWithNtQueryEaFileWithSpecifyingEaIndex() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or Create the victim file.
	LPWSTR victimFilePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	HANDLE hVictimFile = CreateFile(
		victimFilePath
		, GENERIC_WRITE | GENERIC_READ
		, 0
		, NULL
		, OPEN_ALWAYS
		, FILE_ATTRIBUTE_NORMAL
		, NULL
	);



	ULONG eaLength = 5000;
	PVOID eaBuffer = malloc(eaLength);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	ULONG desiredEaIndex = 2;   // It's one-based index. NOT ZERO-BASED!!!
	NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, FALSE, NULL, NULL, &desiredEaIndex, FALSE);
	showAllEaEntriesInEaBuffer(eaBuffer);

	//NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);
	//status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);

	return status;
}


int main()
{
	std::cout << "Hello World!\n";

	// write single EA entry.
	//return test_writeSingleEaEntry();

	// write multiple EA entries.
	//NTSTATUS status = test_writeMultipleEaEntry();

	// write multiple EA entries with NtCreateFile.
	NTSTATUS writeEaStatus = test_writeMultipleEaEntryWithNtCreateFile();
	//return status;

	// read EA entries
	NTSTATUS readEaStatus = test_readEaEntryWithNtQueryEaFile();
	//NTSTATUS readEaStatus = test_readEaEntryWithNtQueryEaFileWithSpecifyingEaName();
	return readEaStatus;

	// 
}