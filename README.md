# NTFS EA Operation Library

This is a library for operating NTFS EA with using WinAPI.

EA operations are probably only possible with native API, and there is little information on the internet, so we created it as a model code and a library that make using EA easy.

## What is NTFS EA (Extended Attribute)
NTFS EA (Extended Attribute) is a place where you can save a little data separately from the file itself.
It feels like NTFS's ADS (Alternate Data Stream), which is often used for malware and other bad uses.

However, the location where the EA is stored is not the place where the data itself is located, but the NTFS MFT (Master File Table).

So it is sometimes used for malware. For example, [Zeroaccess used EA](https://www.symantec.com/connect/blogs/trojanzeroaccessc-hidden-ntfs-ea) acctually.

However, EA's tools and information are not so much, so it seems not very popular.

## Implemented functions in this library

### For writing EA
```
// Calculate size of EA entry( FILE_FULL_EA_INFORMATION ) .ULONG calcEaEntryLength(	IN  UCHAR   EaNameLength,	IN  USHORT  EaValueLength);// Make EA entry ( FILE_FULL_EA_INFORMATION ).FILE_FULL_EA_INFORMATION* makeEaEntry(	IN  ULONG   NextEntryOffset,	IN  UCHAR   Flags,	IN  UCHAR   EaNameLength,	IN  USHORT  EaValueLength,	IN  char* EaName,            // containing terminator 0x00.	IN  char* EaValue,           // EaValue isn't restricted to Ascii format. So EaValueLength should be contain terminator 0x00 if EaValue is Ascii. 	//IN CHAR   EaName[1];	OUT ULONG* EaEntryLength);// Make concatenation a EA entry (FILE_FULL_EA_INFORMATION) and EA buffer containing EA entry list. PVOID appendEaEntryAtTopOfEaBuffer(	IN  FILE_FULL_EA_INFORMATION* EaEntry,	IN  PVOID EaBuffer,	IN  ULONG EaLength,	OUT ULONG* ReturnedEaLength);// Show content of list of EA entries .int showAllEaEntriesInEaBuffer(	PVOID EaBuffer);

// For validation of list of EA entries before writing. // EA buffer which is read has been validated by NtQueryEaBuffer (maybe). So it isn't need validation after read.EASTATUS validateEaBuffer(	PVOID EaBuffer,	ULONG EaLength);
```

### For reading EA
```
//  Calculate the size of the FILE_GET_EA_INFORMATION structure which is used to specify the EA entry you want to display.// (The size is not a constant because it is a variable-length structure)ULONG calcEaSearchTargetEntryLength(	BYTE TargetEaNameLength);//  Make FILE_GET_EA_INFORMATION structure which is used to specify the EA entry you want to display.FILE_GET_EA_INFORMATION* makeEaSeachTargetEntry(	IN  CHAR   SearchTargetEaName[],	OUT ULONG* EaSearchTargetEntryLength);// Concat one FILE_GET_EA_INFORMATION structure and its list.PVOID appendEaSeachTargetEntryAtTopOfList(	IN FILE_GET_EA_INFORMATION* EaSeachTargetEntry,	IN PVOID EaSeachTargetEntryListBuffer,            // can be set to NULL	IN ULONG EaSeachTargetEntryListBufferLength,      // can be set to 0.	OUT ULONG* ReturnedEaSeachTargetEntryListBufferLength);// Create a list of FILE_GET_EA_INFORMATION structures from a list of EaNames of EA entries you want to display.PVOID makeEaSearchTargetEntryListBuffer(	IN  CHAR* SearchTargetEaNameList[],	IN  INT    SearchTargetEaNameListLength,	OUT ULONG* EaSearchTargetEntryListBufferLength);// Show contents of list of FILE_GET_EA_INFORMATION.int showAllEaSearchTargetEntriesInBuffer(	PVOID EaSearchTargetListBuffer);

// For validation of list of FILE_GET_EA_INFORMATION structure before querying.EASTATUS validateEaSearchTargetEntryListBuffer(	IN PVOID EaSeachTargetEntryListBuffer,         	IN ULONG EaSeachTargetEntryListBufferLength);
```

## How to use
Include this library.

	#include "ealib.h"


To know how to write code, read this library header file `NTFS-EA-operation-library/Sample_NtSetEaFile/ealib.h` and the test code which actually reads and writes EA using this library `NTFS-EA-operation-library/Sample_NtSetEaFile/test_main.cpp`.

# NTFS EA Operation Library (In Japanese)
NTFSのEAをWinAPIで操作するライブラリです。  

EAの操作はたぶんネイティブAPIでしかできなくて面倒 & ネットにも情報が少ないので、お手本のコード兼、使う敷居を低くするライブラリとして作成しました。

## NTFSのEAってなに？
NTFS のEA（Extended Attribute, 拡張属性）は、よくマルウェアとかに悪い用途で使われたりするNTFSのADS (Alternate Data Stream, 代替データストリーム) のような感じでデータを少し保存できるものです。  

ただしEAが保存される場所はデータの本体があるところじゃなくてNTFSのMFT (Master File Table) です。  

なのでマルウェアに利用されることも少しはあって、実際に Zeroaccess とかに利用されています。  

しかしEAは人気ないみたいで、あんまりツールとか情報があるわけではりません。

## 本ライブラリの機能
### 書き込み系
```
// EAエントリのサイズ計算
ULONG calcEaEntryLength(
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength);

// EAエントリの作成
FILE_FULL_EA_INFORMATION* makeEaEntry(
	IN  ULONG   NextEntryOffset,
	IN  UCHAR   Flags,
	IN  UCHAR   EaNameLength,
	IN  USHORT  EaValueLength,
	IN  char* EaName,            // containing terminator 0x00.
	IN  char* EaValue,           // EaValue isn't restricted to Ascii format. So EaValueLength should be contain terminator 0x00 if EaValue is Ascii. 
	//IN CHAR   EaName[1];
	OUT ULONG* EaEntryLength
);

// EAエントリをリストにする
// (可変長構造体は単に並べるだけでなく各エントリに次のエントリまでのオフセットを持たせる必要があるのでこれを使う）
PVOID appendEaEntryAtTopOfEaBuffer(
	IN  FILE_FULL_EA_INFORMATION* EaEntry,
	IN  PVOID EaBuffer,
	IN  ULONG EaLength,
	OUT ULONG* ReturnedEaLength
);

// EAエントリのリストを解釈し中身を表示
int showAllEaEntriesInEaBuffer(
	PVOID EaBuffer
);

// EAエントリのリストが正しく作られているかチェックする
// (正しくないとWindows APIが文句言うのでわかるけど、原因を調べる為に使う関数）
EASTATUS validateEaBuffer(
	PVOID EaBuffer,
	ULONG EaLength
);
```

### 読み込み系
```
//  表示したいEAエントリを指定するための、FILE_GET_EA_INFORMATION構造体のサイズを計算する(可変長構造体なのでサイズが定数ではない)
ULONG calcEaSearchTargetEntryLength(
	BYTE TargetEaNameLength
);

// 表示したいEAエントリを指定するための、FILE_GET_EA_INFORMATION構造体を作成する
FILE_GET_EA_INFORMATION* makeEaSeachTargetEntry(
	IN  CHAR   SearchTargetEaName[],
	OUT ULONG* EaSearchTargetEntryLength
);

// 表示したいEAエントリを指定するための、FILE_GET_EA_INFORMATION構造体1つと、そのリストをconcatする
PVOID appendEaSeachTargetEntryAtTopOfList(
	IN FILE_GET_EA_INFORMATION* EaSeachTargetEntry,
	IN PVOID EaSeachTargetEntryListBuffer,            // can be set to NULL
	IN ULONG EaSeachTargetEntryListBufferLength,      // can be set to 0.
	OUT ULONG* ReturnedEaSeachTargetEntryListBufferLength
);

// 表示したいEAエントリのEaNameのリストを渡し、FILE_GET_EA_INFORMATION構造体のリストを作成する
PVOID makeEaSearchTargetEntryListBuffer(
	IN  CHAR* SearchTargetEaNameList[],
	IN  INT    SearchTargetEaNameListLength,
	OUT ULONG* EaSearchTargetEntryListBufferLength
);

// FILE_GET_EA_INFORMATION構造体のリストの中身を解釈し表示する
int showAllEaSearchTargetEntriesInBuffer(
	PVOID EaSearchTargetListBuffer
);


// 表示したいEAエントリを指定するための、FILE_GET_EA_INFORMATION構造体のリストが正しく作られているかチェックする
EASTATUS validateEaSearchTargetEntryListBuffer(
	IN PVOID EaSeachTargetEntryListBuffer,         
	IN ULONG EaSeachTargetEntryListBufferLength
);

```


## 本ライブラリの使い方
`#include "ealib.h"`するだけです。  

書き方は、ライブラリのヘッダファイル
`NTFS-EA-operation-library/Sample_NtSetEaFile/ealib.h`と、  
ライブラリを利用しEAを実際に読み書きする`NTFS-EA-operation-library/Sample_NtSetEaFile/test_main.cpp
`を見ればなんとなくわかります。


