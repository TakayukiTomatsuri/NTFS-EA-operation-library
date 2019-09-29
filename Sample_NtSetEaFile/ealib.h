#pragma once

#include <iostream>
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>

//createDispositions which is argument of CreateFile
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

// ***** Functions for writing EA ****

ULONG calcEaEntryLength(
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength);

FILE_FULL_EA_INFORMATION* makeEaEntry(
	IN  ULONG   NextEntryOffset,
	IN  UCHAR   Flags,
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength,
	IN  char* EaName,            // containing terminator 0x00.
	IN  char* EaValue,           // EaValue isn't restricted to Ascii format. So EaValueLength should be contain terminator 0x00 if EaValue is Ascii. (?)
	//IN CHAR   EaName[1];
	OUT ULONG* EaEntryLength
);

PVOID appendEaEntryAtTopOfEaBuffer(
	IN  FILE_FULL_EA_INFORMATION* EaEntry,
	IN  PVOID EaBuffer,
	IN  ULONG EaLength,
	OUT ULONG* ReturnedEaLength
);

int showAllEaEntriesInEaBuffer(
	PVOID EaBuffer
);

LPWSTR getFilePathWithCurrentDirectory(
	IN LPWSTR FileName
);

//**** Definitions for writing EA with NtCreateFile **

//*****************
// not suitable: https://stackoverflow.com/questions/7486896/need-help-using-ntcreatefile-to-open-by-fileindex
// suitable article!: https://www.sysnative.com/forums/threads/ntcreatefile-example.8592/

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


// NO ANY UTILIYT FUNCTIONS for writing EA with NtCreateFile


// **** Definitions for reading EA ****

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

// **** Functions for reading EA ****

ULONG calcEaSearchTargetEntryLength(
	BYTE TargetEaNameLength
);

FILE_GET_EA_INFORMATION* makeEaSeachTargetEntry(
	IN  CHAR   SearchTargetEaName[],
	OUT ULONG* EaSearchTargetEntryLength
);

PVOID appendEaSeachTargetEntryAtTopOfList(
	IN FILE_GET_EA_INFORMATION* EaSeachTargetEntry,
	IN PVOID EaSeachTargetEntryListBuffer,            // can be set to NULL
	IN ULONG EaSeachTargetEntryListBufferLength,      // can be set to 0.
	OUT ULONG* ReturnedEaSeachTargetEntryListBufferLength
);

PVOID makeEaSearchTargetEntryListBuffer(
	IN  CHAR* SearchTargetEaNameList[],
	IN  INT    SearchTargetEaNameListLength,
	OUT ULONG* EaSearchTargetEntryListBufferLength
);

int showAllEaSearchTargetEntriesInBuffer(
	PVOID EaSearchTargetListBuffer
);

// **** Other utility functions ****

#define EA_VALIDATION_SUCCESS              0
#define BAD_EA_NEXTENTRYOFFSET             1
#define BAD_EA_BUFFER_LENGTH               2
#define BAD_EA_NAME_LENGTH                 3
#define EA_VALIDATION_SOMTHING_WRONG       4
//#define CREATE_ALWAYS                    2
//#define OPEN_EXISTING                    3
//#define OPEN_ALWAYS                      4
//#define TRUNCATE_EXISTING                5

typedef LONG EASTATUS;

// For validation before writing. 
// EA buffer which is read has been validated by NtQueryEaBuffer (maybe). So it isn't need validation.
EASTATUS validateEaBuffer(
	PVOID EaBuffer,
	ULONG EaLength
);




