#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>

//createDispositions of CreateFile
// http://www.jbox.dk/sanos/source/include/win32.h.html
#define CREATE_NEW                       1
#define CREATE_ALWAYS                    2
#define OPEN_EXISTING                    3
#define OPEN_ALWAYS                      4
#define TRUNCATE_EXISTING                5

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
	IN  char*   EaValue,           // EaValue isn't restricted to Ascii format. So EaValueLength should be contain terminator 0x00 if EaValue is Ascii. (?)
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

PVOID appendEaEntryAtTopOfEaBuffer(
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
		OutputDebugString(L"ENTRY showAllEaEntriesInEaBuffer\n");

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

		//EaValue isn't restricted to ascii. In order to print EaValue as ascii, it should be terminated 0x00.
		PVOID tmpBufForShowAsAscii = malloc(currentEaEntry->EaValueLength +1);
		memset(tmpBufForShowAsAscii, 0, currentEaEntry->EaValueLength + 1);
		memcpy(tmpBufForShowAsAscii, &currentEaEntry->EaName[currentEaEntry->EaNameLength + 1], currentEaEntry->EaValueLength);

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
		free(tmpBufForShowAsAscii);

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

	wcscat_s(filePath, L"\\??\\");
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


//*****************
// not suitable: https://stackoverflow.com/questions/7486896/need-help-using-ntcreatefile-to-open-by-fileindex
// suitable article!: https://www.sysnative.com/forums/threads/ntcreatefile-example.8592/

//typedef struct _IO_STATUS_BLOCK {
//	union {
//		NTSTATUS Status;
//		PVOID Pointer;
//	} DUMMYUNIONNAME;
//
//	ULONG_PTR Information;
//} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG           Length;
	HANDLE          RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG           Attributes;
	PVOID           SecurityDescriptor;
	PVOID           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef NTSTATUS(__stdcall* _NtCreateFile)(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength
	);

typedef VOID(__stdcall* _RtlInitUnicodeString)(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
	);

#define FILE_CREATE 0x00000002
#define FILE_NON_DIRECTORY_FILE 0x00000040
#define OBJ_CASE_INSENSITIVE 0x00000040L


//https://processhacker.sourceforge.io/doc/ntioapi_8h.html
#define 	FILE_OPEN   0x00000001
#define 	FILE_CREATE   0x00000002
#define 	FILE_OPEN_IF   0x00000003
#define 	FILE_OVERWRITE   0x00000004
#define 	FILE_OVERWRITE_IF   0x00000005

#define InitializeObjectAttributes( i, o, a, r, s ) {    \
      (i)->Length = sizeof( OBJECT_ATTRIBUTES );         \
      (i)->RootDirectory = r;                            \
      (i)->Attributes = a;                               \
      (i)->ObjectName = o;                               \
      (i)->SecurityDescriptor = s;                       \
      (i)->SecurityQualityOfService = NULL;              \
   }



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
	char name_1[] = "e1aaa"; //短いとInconsistentEaなんとかになる？
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


//******** READ ******
typedef NTSTATUS(__stdcall* pNtQueryEaFile)(
	IN HANDLE               FileHandle,
	OUT PIO_STATUS_BLOCK    IoStatusBlock,
	OUT PVOID               Buffer,
	IN ULONG                Length,
	IN BOOLEAN              ReturnSingleEntry,       
	IN PVOID                EaList OPTIONAL,         // Specify a list of FILE_GET_EA_INFORMATION in order to search EA entries by EaName.
	IN ULONG                EaListLength,            // If EaList is not specified, this argument can be set to NULL, too.
	IN PULONG               EaIndex OPTIONAL,        // One-based index. NOT ZERO-BASED !!! If it specified, it returned only one EA entry.
	IN BOOLEAN              RestartScan              // Basically, NtQueryEafile returns the next entries after the last returned EA entry. If true, returns EA entries starting at index 0 always.
	);

#define MAX_EA_NAME_LENGTH 255

typedef struct _FILE_GET_EA_INFORMATION {
	ULONG                   NextEntryOffset;
	BYTE                    EaNameLength;
	CHAR                    EaName[1];
} FILE_GET_EA_INFORMATION, * PFILE_GET_EA_INFORMATION;

ULONG calcEaSearchTargetEntryLength(
	BYTE TargetEaNameLength
) {
	ULONG eaSearchTargetEntryLength = sizeof(ULONG) + sizeof(BYTE) + (TargetEaNameLength + 1);

	// align 4bytes boundry
	size_t alignmentBoundry = sizeof(ULONG);
	if (eaSearchTargetEntryLength % alignmentBoundry != 0) {
		eaSearchTargetEntryLength = eaSearchTargetEntryLength + (alignmentBoundry - (eaSearchTargetEntryLength % alignmentBoundry));
	}

	return eaSearchTargetEntryLength;
}

FILE_GET_EA_INFORMATION* makeEaSeachTargetEntry(
	IN  CHAR   SearchTargetEaName[],
	OUT ULONG *EaSearchTargetEntryLength
) {
	size_t nameLength = strlen(SearchTargetEaName);
	if (nameLength > MAX_EA_NAME_LENGTH) return NULL;

	*EaSearchTargetEntryLength = calcEaSearchTargetEntryLength(nameLength);
	FILE_GET_EA_INFORMATION* eaSearchTargetEntry = (FILE_GET_EA_INFORMATION*)malloc(*EaSearchTargetEntryLength);

	eaSearchTargetEntry->NextEntryOffset = 0;
	eaSearchTargetEntry->EaNameLength = (BYTE)nameLength;
	memcpy(eaSearchTargetEntry->EaName, SearchTargetEaName, nameLength + 1); //copy start to terminator character.

	return eaSearchTargetEntry;
}

PVOID appendEaSeachTargetEntryToListAtTop(
	IN FILE_GET_EA_INFORMATION* EaSeachTargetEntry,
	IN PVOID EaSeachTargetEntryListBuffer,            // can be set to NULL
	IN ULONG EaSeachTargetEntryListBufferLength,      // can be set to 0.
	OUT ULONG * ReturnedEaSeachTargetEntryListBufferLength
) {
	if (EaSeachTargetEntryListBufferLength < 0) {
		OutputDebugString(L"FAILED: appendEaSeachTargetEntryToListAtTop: Invalid EaSeachTargetEntryListBufferLength.\n");
		return NULL;
	}
	//calc length
	ULONG eaSeachTargetEntryLength = calcEaSearchTargetEntryLength(EaSeachTargetEntry->EaNameLength);
	*ReturnedEaSeachTargetEntryListBufferLength = EaSeachTargetEntryListBufferLength + eaSeachTargetEntryLength;

	//allocate
	PVOID concatenatedEaSeachTargetEntryListBuffer = malloc(*ReturnedEaSeachTargetEntryListBufferLength);
	if (concatenatedEaSeachTargetEntryListBuffer == NULL) {
		OutputDebugString(L"FAILED: appendEaSeachTargetEntryToListAtTop: memory allocation failed.\n");
		return NULL;
	}
	memset(concatenatedEaSeachTargetEntryListBuffer, 0, *ReturnedEaSeachTargetEntryListBufferLength);

	// concat
	memcpy(concatenatedEaSeachTargetEntryListBuffer, EaSeachTargetEntry, eaSeachTargetEntryLength);
	//if NULL, memcpy don't work
	if (EaSeachTargetEntryListBuffer != NULL) {
		memcpy( (char *)concatenatedEaSeachTargetEntryListBuffer + eaSeachTargetEntryLength, EaSeachTargetEntryListBuffer, EaSeachTargetEntryListBufferLength);
		//  change firest entry's NextEntryOffset.
		((FILE_GET_EA_INFORMATION*)concatenatedEaSeachTargetEntryListBuffer)->NextEntryOffset = eaSeachTargetEntryLength;
	}
	
	return concatenatedEaSeachTargetEntryListBuffer;
}

PVOID makeEaSearchTargetEntryListBuffer(
	IN  CHAR   *SearchTargetEaNameList[],
	IN  INT    SearchTargetEaNameListLength,
	OUT ULONG  *EaSearchTargetEntryListBufferLength
) 
{
	//FILE_GET_EA_INFORMATION* eaSearchTargetEntry = NULL;
	//*EaSearchListLength =  
	//eaSearchTargetEntry = (FILE_GET_EA_INFORMATION*)malloc(sizeof(FILE_GET_EA_INFORMATION));
	//if (eaSearchTargetEntry) return NULL;

	PVOID eaSearchTargetEntryListBuffer = NULL;
	ULONG eaSearchTargetEntryListBufferLength = 0;
	for (int ind = 0; ind < SearchTargetEaNameListLength; ind++) {
		if (SearchTargetEaNameList[ind] == NULL) {
			OutputDebugString(L"FAILED: makeEaSearchTargetEntryListBuffer: searchTargetEaName is NULL.\n");
			return NULL;
		}

		size_t nameLength = strlen(SearchTargetEaNameList[ind]);
		if (nameLength > MAX_EA_NAME_LENGTH){
			OutputDebugString(L"FAILED: makeEaSearchTargetEntryListBuffer: searchTargetEaName is too long.\n");
			return NULL;
		}

		//eaSearchTargetEntry->EaNameLength = (BYTE)nameLength;
		//eaSearchTargetEntry->EaName = malloc(nameLength + 1);
		ULONG _eaSearchTargetEntryLength = -1;
		FILE_GET_EA_INFORMATION *eaSearchTargetEntry = makeEaSeachTargetEntry(SearchTargetEaNameList[ind], &_eaSearchTargetEntryLength);
		eaSearchTargetEntryListBuffer = appendEaSeachTargetEntryToListAtTop(eaSearchTargetEntry, eaSearchTargetEntryListBuffer, eaSearchTargetEntryListBufferLength, &eaSearchTargetEntryListBufferLength);
			
		if (eaSearchTargetEntryListBuffer == NULL) {
			OutputDebugString(L"FAILED: makeEaSearchTargetEntryListBuffer: appendEaSeachTargetEntryToListAtTop returned NULL.\n");
			return NULL;
		}
	}

	*EaSearchTargetEntryListBufferLength = eaSearchTargetEntryListBufferLength;
	return eaSearchTargetEntryListBuffer;

}

int showAllEaSearchTargetEntriesInBuffer(PVOID EaSearchTargetListBuffer) {
	FILE_GET_EA_INFORMATION* currentEaSearchTargetEntry = (FILE_GET_EA_INFORMATION*)EaSearchTargetListBuffer;
	ULONG totalOffset = 0;
	ULONG eaEntryIndex = 0;
	while (true) {
		OutputDebugString(L"ENTRY showAllEaSearchTargetEntriesInBuffer\n");

		printf("Index: %d\ntotalOffset: %d\n EaEntry->NextEntryOffset: %d\n EaEntry->EaNameLength: %d\n EaName: %s\n\n",
			eaEntryIndex,
			totalOffset,
			currentEaSearchTargetEntry->NextEntryOffset,
			currentEaSearchTargetEntry->EaNameLength,
			&currentEaSearchTargetEntry->EaName[0]
		);

		// The last entry has 0 in NextEntryOffset.
		if (currentEaSearchTargetEntry->NextEntryOffset == 0) break;

		totalOffset = totalOffset + currentEaSearchTargetEntry->NextEntryOffset;
		eaEntryIndex = eaEntryIndex + 1;
		currentEaSearchTargetEntry = (FILE_GET_EA_INFORMATION*)((char*)currentEaSearchTargetEntry + currentEaSearchTargetEntry->NextEntryOffset);

	}

	return 0;
}

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
	PVOID eaSearchTargetEntryListBuffer = makeEaSearchTargetEntryListBuffer((CHAR **)targetEaNames, 2, &eaSearchTargetEntryListBufferLength);
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