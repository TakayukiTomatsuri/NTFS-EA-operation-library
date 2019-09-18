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

FILE_FULL_EA_INFORMATION* makeEaEntry(
	IN  ULONG   NextEntryOffset,
	IN  UCHAR   Flags,
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength,
	IN  char*   EaName,
	IN  char*   EaValue,
	//IN CHAR   EaName[1];
	OUT ULONG*  EaEntryLength
)
{
	FILE_FULL_EA_INFORMATION* eaEntryBuffer = NULL;
	ULONG eaEntryLength = sizeof(ULONG) + sizeof(UCHAR) * 2 + sizeof(USHORT) + (EaNameLength + 1) + (EaValueLength + 1);
	//4bytes align. 
	eaEntryLength = eaEntryLength + (eaEntryLength % 4);

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

	/*wcscat_s(filePath, "\\\\?\\");*/
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
		strlen(val),
		name,
		val,
		&eaLength
	);
	IO_STATUS_BLOCK ioStatusBlock = { 0 };
	NTSTATUS status = NtSetEaFile(hVictimFile, &ioStatusBlock, eaBuffer, eaLength);


	CloseHandle(hVictimFile);
	return status;
}


int main()
{
    std::cout << "Hello World!\n";
	return writeSingleEaEntry();
}

// プログラムの実行: Ctrl + F5 または [デバッグ] > [デバッグなしで開始] メニュー
// プログラムのデバッグ: F5 または [デバッグ] > [デバッグの開始] メニュー

// 作業を開始するためのヒント: 
//    1. ソリューション エクスプローラー ウィンドウを使用してファイルを追加/管理します 
//   2. チーム エクスプローラー ウィンドウを使用してソース管理に接続します
//   3. 出力ウィンドウを使用して、ビルド出力とその他のメッセージを表示します
//   4. エラー一覧ウィンドウを使用してエラーを表示します
//   5. [プロジェクト] > [新しい項目の追加] と移動して新しいコード ファイルを作成するか、[プロジェクト] > [既存の項目の追加] と移動して既存のコード ファイルをプロジェクトに追加します
//   6. 後ほどこのプロジェクトを再び開く場合、[ファイル] > [開く] > [プロジェクト] と移動して .sln ファイルを選択します
