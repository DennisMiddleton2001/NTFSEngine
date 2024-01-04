/*++

Module Name:

    DUMP_TYPES.H

Abstract:

	Functions for displaying data types.

Author:

	Dennis Middleton      July 1, 2006

Environment:

    User mode

Notes:

Revision History: 

--*/

#pragma once

//
// All display functions accept the following flags.
//
#define FL_BASE10			0x00000001  // Display all numbers in decimal
#define FL_OPEN				0x00000002  // 
#define FL_CLOSE			0x00000004
#define FL_UNDECORATED		0x00000008
#define FL_NOTYPE			0x00000010
#define FL_NOCRLF       	0x00000020
#define FL_NOCLOSINGBRACKET 0x00000040
#define FL_TERSE_INDEX		0x00000080
#define FL_UTC_TIME			0x00000100
#define FL_IS_BITMAP		0x00000200
//
//
//

//
// TYPE IDENTIFIER STRINGS.  THESE MUST NEVER BE CHANGED.
//
#define SZ_U64		TEXT("unsigned __int64")
#define SZ_S64		TEXT("__int64")
#define SZ_U32		TEXT("unsigned long")
#define SZ_S32		TEXT("long")
#define SZ_U32INT	TEXT("unsigned int")
#define SZ_S32INT	TEXT("int")
#define SZ_U16		TEXT("unsigned short")
#define SZ_S16		TEXT("short")
#define SZ_U8		TEXT("unsigned char")
#define SZ_S8		TEXT("char")
#define SZ_U1		TEXT("bool")
//
// ENUMERATED TYPE DEFINITION
//
enum ETYPE { UNKNOWN_TYPE, U64, S64, U32, S32, U16, S16, U8, S8 , U1};
//
// BASE 10 DISPLAY
//
#define SZ_FMT_I64I TEXT(" %020I64i ")
#define SZ_FMT_I64U TEXT(" %020I64u ")
#define SZ_FMT_LI   TEXT(" %08li ")  
#define SZ_FMT_LU   TEXT(" %08lu ")  
#define SZ_FMT_I    TEXT(" %04i ")   
#define SZ_FMT_U	TEXT(" %04u ")
#define SZ_FMT_I    TEXT(" %04i ")
//
// BASE 16 DISPLAY
//
#define SZ_FMT_16X  TEXT("0x%016I64x")
#define SZ_FMT_8X	TEXT("0x%08x")    
#define SZ_FMT_4X   TEXT("0x%04x")    
#define SZ_FMT_2X   TEXT("0x%02x")    
#define SZ_FMT_1X   TEXT("0x%0x")      

#define SET_IF_TYPE_EQUAL(Name1, Name2, TypeVar, SetToIfEqual) if (!_wcsicmp((Name1),(Name2)))  (TypeVar)=(SetToIfEqual)

template <typename ANY_TYPE> 
ETYPE
PrintStructureMember(
		  ANY_TYPE Value,
		  WCHAR * szVariableName,
		  SHORT Indent,
		  DWORD DisplayFlags
		  );

template <typename ANY_TYPE> 
ETYPE 
GetTypeId(
		ANY_TYPE a
		);

template <typename ANY_TYPE> 
ETYPE
GetTypeFormatStrings(
		WCHAR * szTypeFormat,
		WCHAR * szTypeName,
		ANY_TYPE Value,
		DWORD DisplayFlags
		);

template <typename ANY_TYPE> 
ETYPE
WriteValueString(
		WCHAR * szString,
		WCHAR * szVariableName,
		ANY_TYPE Value,
		DWORD DisplayFlags
		);

template <typename ANY_TYPE> 
ETYPE
CreateFormatString(
	  WCHAR * szString,
	  WCHAR * szVariableName,
	  ANY_TYPE Value,
	  DWORD DisplayFlags
	  );

VOID __cdecl
PrintAnyString(
		USHORT Indent, 
		ULONG Flags, 
		WCHAR * szFormat, 
		...
		);

PWCHAR 
CreateNameString(
	  PWCHAR szInput, 
	  size_t Length
	  );



//These two do most of the work.
DWORD OpenOutput(WCHAR * szFilename);
VOID DirectOutput(FILE * OutFile);
DWORD CloseOutput();

VOID PrintFileRecord(PMFT_SEGMENT_REFERENCE pFrs, RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintAttributeStream(PMFT_SEGMENT_REFERENCE Frs,RW_BUFFER * pBuffer, ATTRIBUTE_TYPE_CODE pType, WCHAR * szAttrName, LONGLONG ThisOffset, USHORT Indent, ULONG Flags);

// These print specific structures.

VOID PrintStandardInformation(RW_BUFFER * pBuffer, WCHAR * szFieldName, USHORT Indent, ULONG Flags);
VOID PrintDataAttributeEx(MFT_SEGMENT_REFERENCE Frs, RW_BUFFER * pBuffer, WCHAR * szName, LONGLONG StartOffset, USHORT Indent, ULONG Flags);
VOID PrintDataAttribute(RW_BUFFER * pBuffer, WCHAR * szName, LONGLONG StartOffset, USHORT Indent, ULONG Flags);
VOID PrintIndexEntryByType(RW_BUFFER * pBuffer, PWCHAR szAttributeName, USHORT Indent, ULONG Flags);
VOID PrintIndexEntryFilenameAbbreviated(RW_BUFFER * pBuffer, PWCHAR szAttributeName, USHORT Indent, ULONG Flags);
VOID PrintFullVolumeSummary(PRW_BUFFER pV, PNTFS_VOLUME_DATA_BUFFER pVi, USHORT Indent, ULONG Flags);
VOID PrintVolumeInformation(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintIndexBuffer(MFT_SEGMENT_REFERENCE Frs, RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintFilenameInformation(MFT_SEGMENT_REFERENCE pFrs, RW_BUFFER * pBuffer, WCHAR * szFieldName, USHORT Indent, ULONG Flags);
VOID PrintSiiEntry(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags);
VOID PrintSdhEntry(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags);
VOID PrintReparseIndexKey(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags);
VOID PrintSecurityStream(RW_BUFFER * Buffer, USHORT Indent, ULONG Flags);
VOID PrintReparsePoint(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintVolumeName(RW_BUFFER * pNameBuffer, USHORT Indent, ULONG Flags);
VOID PrintAttributeList(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags);
DWORD PrintObjectSecurity(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags);
VOID PrintQuadword(LONGLONG Number, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintDword(LONG Number, WCHAR * szName, USHORT Indent, ULONG Flags);

VOID PrintRestartPage(PRW_BUFFER pBuffer, USHORT Indent, ULONG Flags);
VOID PrintLogRecordPage(PRW_BUFFER pBuffer, USHORT Indent, ULONG Flags);

