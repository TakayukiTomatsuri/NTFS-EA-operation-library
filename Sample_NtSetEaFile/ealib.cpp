#include"ealib.h"

ULONG calcEaEntryLength(
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength
){
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
){
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



int showAllEaEntriesInEaBuffer(
	PVOID EaBuffer
) {
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

LPWSTR getFilePathWithCurrentDirectory(
	IN LPWSTR FileName
) {
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



//*****Write EA with NtCreateFile

// NONE


//******** READ ******

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

PVOID appendEaSeachTargetEntryAtTopOfList(
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
		eaSearchTargetEntryListBuffer = appendEaSeachTargetEntryAtTopOfList(eaSearchTargetEntry, eaSearchTargetEntryListBuffer, eaSearchTargetEntryListBufferLength, &eaSearchTargetEntryListBufferLength);
			
		if (eaSearchTargetEntryListBuffer == NULL) {
			OutputDebugString(L"FAILED: makeEaSearchTargetEntryListBuffer: appendEaSeachTargetEntryToListAtTop returned NULL.\n");
			return NULL;
		}
	}

	*EaSearchTargetEntryListBufferLength = eaSearchTargetEntryListBufferLength;
	return eaSearchTargetEntryListBuffer;

}

int showAllEaSearchTargetEntriesInBuffer(
	PVOID EaSearchTargetListBuffer
) {
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


EASTATUS validateEaBuffer(
	PVOID EaBuffer,
	ULONG EaLength
) {
	FILE_FULL_EA_INFORMATION* currentEaEntry = (FILE_FULL_EA_INFORMATION*)EaBuffer;
	ULONG residualEaLength = EaLength;
	ULONG totalOffset = 0;
	ULONG eaEntryIndex = 0;
	while (true) {
		ULONG currentEaEntryLength = calcEaEntryLength(currentEaEntry->EaNameLength, currentEaEntry->EaValueLength);

		// Check EaName Length.
		// (Since the total length of the EA entry is inspected, only EaNameLength or EaValueLength needs to be confirmed.)
		if (currentEaEntry->EaNameLength != strlen(currentEaEntry->EaName)) {
			return BAD_EA_NAME_LENGTH;
		}

		
		// The last entry has 0 in NextEntryOffset.
		if (currentEaEntry->NextEntryOffset == 0) {
			if (residualEaLength == currentEaEntryLength) return EA_VALIDATION_SUCCESS;
			// EaLength is invalid when EaLength don't match sum of the all EA entries length.
			else return BAD_EA_BUFFER_LENGTH;
		}
		else {
			// Check whether EaEntry size match EaEntry->NextEntryOffset.
			if (currentEaEntry->NextEntryOffset != currentEaEntryLength) {
				return BAD_EA_NEXTENTRYOFFSET;
			}
		}

		// Update cursor
		residualEaLength = residualEaLength - currentEaEntryLength;

		totalOffset = totalOffset + currentEaEntry->NextEntryOffset;
		eaEntryIndex = eaEntryIndex + 1;
		currentEaEntry = (FILE_FULL_EA_INFORMATION*)((char*)currentEaEntry + currentEaEntry->NextEntryOffset);
	}

	return EA_VALIDATION_SOMTHING_WRONG;
}



// For validation before querying.
EASTATUS validateEaSearchTargetEntryListBuffer(
	IN PVOID EaSeachTargetEntryListBuffer,
	IN ULONG EaSeachTargetEntryListBufferLength
) {
	FILE_GET_EA_INFORMATION* currentEaSeachTargetEntry = (FILE_GET_EA_INFORMATION*)EaSeachTargetEntryListBuffer;
	ULONG residualEaSeachTargetEntryListBufferLength = EaSeachTargetEntryListBufferLength;
	ULONG totalOffset = 0;
	ULONG eaEntryIndex = 0;
	while (true) {
		ULONG currentEaSeachTargetEntryLength = calcEaSearchTargetEntryLength(currentEaSeachTargetEntry->EaNameLength);

		// Check EaName Length.
		// (Since the total length of the EA entry is inspected, only EaNameLength or EaValueLength needs to be confirmed.)
		if (currentEaSeachTargetEntry->EaNameLength != strlen(currentEaSeachTargetEntry->EaName)) {
			return BAD_EA_NAME_LENGTH;
		}


		// The last entry has 0 in NextEntryOffset.
		if (currentEaSeachTargetEntry->NextEntryOffset == 0) {
			if (residualEaSeachTargetEntryListBufferLength == currentEaSeachTargetEntryLength) return EA_VALIDATION_SUCCESS;
			// EaLength is invalid when EaLength don't match sum of the all EA entries length.
			else return BAD_EA_BUFFER_LENGTH;
		}
		else {
			// Check whether EaEntry size match EaEntry->NextEntryOffset.
			if (currentEaSeachTargetEntry->NextEntryOffset != currentEaSeachTargetEntryLength) {
				return BAD_EA_NEXTENTRYOFFSET;
			}
		}

		// Update cursor
		residualEaSeachTargetEntryListBufferLength = residualEaSeachTargetEntryListBufferLength - currentEaSeachTargetEntryLength;

		totalOffset = totalOffset + currentEaSeachTargetEntry->NextEntryOffset;
		eaEntryIndex = eaEntryIndex + 1;
		currentEaSeachTargetEntry = (FILE_GET_EA_INFORMATION*)((char*)currentEaSeachTargetEntry + currentEaSeachTargetEntry->NextEntryOffset);
	}

	return EA_VALIDATION_SOMTHING_WRONG;
}