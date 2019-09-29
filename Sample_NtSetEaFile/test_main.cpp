#include"ealib.h"


// First. 
// Simply open file with CrateFile().
// Then, write $EA with NtSetEaFile().
NTSTATUS test_writeSingleEaEntry() {
	// Create file
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

	//// Write dummy content
	//DWORD dwWriteSize = 0;
	//WriteFile(hVictimFile, L"helloworld", sizeof(L"helloworld"), &dwWriteSize, NULL);

	// Make EA content
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

	// Write EA
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength);


	CloseHandle(hVictimFile);
	return status;
}

#include<stdio.h>

// Write multiple EA entries
NTSTATUS test_writeMultipleEaEntry() {
	// Create file
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

	//// Write dummy content
	//DWORD dwWriteSize = 0;
	//WriteFile(hVictimFile, L"helloworld", sizeof(L"helloworld"), &dwWriteSize, NULL);

	// Make EA content
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

	// Write EA
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	pNtSetEaFile NtSetEaFile = (pNtSetEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtSetEaFile");
	NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, allEaBuffer, allEaLength);
	//NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, eaBuffer_1, eaLength_1);

	CloseHandle(hVictimFile);
	return status;
}

//************

// Write multiple EA entries with NtCreateFile but not NtSetEaFile
NTSTATUS test_writeMultipleEaEntryWithNtCreateFile() {
	_NtCreateFile NtCreateFile = (_NtCreateFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtCreateFile");
	_RtlInitUnicodeString RtlInitUnicodeString = (_RtlInitUnicodeString)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlInitUnicodeString");

	HANDLE hFile;
	OBJECT_ATTRIBUTES objAttribs = { 0 };

	// Prepare calling for NtCreateFile
	PCWSTR filePath = getFilePathWithCurrentDirectory((LPWSTR)L"victim.txt");
	UNICODE_STRING unicodeString;
	RtlInitUnicodeString(&unicodeString, filePath);

	InitializeObjectAttributes(&objAttribs, &unicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

	const int allocSize = 2048;
	LARGE_INTEGER largeInteger;
	largeInteger.QuadPart = allocSize;


	// Create EA entry list in EaBuffer
	ULONG eaLength_1 = -1;
	char name_1[] = "e1aaa"; 
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
	//Validate EaBuffer
	EASTATUS resultOfValidateEaBuffer = validateEaBuffer(allEaBuffer, allEaLength);

	// Write EA with NtCreateFile
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtCreateFile(&hFile, STANDARD_RIGHTS_ALL, &objAttribs, &ioStatusBlock, &largeInteger,
		FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN_IF, FILE_NON_DIRECTORY_FILE, allEaBuffer, allEaLength);
	if (hFile != NULL) CloseHandle(hFile);

	return status;
}

//******************


// Read all EA entries 
NTSTATUS test_readEaEntryWithNtQueryEaFile() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or create the victim file.
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

	// Read EA
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

	CloseHandle(hVictimFile);
	return status;
}

// Read EA entries specified by EaName
NTSTATUS test_readEaEntryWithNtQueryEaFileWithSpecifyingEaName() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or create the victim file.
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

	// Make trget EaName list
	const CHAR* targetEaNames[2];
	targetEaNames[0] = "E2";
	targetEaNames[1] = "E1AAA";

	ULONG eaSearchTargetEntryListBufferLength = -1;
	PVOID eaSearchTargetEntryListBuffer = makeEaSearchTargetEntryListBuffer((CHAR * *)targetEaNames, 2, &eaSearchTargetEntryListBufferLength);
	showAllEaSearchTargetEntriesInBuffer(eaSearchTargetEntryListBuffer);

	// Validation of EaName list
	EASTATUS resultOfVaridateEaSearchTargetEntryListBuffer = validateEaSearchTargetEntryListBuffer(eaSearchTargetEntryListBuffer, eaSearchTargetEntryListBufferLength);

	// Read EA entries 
	ULONG eaLength = 5000;
	PVOID eaBuffer = malloc(eaLength);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, FALSE, eaSearchTargetEntryListBuffer, eaSearchTargetEntryListBufferLength, NULL, FALSE);
	showAllEaEntriesInEaBuffer(eaBuffer);

	//NTSTATUS status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);
	//status = NtQueryEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength, TRUE, NULL, NULL, NULL, FALSE);
	//showAllEaEntriesInEaBuffer(eaBuffer);

	CloseHandle(hVictimFile);
	return status;
}

// Read EA entry specified by EaIndex
NTSTATUS test_readEaEntryWithNtQueryEaFileWithSpecifyingEaIndex() {
	pNtQueryEaFile NtQueryEaFile = (pNtQueryEaFile)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryEaFile");

	//Open or create the victim file.
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

	// Read EA entries
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

	CloseHandle(hVictimFile);
	return status;
}


int main()
{
	std::cout << "Hello World!\n";

	// **** Write EA ****

	// write single EA entry.
	//return test_writeSingleEaEntry();

	// write multiple EA entries.
	//NTSTATUS status = test_writeMultipleEaEntry();

	// write multiple EA entries with NtCreateFile.
	NTSTATUS writeEaStatus = test_writeMultipleEaEntryWithNtCreateFile();
	//return status;

	// **** Read EA ****

	// Read all EA entries
	//NTSTATUS readEaStatus = test_readEaEntryWithNtQueryEaFile();

	// Read all EA entries specified by EaName
	NTSTATUS readEaStatus = test_readEaEntryWithNtQueryEaFileWithSpecifyingEaName();
	
	// Read EA entry specified by EaIndex
	//NTSTATUS readEaStatus = test_readEaEntryWithNtQueryEaFileWithSpecifyingEaIndex();

	return readEaStatus;
}