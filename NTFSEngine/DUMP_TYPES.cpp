/*++

Module Name:

    DUMP_TYPES.CPP

Abstract:

	Functions for displaying data types.

Author:

	Dennis Middleton      July 1, 2006

Environment:

    User mode

Notes:

Revision History: 

--*/

#include "StdAfx.h"
#include "ntfs_volume.h"

#define IDX_STEP 4

static ULONG gSeqBits;
static ULONG gLogPage;

const WCHAR * gOpMap[] =
{
    L"NOP",
    L"CompensationLogRecord",
    L"Init FRS",
    L"Delete FRS",
    L"Write EOF FRS",
    L"CreateAttribute",
    L"DeleteAttribute",
    L"UpdateResidentValue",
    L"UpdateNonresidentValue",
    L"UpdateMappingPairs",
    L"DeleteDirtyClusters",
    L"SetNewAttributeSizes",
    L"AddIndexEntryRoot",
    L"DeleteIndexEntryRoot",
    L"AddIndexEntryAllocation",
    L"DeleteIndexEntryAllocation",
    L"WriteEndOfIndexBuffer",
    L"SetIndexEntryVcnRoot",
    L"SetIndexEntryVcnAllocation",
    L"UpdateFileNameRoot",
    L"UpdateFileNameAllocation",
    L"SetBitsInNonresidentBitMap",
    L"ClearBitsInNonresidentBitMap",
    L"HotFix",
    L"EndTopLevelAction",
    L"PrepareTransaction",
    L"CommitTransaction",
    L"ForgetTransaction",
    L"OpenNonresidentAttribute",
    L"OpenAttributeTableDump",
    L"AttributeNamesDump",
    L"DirtyPageTableDump",
    L"TransactionTableDump",
    L"UpdateRecordDataRoot",
    L"UpdateRecordDataAllocation"
};

const WCHAR * gTransStateMap[] = {
    L"TransactionUninitialized",
    L"TransactionActive",
    L"TransactionPrepared",
    L"TransactionCommitted"
};

int gcOpMap = (sizeof(gOpMap) / sizeof(char *));
int gcTransStateMap = (sizeof(gTransStateMap) / sizeof(char *));
int gSeqNumberBits = 0;
int gLogFileSize = 0;

extern FILE* gOutput = NULL;

DWORD OpenOutput(WCHAR * szFilename)
{
	gOutput = _wfopen(szFilename, L"w");
	return (gOutput==0) ? GetLastError() : NO_ERROR;
}

VOID DirectOutput(FILE* OutputFile)
{
	gOutput = OutputFile;
	return;
}

DWORD CloseOutput()
{
	FILE * fTmp = gOutput;
	gOutput = NULL;
	return fclose(fTmp);
}

//
// Internal functions.
//

VOID PrintFrsHeader(PFILE_RECORD_SEGMENT_HEADER pFrsHeader, USHORT Indent, ULONG Flags);
VOID PrintMultiSectorHeader(PMULTI_SECTOR_HEADER pHeader, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintFileReference(PMFT_SEGMENT_REFERENCE pFileReference, WCHAR * szFieldName, USHORT Indent, ULONG Flags);
VOID PrintAttributeRecordHeader(MFT_SEGMENT_REFERENCE FileReference, _ATTRIBUTE_RECORD_HEADER * pV, WCHAR * szFieldName, USHORT Indent, ULONG Flags);
VOID PrintExtents(PATTRIBUTE_RECORD_HEADER pV, USHORT Indent, ULONG Flags);
VOID PrintObjectId(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintIndexRoot(PINDEX_ROOT pV, USHORT Indent, ULONG Flags);
VOID PrintIndexHeader(PINDEX_HEADER pV, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintIndexAllocationBufferStructure(PINDEX_ALLOCATION_BUFFER pV, USHORT Indent, ULONG Flags);
VOID PrintIndexEntryStructure(PINDEX_ENTRY pV, BOOL FileReference, USHORT Indent, ULONG Flags);
VOID PrintSecurityDescriptorHeader(PSECURITY_DESCRIPTOR_HEADER  pV, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintSecurityHashKey(PSECURITY_HASH_KEY pV, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID  FormatAccessMaskText(ACCESS_MASK Mask, WCHAR * szAccess);
VOID  PrintAcl(PACL pDacl, WCHAR * szName, USHORT Indent, ULONG Flags);
BOOL  GetTextualSid(PSID pSid, LPWSTR szTextualSid, LPDWORD dwBufferLen);
VOID PrintIndexAllocationBuffer(PINDEX_ALLOCATION_BUFFER pV, DWORD BufferLength, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintGuid(USHORT Indent, ULONG Flags, GUID * Guid, WCHAR * szName);
VOID PrintObjectIdIndexValue(POBJID_INDEX_ENTRY_VALUE pV, WCHAR * szName, USHORT Indent, ULONG Flags);
VOID PrintFiletime(LONGLONG Time, WCHAR * szName, USHORT Indent, ULONG Flags);

VOID 
TypeDbgPrint(
	    LONG			MessageImportance,
		CHAR			* FunctionName,
		LONG			Line,
		CHAR			* szFormat,
		...
		)
{  
	va_list ArgList;
	size_t StringLen = strlen(FunctionName) + strlen(szFormat) + 16;
	CHAR * FormatString  = new CHAR [StringLen];
	sprintf(FormatString, "%s (%lu) : %s", FunctionName, Line, szFormat);
	va_start(ArgList, szFormat );
	vfprintf(gOutput, FormatString, ArgList);		
	delete [] FormatString;
}


VOID 
PrintAnyString(
		USHORT Indent, 
		ULONG Flags, 
		WCHAR * szFormat, 
		...
		)
{
	PWCHAR szIndent = new WCHAR [Indent+1];
	PWCHAR szString = new WCHAR [512];

	for (SHORT i=0;i<Indent;i++) szIndent[i] = L' ';
	szIndent[Indent]='\0';

	va_list args;

	va_start( args, szFormat);

	wvsprintf(szString, szFormat, args);
	fwprintf(gOutput, L"%s%s", szIndent, szString);

	if (!IS_FLAG_SET(Flags, FL_NOCRLF))
	{
		fwprintf(gOutput, L"\n");
	}

	delete [] szIndent;
	delete [] szString;
	return;
}

template <typename ANY_TYPE> 
ETYPE GetTypeId(
		ANY_TYPE a
		)
{
	ETYPE TypeId;
	size_t NumberOfChars = strlen(typeid( a ).name())+1;
	PWCHAR szTypeName = new WCHAR [NumberOfChars];
	
	wsprintf(szTypeName, TEXT("%S"), typeid( a ).name());

	TypeId=UNKNOWN_TYPE;	
	
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U64,    TypeId   , U64);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_S64,    TypeId   , S64);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U32,    TypeId   , U32);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_S32,    TypeId   , S32);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U16,    TypeId   , U16);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_S16,    TypeId   , S16);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U8 ,    TypeId   , U8 );
	SET_IF_TYPE_EQUAL(szTypeName,SZ_S8 ,    TypeId   , S8 );
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U1 ,    TypeId   , U1 );
	SET_IF_TYPE_EQUAL(szTypeName,SZ_S32INT, TypeId   , S32);
	SET_IF_TYPE_EQUAL(szTypeName,SZ_U32INT, TypeId   , S32);

	delete [] szTypeName;
	return TypeId;
}

template <typename ANY_TYPE> 
ETYPE
GetTypeFormatStrings(
		WCHAR * szTypeFormat,
		WCHAR * szTypeName,
		ANY_TYPE Value,
		DWORD Flags
		)
{
	
	ETYPE TypeId = GetTypeId(Value);
	
	switch (TypeId)
	{
	case S64:
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_I64I : SZ_FMT_16X );
		wcscpy(szTypeName, TEXT("Int64"));					  
		break;											  
	case U64:											  
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_I64U : SZ_FMT_16X );
		wcscpy(szTypeName, TEXT("UInt64"));				  
		break;											  
	case S32:											  
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_LI   : SZ_FMT_8X);
		wcscpy(szTypeName, TEXT("Int32"));						  
		break;											  
	case U32:											  
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_LU   : SZ_FMT_8X);
		wcscpy(szTypeName, TEXT("UInt32"));					  
		break;											  
	case S16:											  
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_I    : SZ_FMT_4X);
		wcscpy(szTypeName, TEXT("Int16"));
		break;
	case U16:
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_U    : SZ_FMT_4X);
		wcscpy(szTypeName, TEXT("UInt16"));
		break;
	case S8:
        wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_I    : SZ_FMT_2X);
		wcscpy(szTypeName, TEXT("CHAR"));
		break;
	case U8:
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_U    : SZ_FMT_2X);
		wcscpy(szTypeName, TEXT("UCHAR"));
		break;
	case U1:
		wcscpy(szTypeFormat, (Flags & FL_BASE10) ? SZ_FMT_U    : SZ_FMT_1X);
		wcscpy(szTypeName, TEXT("bool"));
		break;
	default:
		{
			WCHAR * szTemp = new WCHAR [MAX_PATH];
			wcscpy(szTemp,TEXT(""));
			wsprintf(szTemp,TEXT("%S"), typeid( Value ).name());
			wcscpy(szTypeName, szTemp+7);
			delete [] szTemp;
		}
		break;
	}

	return TypeId;
}

template <typename ANY_TYPE> 
ETYPE
CreateFormatString(
		  WCHAR * szString,
		  WCHAR * szVariableName,
		  ANY_TYPE Value,
		  DWORD Flags
		  )
{

	PWCHAR szTypeFormat, szTypeName;

	szTypeFormat = new WCHAR [512];
	szTypeName   = new WCHAR [512];
								   
    ETYPE TypeId = GetTypeFormatStrings(szTypeFormat, szTypeName, Value, Flags);
	if (IS_FLAG_SET(Flags, FL_NOTYPE))
	{
		TypeId = UNKNOWN_TYPE;

	}

	switch (TypeId)				   
	{							   
	case UNKNOWN_TYPE:
		if (IS_FLAG_SET(Flags, FL_NOTYPE)) {
			wsprintf(szString,
				TEXT("%-10s %-22s: "),
				TEXT("....."),
				szVariableName);

		} else if (IS_FLAG_SET(Flags, FL_UNDECORATED)) {
			wsprintf(szString,
				TEXT("%s {"),
				szVariableName);
		} else {
				//Not a primative type.  P rint this as a struct header.
			wsprintf(szString, TEXT("%s %s {"), szTypeName, szVariableName);
		}
		break;
	default:
		if (Flags & FL_BASE10)
		{
			wsprintf(szString,
				TEXT("%-10s %-22s: %s"),	//Spaced for decimal output.
				szTypeName,
				szVariableName,
				szTypeFormat); 
		}
		else
		{
			wsprintf(szString,
				TEXT("%-10s %-22s: %s"),   //Spaced for hex output.
				szTypeName,
				szVariableName,
				szTypeFormat);
		}

		if (IS_FLAG_SET(Flags, FL_UNDECORATED))
		{
				wsprintf(szString, TEXT("%s"), (szString+11));
		}
		break;
	}

	delete [] szTypeFormat;
	delete [] szTypeName;

	return TypeId;
}

template <typename ANY_TYPE> 
ETYPE
WriteValueString(
		  WCHAR * szString,
		  WCHAR * szVariableName,
		  ANY_TYPE Value,
		  DWORD Flags
		  )
{

	WCHAR szFormat[256];
	
	ETYPE TypeId = CreateFormatString(szFormat, szVariableName, Value, Flags);
	
	if (TypeId==UNKNOWN_TYPE)
	{
		//This is not a primitive type
		wcscpy(szString, szFormat);
	}
	else
	{
		// Format string is in the format "ULONG VariableName : 0x%08x"
		wsprintf(szString, szFormat, Value); 
	}

	return TypeId;
}

template <typename ANY_TYPE> 
ETYPE
PrintStructureMember(
		  ANY_TYPE Value,
		  WCHAR * szVariableName,
		  SHORT Indent,
		  DWORD Flags
		  )
{
	ETYPE TypeId = UNKNOWN_TYPE;
	size_t	StringLen = 0;
	PWCHAR szString = new WCHAR [512];

	if (IS_FLAG_SET(Flags, FL_CLOSE))	{
		// Do not display the type name.
		// Only for use on headers.
		// EXAMPLE:    }
		wcscpy(szString,TEXT("}"));
	} else {
			// Display the type name normally.
			//
			// EXAMPLE:    ANY_STRUCTURE VariableName {
			TypeId = WriteValueString(szString, szVariableName, Value, Flags);
	}

    PrintAnyString(Indent, Flags, L"%s", szString);
	
	delete [] szString;
	return TypeId;
}

PWCHAR 
CreateNameString(
	  PWCHAR szInput, 
	  size_t Length
	  )
{
	WCHAR * szOut = new WCHAR [Length+1];
	if (szOut)
	{
		wcsncpy(szOut, szInput, Length);
		szOut[Length]=0;
	}
	return szOut;
}

VOID PrintFrsHeader(PFILE_RECORD_SEGMENT_HEADER pFrsHeader, USHORT Indent, ULONG Flags)
{
	MFT_SEGMENT_REFERENCE ThisFrs={0};
	PrintStructureMember(*pFrsHeader, L"", Indent, Flags);
	Indent += IDX_STEP;
	
		PrintMultiSectorHeader(&pFrsHeader->MultiSectorHeader, L"MultiSectorHeader", Indent, Flags);
		PrintStructureMember(pFrsHeader->Lsn.QuadPart, L"Lsn", Indent, Flags);
		PrintStructureMember(pFrsHeader->SequenceNumber, L"SequenceNumber", Indent, Flags);
		PrintStructureMember(pFrsHeader->ReferenceCount, L"ReferenceCount", Indent, Flags);
		PrintStructureMember(pFrsHeader->FirstAttributeOffset,L"FirstAttributeOffset", Indent, Flags);
		PrintStructureMember(pFrsHeader->Flags,L"Flags", Indent, Flags);
		PrintStructureMember(pFrsHeader->FirstFreeByte, L"FirstFreeByte", Indent, Flags);
		PrintStructureMember(pFrsHeader->BytesAvailable, L"BytesAvailable", Indent, Flags);
		PrintFileReference(&pFrsHeader->BaseFileRecordSegment, L"BaseFileRecordSegment", Indent, Flags);
		PrintStructureMember(pFrsHeader->NextAttributeInstance, L"NextAttributeInstance", Indent, Flags);

		ThisFrs.SequenceNumber = pFrsHeader->SequenceNumber;
		ThisFrs.SegmentNumberHighPart = pFrsHeader->SegmentNumberHighPart;
		ThisFrs.SegmentNumberLowPart = pFrsHeader->SegmentNumberLowPart;

		PrintFileReference(&ThisFrs, L"ThisFileReference", Indent, Flags);

	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintMultiSectorHeader(PMULTI_SECTOR_HEADER pHeader, WCHAR * szName, USHORT Indent, ULONG Flags)
{	
	WCHAR * szSignature = new WCHAR [5];
	PrintStructureMember(*pHeader, szName, Indent, Flags);
	
	for (int i=0; i<4; i++)
	{
		szSignature[i] = (WCHAR) * (PCHAR) ADD_PTR(&pHeader->Signature, i);
	}
	szSignature[4]=0;

	Indent += IDX_STEP;
		PrintStructureMember(pHeader->Signature, L"Signature", Indent+4, Flags | FL_NOCRLF);
		PrintAnyString(1, Flags, L"\"%s\"", szSignature);
		PrintStructureMember(pHeader->SequenceArrayOffset, L"SequenceArrayOffset", Indent+4, Flags);
		PrintStructureMember(pHeader->SequenceArraySize, L"SequenceArraySize", Indent+4, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
	delete [] szSignature;
	//todo print the array
}

VOID PrintFileReference(PMFT_SEGMENT_REFERENCE pFileReference, WCHAR * szFieldName, USHORT Indent, ULONG Flags)
{
	ULONGLONG SegmentNumber = NtfsFullSegmentNumber(pFileReference);
	ULONGLONG Seq = pFileReference->SequenceNumber;
	
	WCHAR * pszHyperlink = new WCHAR[MAX_PATH];
	wsprintf(pszHyperlink, L"<\\\\FRS:%08I64x>", SegmentNumber);

	PrintStructureMember(*pFileReference, szFieldName, Indent, Flags);
	Indent += IDX_STEP;
		PrintStructureMember(SegmentNumber, L"SegmentNumber", Indent, Flags | FL_NOCRLF);
		PrintAnyString(2, 0, L"%s", pszHyperlink);
		PrintStructureMember(pFileReference->SequenceNumber, L"SequenceNumber", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");

	delete pszHyperlink;
}

WCHAR * GetAttributeTypeName(ATTRIBUTE_TYPE_CODE Type)
{
	WCHAR * szOut = new WCHAR [40];

	switch (Type)
	{
			case $STANDARD_INFORMATION:
				wcscpy(szOut,L"$STANDARD_INFORMATION");
				break;
			case $ATTRIBUTE_LIST:
				wcscpy(szOut,L"$ATTRIBUTE_LIST");
				break;
			case $FILE_NAME:
				wcscpy(szOut,L"$FILE_NAME");
				break;
			case $OBJECT_ID:
				wcscpy(szOut,L"$OBJECT_ID");
				break;
			case $SECURITY_DESCRIPTOR:
				wcscpy(szOut,L"$SECURITY_DESCRIPTOR");
				break;
			case $VOLUME_NAME:
				wcscpy(szOut,L"$VOLUME_NAME");
				break;
			case $VOLUME_INFORMATION:
				wcscpy(szOut,L"$VOLUME_INFORMATION");
				break;
			case $DATA:
				wcscpy(szOut,L"$DATA");
				break;
			case $INDEX_ROOT:
				wcscpy(szOut,L"$INDEX_ROOT");
				break;
			case $INDEX_ALLOCATION:
				wcscpy(szOut,L"$INDEX_ALLOCATION");
				break;
			case $BITMAP:
				wcscpy(szOut,L"$BITMAP");
				break;
			case $REPARSE_POINT:
				wcscpy(szOut,L"$REPARSE_POINT");
				break;
			case $EA_INFORMATION:
				wcscpy(szOut,L"$EA_INFORMATION");
				break;
			case $EA:
				wcscpy(szOut,L"$EA");
				break;
			case $LOGGED_UTILITY_STREAM:
				wcscpy(szOut,L"$LOGGED_UTILITY_STREAM");
				break;
			default:
				wcscpy(szOut,L"**UNKNOWN_TYPE**");
				break;
	}
	return szOut;
}

VOID PrintAttributeRecordHeader(MFT_SEGMENT_REFERENCE Frs, _ATTRIBUTE_RECORD_HEADER * pV, WCHAR * szFieldName, USHORT Indent, ULONG Flags)
{
	WCHAR * szHyperlink = new WCHAR[MAX_PATH];


	PWCHAR szName = CreateNameString((PWCHAR) ADD_PTR(pV, pV->NameOffset) ,pV->NameLength);
	PWCHAR szAttrType = GetAttributeTypeName(pV->TypeCode);
	PrintStructureMember(*pV, szFieldName, Indent, Flags  | FL_NOCRLF);
	wsprintf(szHyperlink, L"<\\\\STR:%08I64x:%lx:%s>", Frs, pV->TypeCode, szName);
	PrintAnyString(Indent, Flags, L"        %s:\"%s\" (%s)", szAttrType, szName, szHyperlink);
	delete [] szHyperlink;
	delete [] szName;
	delete [] szAttrType;
	Indent += IDX_STEP;
		PrintStructureMember(pV->TypeCode, L"TypeCode", Indent, Flags);
		PrintStructureMember(pV->RecordLength, L"RecordLength", Indent, Flags);
		PrintStructureMember(pV->FormCode, L"FormCode", Indent, Flags);
		PrintStructureMember(pV->NameLength, L"NameLength", Indent, Flags);
		PrintStructureMember(pV->NameOffset, L"NameOffset", Indent, Flags);
		PrintStructureMember(pV->Flags, L"Flags", Indent, Flags);
		PrintStructureMember(pV->Instance, L"Instance", Indent, Flags);

	switch (pV->FormCode)
	{
	case RESIDENT_FORM:
		PrintStructureMember(pV->Form, L"Resident", Indent, Flags | FL_UNDECORATED);

		Indent += IDX_STEP;

			PrintStructureMember(pV->Form.Resident.ValueLength, L"ValueLength", Indent, Flags);
			PrintStructureMember(pV->Form.Resident.ValueOffset, L"ValueOffset", Indent, Flags);
			PrintStructureMember(pV->Form.Resident.ResidentFlags, L"ResidentFlags", Indent, Flags);
			PrintStructureMember(pV->Form.Resident.Reserved, L"Reserved", Indent, Flags);

		Indent -= IDX_STEP;
		
		PrintAnyString(Indent, Flags, L"}");

		break;
	case NONRESIDENT_FORM:
		PrintStructureMember(pV->Form, L"Nonresident", Indent, Flags | FL_UNDECORATED);

		Indent += IDX_STEP;

		PrintStructureMember(pV->Form.Nonresident.LowestVcn, L"LowestVcn", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.HighestVcn, L"HighestVcn", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.MappingPairsOffset, L"MappingPairsOffset", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.CompressionUnit, L"CompressionUnit", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.Reserved, L"Reserved", Indent, Flags | FL_NOTYPE);
		PrintStructureMember(pV->Form.Nonresident.AllocatedLength, L"AllocatedLength", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.FileSize, L"FileSize", Indent, Flags);
		PrintStructureMember(pV->Form.Nonresident.ValidDataLength, L"ValidDataLength", Indent, Flags);
		if (pV->Form.Nonresident.CompressionUnit)
		{
			PrintStructureMember(pV->Form.Nonresident.TotalAllocated, L"TotalAllocated", Indent, Flags);
		}

		Indent -= IDX_STEP;
		PrintAnyString(Indent, Flags, L"}");
		break;
	}

	Indent -= IDX_STEP;

	PrintAnyString(Indent, Flags, L"}");
	
}

VOID PrintStandardInformation(RW_BUFFER * pBuffer, WCHAR * szFieldName, USHORT Indent, ULONG Flags)
{
	PSTANDARD_INFORMATION pV = (PSTANDARD_INFORMATION) pBuffer->_pBuffer;
	
	PrintStructureMember(*pV, szFieldName, Indent, Flags);

	Indent += IDX_STEP;
		PrintFiletime(pV->CreationTime, L"CreationTime", Indent, Flags);
		PrintFiletime(pV->LastModificationTime, L"LastModificationTime", Indent, Flags);
		PrintFiletime(pV->LastChangeTime, L"LastChangeTime", Indent, Flags);
		PrintFiletime(pV->LastAccessTime, L"LastAccessTime", Indent, Flags);
		PrintStructureMember(pV->FileAttributes, L"FileAttributes", Indent, Flags);
		PrintStructureMember(pV->MaximumVersions, L"MaximumVersions", Indent, Flags);
		PrintStructureMember(pV->VersionNumber, L"VersionNumber", Indent, Flags);
		PrintStructureMember(pV->ClassId, L"ClassId", Indent, Flags);
		PrintStructureMember(pV->OwnerId, L"OwnerId", Indent, Flags);
		PrintStructureMember(pV->SecurityId, L"SecurityId", Indent, Flags);
		PrintStructureMember(pV->QuotaCharged, L"QuotaCharged", Indent, Flags);
		PrintStructureMember(pV->Usn, L"Usn", Indent, Flags);
	Indent -= IDX_STEP;

	PrintAnyString(Indent, Flags, L"}");

}

VOID PrintDuplicatedInformation(PDUPLICATED_INFORMATION pV, WCHAR * szFieldName, USHORT Indent, ULONG Flags)
{
		PrintStructureMember(*pV, szFieldName, Indent, Flags);
		Indent += IDX_STEP;
			PrintFiletime(pV->CreationTime, L"CreationTime", Indent, Flags);	
			PrintFiletime(pV->LastModificationTime, L"LastModificationTime", Indent, Flags);	
			PrintFiletime(pV->LastChangeTime, L"LastChangeTime", Indent, Flags);	
			PrintFiletime(pV->LastAccessTime, L"LastAccessTime", Indent, Flags);	
			PrintStructureMember(pV->AllocatedLength, L"AllocatedLength", Indent, Flags);	
			PrintStructureMember(pV->FileSize, L"FileSize", Indent, Flags);	
			PrintStructureMember(pV->FileAttributes, L"FileAttributes", Indent, Flags);	
			PrintStructureMember(pV->PackedEaSize, L"PackedEaSize", Indent, Flags | FL_UNDECORATED);
			PrintStructureMember(pV->ReparsePointTag, L"ReparsePointTag", Indent, Flags);	
		Indent -= IDX_STEP;
		PrintAnyString(Indent, Flags, L"}");
}

VOID PrintFilenameInformation(MFT_SEGMENT_REFERENCE Frs, RW_BUFFER * pBuffer, WCHAR * szFieldName, USHORT Indent, ULONG Flags)
{
	PFILE_NAME pV = (PFILE_NAME) pBuffer->_pBuffer;
	WCHAR * szHyperlink = new WCHAR[MAX_PATH];
	
	if (pV->Info.FileAttributes & DUP_FILE_NAME_INDEX_PRESENT)
	{
		wsprintf(szHyperlink, L"<\\\\DIR:%08I64x>", NtfsFullSegmentNumber(&Frs));
	}
	else
	{
		wsprintf(szHyperlink, L"<\\\\FRS:%08I64x>", NtfsFullSegmentNumber(&Frs));
	}

	if (NtfsFullSegmentNumber(&Frs)==0)
	{
		szHyperlink[0] = L'\0';
	}

	PrintStructureMember(*pV, szFieldName, Indent, Flags);
	Indent += IDX_STEP;
		PrintFileReference(&pV->ParentDirectory, L"ParentDirectory", Indent, Flags);
		PrintDuplicatedInformation(&pV->Info, L"Info", Indent,Flags);
		PrintStructureMember(pV->FileNameLength, L"FileNameLength", Indent, Flags);	
		PrintStructureMember(pV->Flags, L"Flags", Indent, Flags);	
		PrintStructureMember(pV->FileName, L"FileName", Indent, Flags | FL_NOTYPE | FL_NOCRLF);
		
		WCHAR * szName = CreateNameString(pV->FileName, pV->FileNameLength);
		PrintAnyString(2, Flags, L"\"%s\"  (%s)",szName, szHyperlink);
		delete [] szName;

	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");

	delete [] szHyperlink;
}

/*
BOOL PrintIfMatchingFilename(RW_BUFFER * pBuffer, WCHAR * szFileName, USHORT Indent, ULONG Flags)
{
	BOOL bReturn = FALSE;
	PFILE_NAME pV = (PFILE_NAME) pBuffer->_pBuffer;
	WCHAR * szName = CreateNameString(pV->FileName, pV->FileNameLength);
	if (szName)
	{
		switch (CompareType)
		{
		case Insensitive:
			if (!wcsnicmp(szNameToMatch, szName, pV->FileNameLength)) bReturn = TRUE;
			break;
		case CaseSensitive:
			if (!wcsncmp(szNameToMatch, szName, pV->FileNameLength)) bReturn = TRUE;
			break;
		case Partial:
			break;
		delete [] szName;
	}

	return bReturn;
}
*/
VOID PrintExtents(PATTRIBUTE_RECORD_HEADER pV, USHORT Indent, ULONG Flags)
{
	MAPPING_PAIRS mp;
	DWORD Error;
	
	Error=mp.AppendRunsToArray(pV);
	if (Error)
	{
		PrintAnyString(Indent, Flags, L"Error - invalid extents.");
		return;
	}

	PrintAnyString(Indent, Flags, L"Extents  {");
	
	Indent += IDX_STEP;

	for (LONG i=0;i<mp._NumberOfRuns;i++)
	{
		if (mp._pMapArray[i].Allocated)
		{
			if (IS_FLAG_SET(Flags, FL_BASE10))
			{
				PrintAnyString(
					Indent, 
					Flags, 
					L"VCN = %I64u LEN = %I64u : LCN = %I64u     <\\\\LCN:%I64x:%lx>", 
					mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].CurrentLcn,
					mp._pMapArray[i].CurrentLcn, 
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn);
			}
			else
			{
				PrintAnyString(
					Indent, 
					Flags, 
					L"VCN = 0x%I64x LEN = 0x%I64x : LCN = 0x%I64x    <\\\\LCN:%I64x:%lx>",
					mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].CurrentLcn,
					mp._pMapArray[i].CurrentLcn,
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn);
			}
		}
		else
		{
			if (IS_FLAG_SET(Flags, FL_BASE10))
			{
				PrintAnyString(
					Indent, 
					Flags, 
					L"VCN = %I64u LEN = %I64u",
					mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn);
			}
			else
			{
				PrintAnyString(
					Indent, 
					Flags, 
					L"VCN = 0x%I64x LEN = 0x%I64x",
					mp._pMapArray[i].CurrentVcn,
					mp._pMapArray[i].NextVcn - mp._pMapArray[i].CurrentVcn);
			}
		}
	}

	Indent -= IDX_STEP;

	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintIndexEntryStructure(PINDEX_ENTRY pV, BOOL FileReference, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
		if (pV->ReservedForZero != 0)
		{
			PrintFileReference(&pV->FileReference, L"FileReference", Indent, Flags);
		}
		else
		{
			PrintStructureMember(pV->DataOffset, L"DataOffset", Indent, Flags);
			PrintStructureMember(pV->DataLength, L"DataLength", Indent, Flags);
			PrintStructureMember(pV->ReservedForZero, L"ReservedForZero", Indent, Flags);
		}
		PrintStructureMember(pV->Length, L"Length", Indent, Flags);
		PrintStructureMember(pV->AttributeLength, L"AttributeLength", Indent, Flags);
		PrintStructureMember(pV->Flags, L"Flags", Indent, Flags);
		PrintStructureMember(pV->Reserved, L"Reserved", Indent, Flags | FL_NOTYPE);
	Indent -= IDX_STEP;

	if (!IS_FLAG_SET(Flags, FL_NOCLOSINGBRACKET))
	{
		PrintAnyString(Indent, Flags, L"}");
	}

}

VOID PrintIndexEntryFilenameAbbreviated(RW_BUFFER * pBuffer, PWCHAR szAttributeName, USHORT Indent, ULONG Flags)
{
	RW_BUFFER IdxData;

	if (!wcscmp(szAttributeName, L"$I30"))
	{
		PINDEX_ENTRY pEntry = (PINDEX_ENTRY)pBuffer->_pBuffer;
		PFILE_NAME   pF = (PFILE_NAME)ADD_PTR(pEntry, sizeof(INDEX_ENTRY));
		WCHAR *		 szFilename = CreateNameString(pF->FileName, pF->FileNameLength);
		VCN * pDownPointerVcn = (VCN *)ADD_PTR(pEntry, pEntry->Length - sizeof(VCN));
		WCHAR szFrsNumber[MAX_PATH];
		BOOL bIsBranchNode = IS_FLAG_SET(pEntry->Flags, INDEX_NODE);

		if (IS_FLAG_SET(pEntry->Flags, INDEX_ENTRY_END))
		{
			szFilename = CreateNameString(L"$INDEX_END", wcslen(L"$INDEX_END"));
			wcscpy(szFrsNumber, L"                         ");
		}
		else
		{
			wsprintf(szFrsNumber, L"<\\\\FRS:%08I64x>,SEQ %4x", NtfsFullSegmentNumber(&pEntry->FileReference), pEntry->FileReference.SequenceNumber);
		}

		if (bIsBranchNode)
		{
			if (IS_FLAG_SET(pF->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT))
			{
				PrintAnyString(
					Indent,
					Flags,
					L" %s <\\\\DIR:%08I64x> %33s  DP = VCN 0x%I64x",
					szFrsNumber,
					NtfsFullSegmentNumber(&pEntry->FileReference),
					szFilename,
					*pDownPointerVcn);
			}
			else
			{
				PrintAnyString(
					Indent,
					Flags,
					L" %s                  %33s  DP = VCN 0x%I64x",
					szFrsNumber,
					szFilename,
					*pDownPointerVcn);
			}
		}
		else // Leaf node.
		{
			if (IS_FLAG_SET(pF->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT))
			{
				PrintAnyString(
					Indent,
					Flags,
					L" %s <\\\\DIR:%08I64x> %33s",
					szFrsNumber,
					NtfsFullSegmentNumber(&pEntry->FileReference),
					szFilename);
			}
			else
			{
				PrintAnyString(
					Indent,
					Flags,
					L" %s                  %33s",
					szFrsNumber,
					szFilename);
			}
		}
		delete[] szFilename;
	}

}



VOID PrintIndexEntryByType(RW_BUFFER * pBuffer, PWCHAR szAttributeName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	RW_BUFFER IdxData;
	
	PINDEX_ENTRY pEntry = (PINDEX_ENTRY) pBuffer->_pBuffer;

	Error = IdxData.BUFF_CopyStdBuffer(ADD_PTR(pEntry,sizeof(INDEX_ENTRY)), pEntry->Length);
	if (Error) 
	{
		TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
		return;
	}
	PrintAnyString(Indent, Flags, L"");

	if (!wcscmp(szAttributeName, L"$I30"))
	{
		PrintIndexEntryStructure(pEntry, TRUE, Indent, Flags | FL_NOCLOSINGBRACKET);
		if (!IS_FLAG_SET(pEntry->Flags, INDEX_ENTRY_END))
		{
			PrintFilenameInformation(pEntry->FileReference, &IdxData, L"", Indent + 4, Flags);
		}
		PrintAnyString(Indent, Flags, L"}");
	}

	if (!wcscmp(szAttributeName, L"$SII"))
	{
		PrintIndexEntryStructure(pEntry, FALSE, Indent, Flags | FL_NOCLOSINGBRACKET);
		PSII_ENTRY pV = (PSII_ENTRY) ADD_PTR(pEntry, sizeof(INDEX_ENTRY));
		PrintSiiEntry(&IdxData, Indent+4, Flags);
		PrintAnyString(Indent, Flags, L"}");
	}

	if (!wcscmp(szAttributeName, L"$O"))
	{
		PrintIndexEntryStructure(pEntry, FALSE, Indent, Flags | FL_NOCLOSINGBRACKET);
		POBJID_INDEX_ENTRY_VALUE pV = (POBJID_INDEX_ENTRY_VALUE) ADD_PTR(pEntry, sizeof(INDEX_ENTRY));
		PrintObjectIdIndexValue(pV, L"", Indent+4, Flags);
	}

	if (!wcscmp(szAttributeName, L"$SDH"))
	{
		PrintIndexEntryStructure(pEntry, FALSE, Indent, Flags | FL_NOCLOSINGBRACKET);
		PrintSdhEntry(&IdxData, Indent+4, Flags);
		PrintAnyString(Indent, Flags, L"}");
	}

	if (!wcscmp(szAttributeName, L"$R"))
	{
		PrintIndexEntryStructure(pEntry, TRUE, Indent, Flags | FL_NOCLOSINGBRACKET);
		PrintReparseIndexKey(&IdxData, Indent+4, Flags);
		PrintAnyString(Indent, Flags, L"}");
	}

	if (pEntry->Flags & INDEX_NODE)
	{
		VCN * pDownPointerVcn = (VCN *)ADD_PTR(pEntry, pEntry->Length - sizeof(VCN));
		PrintStructureMember(*pDownPointerVcn, L"DownPointerVcn", Indent, Flags);
	}

}

VOID PrintObjectIdIndexValue(POBJID_INDEX_ENTRY_VALUE pV, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	HEX_DUMP h;
	h.DirectOutput(gOutput);
	RW_BUFFER e;
	DWORD Error;
	PrintStructureMember(*pV, L"", Indent, Flags);		
	Indent += IDX_STEP;
	PrintGuid(Indent, Flags, &pV->key, L"key");
	PrintFileReference(&pV->SegmentReference, L"SegmentReference", Indent, Flags);		
	PrintAnyString(Indent, Flags, L"CHAR extraInfo[] {");

	Error = e.BUFF_CopyStdBuffer(pV->extraInfo, 48);
	if (Error) 
	{
		TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
		return;
	}
	Indent += IDX_STEP;
	if (!e.BUFF_IsAllZeroes())
	{
		h.HEX_DumpHexData(&e,Indent, 8);
	}
	else
	{
		PrintAnyString(Indent+IDX_STEP, Flags, L"...");
	}


	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");

	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}


VOID PrintIndexHeader(PINDEX_HEADER pV, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->FirstIndexEntry, L"FirstIndexEntry", Indent, Flags);
	PrintStructureMember(pV->FirstFreeByte, L"FirstFreeByte", Indent, Flags);
	PrintStructureMember(pV->BytesAvailable, L"BytesAvailable", Indent, Flags);
	PrintStructureMember(pV->Flags, L"Flags", Indent, Flags);
	PrintStructureMember(pV->Reserved, L"Reserved", Indent, Flags | FL_NOTYPE);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintIndexRoot(PINDEX_ROOT pV, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->IndexedAttributeType, L"IndexedAttributeType", Indent, Flags);
	PrintStructureMember(pV->CollationRule, L"CollationRule", Indent, Flags);
	PrintStructureMember(pV->BytesPerIndexBuffer, L"BytesPerIndexBuffer", Indent, Flags);
	PrintIndexHeader(&pV->IndexHeader, L"IndexHeader", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintSdhEntry(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
	PSDH_ENTRY pV = (PSDH_ENTRY) pBuffer->_pBuffer;
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintSecurityHashKey(&pV->Hash, L"Hash", Indent, Flags);
	PrintSecurityDescriptorHeader(&pV->Header, L"Header", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintSiiEntry(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
	PSII_ENTRY pV = (PSII_ENTRY) pBuffer->_pBuffer;
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->SecuirtyId, L"SecuirtyId", Indent, Flags);
	PrintSecurityDescriptorHeader(&pV->Header, L"Header", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
	PrintAnyString(Indent, Flags, L"");
}

VOID PrintSecurityHashKey(PSECURITY_HASH_KEY pV, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	
	PrintStructureMember(*pV, szName, Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->Hash, L"Hash", Indent, Flags);
	PrintStructureMember(pV->SecurityId, L"SecurityId", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintSecurityDescriptorHeader(PSECURITY_DESCRIPTOR_HEADER  pV, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(*pV, szName, Indent, Flags);
	Indent += IDX_STEP;
	PrintSecurityHashKey((PSECURITY_HASH_KEY)&pV->HashKey, L"HashKey", Indent, Flags);
	PrintStructureMember(pV->Length, L"Length", Indent, Flags);
	PrintStructureMember(pV->Offset, L"Offset", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintReparseIndexKey(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
	PREPARSE_INDEX_KEY pV = (PREPARSE_INDEX_KEY) pBuffer->_pBuffer;
	
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->FileReparseTag, L"FileReparseTag", Indent, Flags);
	PrintFileReference(&pV->FileId, L"FileId", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintIndexAllocationBufferStructure(PINDEX_ALLOCATION_BUFFER pV, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintMultiSectorHeader(&pV->MultiSectorHeader, L"MultiSectorHeader", Indent, Flags);
	PrintStructureMember(pV->Lsn.QuadPart, L"Lsn", Indent, Flags);
	PrintStructureMember(pV->ThisBlock, L"ThisBlock", Indent, Flags);
	PrintIndexHeader(&pV->IndexHeader, L"", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintIndexBuffer(MFT_SEGMENT_REFERENCE ParentFrs, RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	PINDEX_ROOT pV = (PINDEX_ROOT) pBuffer->_pBuffer;
	PINDEX_ENTRY pEntry = NtfsFirstIndexEntry(&pV->IndexHeader);
	PINDEX_ENTRY pMax	= (PINDEX_ENTRY) ADD_PTR(pBuffer->_pBuffer, pBuffer->_AllocatedLength);

	if (!IS_FLAG_SET(Flags, FL_TERSE_INDEX)) 
	{		
		PrintIndexRoot(pV, Indent, Flags);
	}
	else
	{		
		PrintAnyString(0, Flags, L"==================================================================");
		PrintAnyString(0, Flags, L"$INDEX_ROOT (%s)", (IS_FLAG_SET(pEntry->Flags, INDEX_NODE)) ? L"BRANCH_NODE" : L"LEAF_NODE");
		PrintAnyString(0, Flags, L"==================================================================");
	}

	while (pEntry < pMax)
	{
		RW_BUFFER IndexEntryBuffer;
		Error = IndexEntryBuffer.BUFF_CopyStdBuffer((PVOID)pEntry,pEntry->Length);
		if (Error) 
		{
			TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
			return;
		}

		if (IS_FLAG_SET(Flags, FL_TERSE_INDEX))	
		{
			PrintIndexEntryFilenameAbbreviated(&IndexEntryBuffer, szName, Indent, Flags);
		}
		else									
		{
			PrintIndexEntryByType(&IndexEntryBuffer, szName, Indent, Flags);
			PrintAnyString(Indent, Flags, L"");
		}


		pEntry = NtfsNextIndexEntry(pEntry);
	}
}

VOID PrintFiletime(LONGLONG Time, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	SYSTEMTIME st, lt;
	
	PrintStructureMember(Time, szName, Indent, Flags | FL_NOCRLF);

	// Convert file time to system time.
	if (FileTimeToSystemTime((PFILETIME)&Time, &st))
	{
		TIME_ZONE_INFORMATION tzi;	
		if (GetTimeZoneInformation(&tzi) != TIME_ZONE_ID_INVALID && 
			SystemTimeToTzSpecificLocalTime(&tzi, &st, &lt)
			&& (!IS_FLAG_SET(Flags, FL_UTC_TIME)))
		{
			PrintAnyString(1, Flags, L"%02i/%02i/%04i LCL %02i:%02i:%02i.%03i",	lt.wMonth, lt.wDay,	lt.wYear, lt.wHour,	lt.wMinute,	lt.wSecond,	lt.wMilliseconds);
		}
		else
		{
			PrintAnyString(1, Flags, L"%02i/%02i/%04i UTC %02i:%02i:%02i.%03i",	st.wMonth, st.wDay,	st.wYear, st.wHour, st.wMinute, st.wSecond,	st.wMilliseconds);
		}
	}
	else 
	{
		PrintAnyString(Indent, Flags, L"");
	}
}

VOID PrintIndexAllocationBuffer(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	PINDEX_ALLOCATION_BUFFER pV = (PINDEX_ALLOCATION_BUFFER) pBuffer->_pBuffer;
	DWORD BufferLength			= (DWORD) pBuffer->_NumberOfBytes;
	LONG BlocksPerIab			= (LONG) pV->MultiSectorHeader.SequenceArraySize - 1;
	LONG BytesPerIab			= BlocksPerIab * USA_SECTOR_SIZE;
	LONG NumberOfBuffers		= BufferLength / BytesPerIab;

	for (LONG i=0 ; i < NumberOfBuffers ; i++)
	{
		PINDEX_ALLOCATION_BUFFER pThisBuffer = (PINDEX_ALLOCATION_BUFFER)ADD_PTR(pV, (i * BytesPerIab));

		PINDEX_ENTRY pEntry = NtfsFirstIndexEntry(&pThisBuffer->IndexHeader);
		PINDEX_ENTRY pMax	= (PINDEX_ENTRY) ADD_PTR(pThisBuffer, pThisBuffer->IndexHeader.BytesAvailable);

		BOOL bIsBranchNode = IS_FLAG_SET(pEntry->Flags, INDEX_NODE);
		PrintAnyString(0, Flags, L"==================================================================");
		PrintAnyString(0, Flags, L"VCN 0x%I64x (%s)", pThisBuffer->ThisBlock, (bIsBranchNode) ? L"BRANCH_NODE" : L"LEAF_NODE");
		PrintAnyString(0, Flags, L"==================================================================");

		if (!IS_FLAG_SET(Flags, FL_TERSE_INDEX)) 
		{
			PrintAnyString(Indent, Flags, L"");
			PrintIndexAllocationBufferStructure(pV, Indent, Flags);	
		}
		
		while (pEntry < pMax && pEntry->Length)
		{
			RW_BUFFER IndexEntryBuffer;

			Error = IndexEntryBuffer.BUFF_CopyStdBuffer((PVOID)pEntry,pEntry->Length);
			if (Error) 
			{
				TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
				return;
			}

			if (IS_FLAG_SET(Flags, FL_TERSE_INDEX))	
			{
				PrintIndexEntryFilenameAbbreviated(&IndexEntryBuffer, szName, Indent, Flags);
			}
			else									
			{
				PrintIndexEntryByType(&IndexEntryBuffer, szName, Indent, Flags);
			}

			if (IS_FLAG_SET(pEntry->Flags, INDEX_ENTRY_END))
			{
				break;
			}

			pEntry = NtfsNextIndexEntry(pEntry);
		}
	}
}

VOID FormatAccessMaskText(ACCESS_MASK Mask, WCHAR * szAccess)
{

#ifdef DEBUG_SD
    wsprintf(szAccess,L"Mask : 0x%08x\n", Mask);
	if (IS_FLAG_SET(Mask, MAXIMUM_ALLOWED         )) wcscat(szAccess,L"\n\t\n\tMAXIMUM_ALLOWED ");
    if (IS_FLAG_SET(Mask, ACCESS_SYSTEM_SECURITY  )) wcscat(szAccess,L"\n\tACCESS_SYSTEM_SECURITY ");
    if (IS_FLAG_SET(Mask, DELETE                  )) wcscat(szAccess,L"\n\tDELETE ");
    if (IS_FLAG_SET(Mask, READ_CONTROL            )) wcscat(szAccess,L"\n\tREAD_CONTROL ");
    if (IS_FLAG_SET(Mask, WRITE_DAC               )) wcscat(szAccess,L"\n\tWRITE_DAC ");
    if (IS_FLAG_SET(Mask, WRITE_OWNER             )) wcscat(szAccess,L"\n\tWRITE_OWNER ");
    if (IS_FLAG_SET(Mask, SYNCHRONIZE             )) wcscat(szAccess,L"\n\tSYNCHRONIZE ");
    if (IS_FLAG_SET(Mask, STANDARD_RIGHTS_REQUIRED)) wcscat(szAccess,L"\n\tSTANDARD_RIGHTS_REQUIRED ");
    if (IS_FLAG_SET(Mask, STANDARD_RIGHTS_READ    )) wcscat(szAccess,L"\n\tSTANDARD_RIGHTS_READ ");
    if (IS_FLAG_SET(Mask, STANDARD_RIGHTS_WRITE   )) wcscat(szAccess,L"\n\tSTANDARD_RIGHTS_WRITE ");
    if (IS_FLAG_SET(Mask, STANDARD_RIGHTS_EXECUTE )) wcscat(szAccess,L"\n\tSTANDARD_RIGHTS_EXECUTE ");
    if (IS_FLAG_SET(Mask, STANDARD_RIGHTS_ALL     )) wcscat(szAccess,L"\n\tSTANDARD_RIGHTS_ALL ");
    if (IS_FLAG_SET(Mask, SPECIFIC_RIGHTS_ALL     )) wcscat(szAccess,L"\n\tSPECIFIC_RIGHTS_ALL ");
    if (IS_FLAG_SET(Mask, GENERIC_READ            )) wcscat(szAccess,L"\n\tGENERIC_READ ");
    if (IS_FLAG_SET(Mask, GENERIC_WRITE           )) wcscat(szAccess,L"\n\tGENERIC_WRITE ");
    if (IS_FLAG_SET(Mask, GENERIC_EXECUTE         )) wcscat(szAccess,L"\n\tGENERIC_EXECUTE ");
    if (IS_FLAG_SET(Mask, GENERIC_ALL             )) wcscat(szAccess,L"\n\tGENERIC_ALL ");
#else 
	wsprintf(szAccess,L"MASK : 0x%08x", Mask);
#endif
    wcscat(szAccess,L"\n");
}

VOID PrintAcl(PACL pDacl, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
    WCHAR szTextualSid[MAX_PATH], szAccess[MAX_PATH];
    DWORD TextualSidSize;
    PACE_HEADER pThisAce;
    PSID pThisSid;
#ifdef DEBUG_SD
	HEX_DUMP h;
	h.DirectOutput(gOutput);
#endif
    RW_BUFFER b;
		
	PrintStructureMember(*pDacl, szName, Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pDacl->AclRevision,L"AclRevision", Indent,Flags);
	PrintStructureMember(pDacl->Sbz1,L"Sbz1", Indent, Flags);
	PrintStructureMember(pDacl->AclSize,L"AclSize", Indent, Flags);
	PrintStructureMember(pDacl->AceCount,L"AceCount", Indent, Flags);
	PrintStructureMember(pDacl->Sbz2,L"Sbz2", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent,Flags,L"}");

	Error = b.BUFF_CopyStdBuffer((PVOID)pDacl,sizeof(_ACL));
	if (Error) 
	{
		TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
		return;
	}
#ifdef DEBUG_SD
	h.HEX_DumpHexData(&b,2);
#endif

	Indent += IDX_STEP;
    
    for (ULONG i=0; i < pDacl->AceCount; i++)
    {                                
        if (GetAce(pDacl,i,(LPVOID*)&pThisAce))
        {
            switch (pThisAce->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                {
                    PACCESS_ALLOWED_ACE pAce = (PACCESS_ALLOWED_ACE) pThisAce;
                    pThisSid = (PSID) (&pAce->SidStart );
                                                                                
                    // Print this sid.
                    TextualSidSize=MAX_PATH;
                    if (GetTextualSid(pThisSid, szTextualSid, &TextualSidSize))
                    {
                        FormatAccessMaskText(pAce->Mask,szAccess);
                        PrintAnyString(Indent, Flags, L"ALLOWED : %s ", szTextualSid);
                        PrintAnyString(Indent, Flags, L"   %s", szAccess );
                    }
                    else
                    {
                        PrintAnyString(Indent, Flags, L"Invalid ACL");
                    }

                }
                break;
            case ACCESS_DENIED_ACE_TYPE:
                {
                    PACCESS_DENIED_ACE pAce = (PACCESS_DENIED_ACE) pThisAce;
                    pThisSid = (PSID) (&pAce->SidStart );
                                        
                    TextualSidSize=MAX_PATH;
                    if (GetTextualSid(pThisSid, szTextualSid, &TextualSidSize))
                    {
                        FormatAccessMaskText(pAce->Mask,szAccess);
                        PrintAnyString(Indent, Flags, L"DENIED : %s ", szTextualSid);
                        PrintAnyString(Indent, Flags, L"  %s ", szAccess );
                    }
                    else
                    {
                        PrintAnyString(Indent, Flags, L"    Invalid ACL");
                    }

                }
                break;
            }

			Error = b.BUFF_CopyStdBuffer((PVOID)pThisAce,pThisAce->AceSize);
			if (Error) 
			{
				TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
				return;
			}
#ifdef DEBUG_SD
			h.HEX_DumpHexData(&b,4);
#endif

        }

    }	
}

BOOL GetTextualSid(PSID pSid, LPWSTR szTextualSid, LPDWORD dwBufferLen)
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev = SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    DWORD DomainNameSize  = MAX_PATH;
    DWORD AccountNameSize = MAX_PATH;
    WCHAR szAccountName[MAX_PATH];
    WCHAR szDomainName[MAX_PATH];
    WCHAR szTempString[MAX_PATH];
    SID_NAME_USE Use;

    // Test if SID passed in is valid.
    if(!IsValidSid(pSid)) 
        return FALSE;

    AccountNameSize = MAX_PATH;
    DomainNameSize  = MAX_PATH;

    if (!LookupAccountSid(NULL,pSid,szAccountName,&AccountNameSize,szDomainName,&DomainNameSize,&Use))
    {
        wcscpy(szDomainName,L"");
        wcscpy(szAccountName,L"");
        DomainNameSize = 1;
        AccountNameSize = 1;
    }

    // Obtain SidIdentifierAuthority.
    psia = GetSidIdentifierAuthority(pSid);

    // Obtain sidsubauthority count.
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    // Compute buffer length.
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    // Check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    if (*dwBufferLen < dwSidSize) {

    *dwBufferLen = dwSidSize;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
    }

    // Prepare S-SID_REVISION-.
    dwSidSize = wsprintf(szTextualSid, TEXT("S-%lu-"), dwSidRev);

    // Prepare SidIdentifierAuthority.
    if ((psia->Value[0] != 0) || (psia->Value[1] != 0)) {

        dwSidSize += wsprintf(szTextualSid + lstrlen(szTextualSid),
            TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
            (USHORT) psia->Value[0],
            (USHORT) psia->Value[1],
            (USHORT) psia->Value[2],
            (USHORT) psia->Value[3],
            (USHORT) psia->Value[4],
            (USHORT) psia->Value[5]);
    
    } else {
    
        dwSidSize += wsprintf(szTextualSid + lstrlen(szTextualSid),
            TEXT("%lu"),
            (ULONG) (psia->Value[5]      ) +
            (ULONG) (psia->Value[4] <<  8) +
            (ULONG) (psia->Value[3] << 16) +
            (ULONG) (psia->Value[2] << 24));
    }

    // Loop through SidSubAuthorities.
    for (dwCounter = 0; dwCounter < dwSubAuthorities; dwCounter++) {

        dwSidSize += wsprintf(szTextualSid + dwSidSize, TEXT("-%lu"),
            *GetSidSubAuthority(pSid, dwCounter));
    }

    if (AccountNameSize>1 && DomainNameSize>1)
    {
        wsprintf(szTempString,L"%s\\%s (%s)", szDomainName, szAccountName, szTextualSid);
    }
    else
    {
        wsprintf(szTempString,L"%s", szTextualSid);
    }

    wcscpy(szTextualSid, szTempString);
    *dwBufferLen = (DWORD) wcslen(szTextualSid);
    return TRUE;
}

DWORD PrintObjectSecurity(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
    WCHAR szTextualSid[MAX_PATH];
    ULONG SidStringLen = MAX_PATH;

    PSID psidOwner = NULL;
    PSID psidGroup = NULL;
    PACL pDacl     = NULL;
    PACL pSacl     = NULL;

	RW_BUFFER Buffer;
	DWORD dwSidSize=sizeof(SID);
	
	SECURITY_DESCRIPTOR_RELATIVE * pSD = (SECURITY_DESCRIPTOR_RELATIVE*) pBuffer->_pBuffer;



	PrintAnyString(Indent, Flags, L"SECURITY_DESCRIPTOR {");
	
	if (!IS_FLAG_SET(pSD->Control,SE_SELF_RELATIVE) || pSD->Revision !=SECURITY_DESCRIPTOR_REVISION)	
	{
		PrintAnyString(Indent+IDX_STEP, Flags, L"*****ERROR_INVALID_SECURITY_DESCR*******");
		PrintAnyString(Indent, Flags, L"}");
		return ERROR_INVALID_SECURITY_DESCR;
	}
		
	if (pSD->Dacl)  pDacl = (PACL) ADD_PTR(pSD, pSD->Dacl);
	if (pSD->Sacl)	pSacl = (PACL) ADD_PTR(pSD, pSD->Sacl);
	if (pSD->Owner)	psidOwner = (PSID) ADD_PTR(pSD, pSD->Owner);
	if (pSD->Group)	psidGroup = (PSID) ADD_PTR(pSD, pSD->Group);
        
#ifdef DEBUG_SD
    DWORD Error;
	if (pDacl)
    {
        PrintAnyString(Indent, Flags, L"COMPLETE DACL HEX DUMP\n");
        PrintAnyString(Indent, Flags, L"======================\n");

        Error = Buffer.BUFF_CopyStdBuffer((PVOID) pDacl, pDacl->AclSize);
		if (Error)
		{
			return Error;
		}
        h.HEX_DumpHexData(&Buffer,2);
    }
    
    if (pSacl)
    {
        PrintAnyString(Indent, Flags, L"COMPLETE SACL HEX DUMP\n");
        PrintAnyString(Indent, Flags, L"======================\n");
        Error = Buffer.BUFF_CopyStdBuffer((PVOID) pDacl, pSacl->AclSize);
		if (Error)
		{
			return Error;
		}
        h.HEX_DumpHexData(&Buffer,2);
    }
#endif

    //Print owner information.

	Indent += IDX_STEP;

	SidStringLen=MAX_PATH;
    GetTextualSid(psidOwner, szTextualSid, &SidStringLen);
	PrintAnyString(Indent, Flags, L"Owner:  %s", szTextualSid);

#ifdef DEBUG_SD
	if (psidOwner)
	{
		Error = Buffer.BUFF_CopyStdBuffer((PVOID) psidOwner, sizeof(SID));
		if (Error)
		{
			return Error;
		}
		h.HEX_DumpHexData(&Buffer,2);
	}
#endif

    //Print group information.
    SidStringLen=MAX_PATH;
    GetTextualSid(psidGroup, szTextualSid, &SidStringLen);
    PrintAnyString(Indent, Flags, L"Group:  %s", szTextualSid);

#ifdef DEBUG_SD    
	if (psidGroup)
	{
        Error = Buffer.BUFF_CopyStdBuffer((PVOID) psidGroup, sizeof(SID));
		if (Error)
		{
			return Error;
		}
		h.HEX_DumpHexData(&Buffer,2);
	} 
#endif

	if (pSacl)
    {
        PrintAcl(pSacl, L"SACL",Indent,Flags);
#ifdef DEBUG_SD
        Error = Buffer.BUFF_CopyStdBuffer((PVOID)pSacl,pSacl->AclSize);
		if (Error)
		{
			return Error;
		}
		h.HEX_DumpHexData(&Buffer,2);
#endif
		
    }

    if (pDacl)
    {
        PrintAcl(pDacl, L"DACL",Indent,Flags);
#ifdef DEBUG_SD
        Error = Buffer.BUFF_CopyStdBuffer((PVOID)pDacl, pDacl->AclSize);
		if (Error)
		{
			return Error;
		}
		h.HEX_DumpHexData(&Buffer,2);
#endif
    }
	
	Indent -= IDX_STEP;

	PrintAnyString(Indent, Flags, L"}");
    
	return NO_ERROR;
}        

VOID PrintSecurityStream(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	HEX_DUMP h;
	h.DirectOutput(gOutput);
	PSECURITY_DESCRIPTOR_HEADER pSecurityHeader;
	PSECURITY_DESCRIPTOR_HEADER pMax = (PSECURITY_DESCRIPTOR_HEADER) ADD_PTR(pBuffer->_pBuffer,pBuffer->_NumberOfBytes);
	BOOL bExit=FALSE;

	// Pointer calculation for each mapping buffer.
	pSecurityHeader = (PSECURITY_DESCRIPTOR_HEADER) pBuffer->_pBuffer;
	ULONG MappingOffset=0;
	ULONG MappingBufferNumber = 0;

	// Work through each chunk.
	while ((pSecurityHeader < pMax))
	{
		// Do some bounds checking on the SD.
		if (ADD_PTR(pSecurityHeader,pSecurityHeader->Length) > (PCHAR) pMax) 
		{
			break;
		}
		// If this is an "in use" SD, then print it.
		if (pSecurityHeader->HashKey.SecurityId)
		{	
			// Print the SDS header structure.
			LONGLONG TrueSdsOffset = (LONGLONG) ((PCHAR)pSecurityHeader - pBuffer->_pBuffer);
			if (TrueSdsOffset == 0x00000000000d2e40)
			{
				TrueSdsOffset ++;
				TrueSdsOffset --;
			}

			PrintAnyString(Indent,Flags,L"");
			PrintStructureMember(TrueSdsOffset, L"TrueSdsOffset",Indent,Flags);
			PrintAnyString(Indent,Flags,L"");
			PrintSecurityDescriptorHeader(pSecurityHeader, L"", Indent, Flags);
			RW_BUFFER b;

			// Copy the actual SD into a buffer of its own.
			LONGLONG SdLength = DQ_ALIGN(GETSECURITYDESCRIPTORLENGTH(pSecurityHeader));
			Error = b.BUFF_CopyStdBuffer(
				ADD_PTR(pSecurityHeader, sizeof(SECURITY_DESCRIPTOR_HEADER)),
				SdLength);
			if (Error)
			{
				TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
				return;
			}
			// Print the nice ACL/DACL display.
			PrintObjectSecurity(&b, Indent, Flags);
		}
		// Calculate the offset into this map buffer for bounds check.
		MappingOffset += DQ_ALIGN(pSecurityHeader->Length);
		// Go to next SD.
		if (pSecurityHeader->Length==0 || (MappingOffset>=VACB_MAPPING_GRANULARITY)) 
		{
			// Go to next mapping buffer.							
			MappingBufferNumber++;
			pSecurityHeader = (PSECURITY_DESCRIPTOR_HEADER) ADD_PTR(pBuffer->_pBuffer, MappingBufferNumber* VACB_MAPPING_GRANULARITY);
			MappingOffset = 0;
		}
		else
		{
			pSecurityHeader = (PSECURITY_DESCRIPTOR_HEADER) ADD_PTR(pSecurityHeader, pSecurityHeader->Length);
			DWORD * pSdAddress = (DWORD*) &pSecurityHeader;
			*pSdAddress = DQ_ALIGN(*pSdAddress);
		}
	} 
}

VOID PrintReparsePoint(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
//	if (!wcscmp(L"$R", szName))
	{
		PREPARSE_DATA_BUFFER pV = (PREPARSE_DATA_BUFFER) pBuffer->_pBuffer;

		try 
		{
			WCHAR * szSubName;
			WCHAR * szPrintName;

			PrintStructureMember(*pV, L"", Indent, Flags);
			Indent += IDX_STEP;
			PrintStructureMember(pV->ReparseTag, L"ReparseTag", Indent, Flags);
			PrintStructureMember(pV->ReparseDataLength, L"ReparseDataLength", Indent, Flags);
			PrintStructureMember(pV->Reserved, L"Reserved", Indent, Flags);
			PrintAnyString(Indent, Flags, L"MountPointReparseBuffer {");

			if (pV->ReparseTag == 0x9000601a || pV->ReparseTag == 0x9000401a)
			{
				Indent += IDX_STEP;
				HEX_DUMP h;
				h.DirectOutput(gOutput);
				h.HEX_DumpHexData(pBuffer, Indent);
				Indent -= IDX_STEP;
				
			}
			else
			{
				szSubName = CreateNameString((WCHAR*)ADD_PTR(&pV->MountPointReparseBuffer.PathBuffer, pV->MountPointReparseBuffer.SubstituteNameOffset), pV->MountPointReparseBuffer.SubstituteNameLength);
				szPrintName = CreateNameString((WCHAR*)ADD_PTR(&pV->MountPointReparseBuffer.PathBuffer, pV->MountPointReparseBuffer.PrintNameOffset), pV->MountPointReparseBuffer.PrintNameLength);
				
				Indent += IDX_STEP;
				PrintStructureMember(pV->MountPointReparseBuffer.SubstituteNameOffset, L"SubstituteNameOffset", Indent, Flags);
				PrintStructureMember(pV->MountPointReparseBuffer.SubstituteNameLength, L"SubstituteNameLength", Indent, Flags);
				PrintStructureMember(pV->MountPointReparseBuffer.PrintNameOffset, L"PrintNameOffset", Indent, Flags);
				PrintStructureMember(pV->MountPointReparseBuffer.PrintNameLength, L"PrintNameLength", Indent, Flags);
				PrintStructureMember(pV->MountPointReparseBuffer.PathBuffer, L"PathBuffer[]", Indent, Flags | FL_NOTYPE);
				PrintAnyString(Indent, Flags, L"");
				PrintAnyString(Indent, Flags, L"SubName   : %s", szSubName);
				PrintAnyString(Indent, Flags, L"PrintName : %s", szPrintName);
				delete[] szSubName;
				delete[] szPrintName;
			}

				Indent -= IDX_STEP;
				PrintAnyString(Indent, Flags, L"}");
			Indent -= IDX_STEP;
			PrintAnyString(Indent, Flags, L"}");
		}
		catch (...)
		{
		}

	} 
/*
	else
	{
		// Unnamed reparse points should be SIS related.
		PREPARSE_DATA_BUFFER pV = (PREPARSE_DATA_BUFFER) pBuffer->_pBuffer;
		PrintStructureMember(*pV, L"", Indent, Flags);
		Indent += IDX_STEP;
		PrintStructureMember(pV->ReparseTag, L"ReparseTag", Indent, Flags);
		PrintStructureMember(pV->ReparseDataLength, L"ReparseDataLength", Indent, Flags);
		PrintStructureMember(pV->SIS_REPARSE_BUFFER.ReparsePointFormatVersion, L"ReparsePointFormatVersion", Indent, Flags);
		PrintStructureMember(pV->SIS_REPARSE_BUFFER.Reserved, L"Reserved", Indent, Flags);
		PrintGuid(Indent, Flags, &pV->SIS_REPARSE_BUFFER.CSid, L"CSid");
		PrintStructureMember(pV->SIS_REPARSE_BUFFER.LinkIndex.QuadPart, L"LinkIndex.QuadPart", Indent, Flags);
		PrintFileReference((PMFT_SEGMENT_REFERENCE) &pV->SIS_REPARSE_BUFFER.LinkFileNtfsId, L"LinkFileNtfsId", Indent, Flags);
		PrintFileReference((PMFT_SEGMENT_REFERENCE) &pV->SIS_REPARSE_BUFFER.CSFileNtfsId, L"CSFileNtfsId", Indent, Flags);
		PrintStructureMember(pV->SIS_REPARSE_BUFFER.CSChecksum.QuadPart, L"CSChecksum.QuadPart", Indent, Flags);
		PrintStructureMember(pV->SIS_REPARSE_BUFFER.Checksum.QuadPart, L"Checksum.QuadPart", Indent, Flags);
	
		Indent -= IDX_STEP;
		PrintAnyString(Indent, Flags, L"}");
	}
*/
}

VOID PrintVolumeInformation(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PVOLUME_INFORMATION pV = (PVOLUME_INFORMATION) pBuffer->_pBuffer;
	PrintStructureMember(*pV, szName, Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->Reserved, L"Reserved", Indent, Flags);
	PrintStructureMember(pV->MajorVersion, L"MajorVersion", Indent, Flags);
	PrintStructureMember(pV->MinorVersion, L"MinorVersion", Indent, Flags);
	PrintStructureMember(pV->VolumeFlags, L"VolumeFlags", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintObjectId(RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	HEX_DUMP h;
	h.DirectOutput(gOutput);
	RW_BUFFER e;
	
	PFILE_OBJECTID_BUFFER pV = (PFILE_OBJECTID_BUFFER) pBuffer->_pBuffer;
	PrintStructureMember(*pV, szName, Indent, Flags);
	Indent += IDX_STEP;
	
	PrintGuid(Indent, Flags, (GUID*) pV->BirthVolumeId, L"BirthVolumeId");
	PrintGuid(Indent, Flags, (GUID*) pV->BirthObjectId, L"BirthObjectId");
	PrintGuid(Indent, Flags, (GUID*) pV->DomainId,      L"DomainId     ");
	PrintGuid(Indent, Flags, (GUID*) pV->ObjectId,      L"ObjectId     ");
	PrintAnyString(Indent, Flags, L"CHAR ExtendedInfo[] {");

	Error = e.BUFF_CopyStdBuffer(pV->ExtendedInfo, 48);
	if (Error)
	{
		TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
		return;
	}
	if (!e.BUFF_IsAllZeroes())
	{
		PrintStructureMember(pV->ExtendedInfo[0], L"ExtendedInfo", Indent, Flags | FL_UNDECORATED);
		h.HEX_DumpHexData(&e,Indent, 8);
	}
	else
	{
		PrintAnyString(Indent+IDX_STEP, Flags, L"...");
	}

	PrintAnyString(Indent, Flags, L"}");
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintFileRecord(PMFT_SEGMENT_REFERENCE pFrs, RW_BUFFER * pBuffer, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	DWORD Error;
	PFILE_RECORD_SEGMENT_HEADER pH = (PFILE_RECORD_SEGMENT_HEADER) pBuffer->_pBuffer;
	PATTRIBUTE_RECORD_HEADER pAttribute = (PATTRIBUTE_RECORD_HEADER) ADD_PTR(pH, pH->FirstAttributeOffset);
	PFILE_NAME pFilename = NULL;
	WCHAR * szFileName = NULL;

	if (NtfsFullSegmentNumber(pFrs) > 0)
	{
		PrintAnyString(Indent, Flags, L"");
		PrintAnyString(Indent, Flags, L"Jump to Previous FRS <\\\\FRS:%08I64x>", NtfsFullSegmentNumber(pFrs) - 1);
		PrintAnyString(Indent, Flags, L"Jump to Next     FRS <\\\\FRS:%08I64x>", NtfsFullSegmentNumber(pFrs) + 1);
		PrintAnyString(Indent, Flags, L"");
	}
	else
	{
		PrintAnyString(Indent, Flags, L"");
		PrintAnyString(Indent, Flags, L"Jump to Next     FRS <\\\\FRS:%08I64x>", NtfsFullSegmentNumber(pFrs) + 1);
		PrintAnyString(Indent, Flags, L"");
	}

	if (pH->MultiSectorHeader.Signature != 'ELIF')
	{
		HEX_DUMP h;
		h.DirectOutput(gOutput);
		h.HEX_DumpHexData(pBuffer, 0, 0x10);
		return;
	}

	if (pAttribute && !pAttribute->TypeCode)
	{
		return;
	}
	
	if (IS_FLAG_SET(pH->Flags, FILE_NAME_INDEX_PRESENT))
	{
		PrintAnyString(Indent, Flags, L"FOLDER VIEW        <\\\\DIR:%08I64x>", NtfsFullSegmentNumber(pFrs));
	}
	else
	{
		PrintAnyString(Indent, Flags, L"HEX DUMP DATA      <\\\\STR:%08I64x:80:>", NtfsFullSegmentNumber(pFrs));

		//Create a temporary volume class to search for the filename.
		NTFS_VOLUME * pVolume = new NTFS_VOLUME;
		PATTRIBUTE_RECORD_HEADER pNameAttribute = (PATTRIBUTE_RECORD_HEADER)pVolume->GetFileNameAttributerRecordPointer(pBuffer, 0);

		if (pNameAttribute != NULL)
		{
			PFILE_NAME pFileName = (PFILE_NAME)ADD_PTR(pNameAttribute, pNameAttribute->Form.Resident.ValueOffset);
			szFileName = CreateNameString(pFileName->FileName, pFileName->FileNameLength);
		}

		PrintAnyString(Indent, Flags, L"SAVE DATA AS BIN   <\\\\BIN:%08I64x:80:*:%s>", NtfsFullSegmentNumber(pFrs), (szFileName) ? szFileName : L"");
		if (szFileName) delete[] szFileName;
	}
	
	PrintAnyString(Indent, Flags, L"%s", szName);
	PrintFrsHeader(pH,Indent,Flags);
	PrintAnyString(Indent, Flags, L"");

	Indent += IDX_STEP;

	while ( pAttribute && (pAttribute->TypeCode != $END) )
	{
		PrintAttributeRecordHeader(*pFrs, pAttribute, L"", Indent, Flags);
		PrintAnyString(Indent, Flags, L"");
		
		Indent += IDX_STEP;

		if (pAttribute->FormCode == NONRESIDENT_FORM)
		{
			PrintExtents(pAttribute, Indent, Flags);
			PrintAnyString(Indent, Flags, L"");
		}
		else
		{
			PWCHAR szAttributeName = CreateNameString((PWCHAR) ADD_PTR(pAttribute, pAttribute->NameOffset) , pAttribute->NameLength);
			RW_BUFFER AttrBuffer;
			HEX_DUMP h;
			h.DirectOutput(gOutput);
			
			Error = AttrBuffer.BUFF_CopyStdBuffer(
							ADD_PTR(pAttribute, pAttribute->Form.Resident.ValueOffset),
							pAttribute->Form.Resident.ValueLength);
			if (Error)
			{
				TypeDbgPrint(0,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
				return;
			}
			switch (pAttribute->TypeCode)
			{
				case $STANDARD_INFORMATION:
					PrintStandardInformation(&AttrBuffer,L"",Indent,Flags);
					PrintAnyString(Indent, Flags, L"");
					break;
				case $ATTRIBUTE_LIST:
					PrintAttributeList(&AttrBuffer, Indent, Flags);
					break;
				case $FILE_NAME:
					PrintFilenameInformation(*pFrs, &AttrBuffer, L"", Indent, Flags);
					break;
				case $SECURITY_DESCRIPTOR:
					PrintObjectSecurity(&AttrBuffer, Indent, Flags);
					break;
				case $VOLUME_NAME:
					PrintVolumeName(&AttrBuffer, Indent, Flags);
					break;
				case $INDEX_ROOT:
					PrintIndexBuffer(
						*pFrs, // Not used.
						&AttrBuffer, 
						szAttributeName,
						Indent, 
						Flags);
					break;
				case $OBJECT_ID:
					PrintObjectId(&AttrBuffer, L"", Indent, Flags);
					break;
				case $VOLUME_INFORMATION:
					PrintVolumeInformation(&AttrBuffer, L"", Indent, Flags);
					break;
				case $REPARSE_POINT:
					PrintReparsePoint(&AttrBuffer, L"", Indent, Flags);
					break;
				case $BITMAP:
					PrintAnyString(Indent, Flags, L"BITMAP {");
					h.HEX_DumpHexDataAsBitmap(&AttrBuffer,Indent, 4);
					PrintAnyString(Indent, Flags, L"}");
					break;
				default:
					h.HEX_DumpHexData(&AttrBuffer, Indent, 0x10, 0);
					PrintAnyString(Indent, Flags, L"");
					break;
				}
			
			PrintAnyString(Indent, Flags, L"");
			delete [] szAttributeName;
		}
		Indent -= IDX_STEP;
		pAttribute = (PATTRIBUTE_RECORD_HEADER) ADD_PTR(pAttribute, pAttribute->RecordLength);
		
	}
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"");
}

VOID PrintVolumeName(RW_BUFFER * pNameBuffer, USHORT Indent, ULONG Flags)
{
	PWCHAR szVolumeName = CreateNameString((PWCHAR)pNameBuffer->_pBuffer, (size_t) pNameBuffer->_NumberOfBytes / sizeof(WCHAR));
	PrintAnyString(Indent, Flags, L"VOLUME_NAME {");
	PrintAnyString(Indent+IDX_STEP, Flags, L"Name : \"%s\"", szVolumeName);
	PrintAnyString(Indent, Flags, L"}");
	delete [] szVolumeName;
}

VOID PrintAttributeListEntry(PATTRIBUTE_LIST_ENTRY pV, USHORT Indent, ULONG Flags)
{
	PWCHAR szName = CreateNameString((PWCHAR) ADD_PTR(pV, pV->AttributeNameOffset),pV->AttributeNameLength);

	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
		PrintStructureMember(pV->AttributeTypeCode, L"AttributeTypeCode", Indent, Flags);
		PrintStructureMember(pV->RecordLength, L"RecordLength", Indent, Flags);
		PrintStructureMember(pV->AttributeNameLength, L"AttributeNameLength", Indent, Flags);
		PrintStructureMember(pV->AttributeNameOffset, L"AttributeNameOffset", Indent, Flags);
		PrintStructureMember(pV->LowestVcn, L"LowestVcn", Indent, Flags);
		PrintFileReference(&pV->SegmentReference, L"SegmentReference", Indent, Flags);
		PrintStructureMember(pV->Instance, L"Instance", Indent, Flags);
		PrintStructureMember(pV->AttributeName, L"AttributeName", Indent, Flags | FL_NOTYPE | FL_NOCRLF);
		PrintAnyString(Indent, Flags, L"\"%s\"", szName);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}

VOID PrintAttributeList(RW_BUFFER * pBuffer, USHORT Indent, ULONG Flags)
{
	PATTRIBUTE_LIST_ENTRY pEntry = (PATTRIBUTE_LIST_ENTRY) pBuffer->_pBuffer;
	PATTRIBUTE_LIST_ENTRY pMax   = (PATTRIBUTE_LIST_ENTRY) ADD_PTR(pBuffer->_pBuffer, pBuffer->_NumberOfBytes);

	while (	pEntry < pMax && 
			pEntry->AttributeTypeCode != $END && 
			pEntry->RecordLength &&
			(PATTRIBUTE_LIST_ENTRY) (ADD_PTR(pEntry, pEntry->RecordLength)) <= pMax)
	{
		PrintAttributeListEntry(pEntry, Indent+4, Flags);
		pEntry = (PATTRIBUTE_LIST_ENTRY) NtfsGetNextRecord(pEntry);
	}
}

VOID PrintDataAttribute(RW_BUFFER * pBuffer, WCHAR * szName, LONGLONG StartOffset, USHORT Indent, ULONG Flags)
{
	MFT_SEGMENT_REFERENCE Frs={0};	
	PrintDataAttributeEx(Frs, pBuffer, szName, StartOffset, Indent, Flags);
}

VOID PrintDataAttributeEx(MFT_SEGMENT_REFERENCE Frs, RW_BUFFER * pBuffer, WCHAR * szName, LONGLONG StartOffset, USHORT Indent, ULONG Flags)
{
	if (szName && !wcscmp(szName,L"$SDS"))
	{
		PrintSecurityStream(pBuffer, Indent, Flags);
	}
	else
	{
		HEX_DUMP h;
		h.DirectOutput(gOutput);
		h.HEX_DumpHexData(pBuffer, Indent, 0x10, StartOffset);
	}
}

VOID PrintGuid(USHORT Indent, ULONG Flags, GUID * Guid, WCHAR * szName)
{
        
	PrintAnyString(Indent, Flags, L"GUID %s {%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}",
		szName,
		Guid->Data1,
		Guid->Data2,
        Guid->Data3,
		Guid->Data4[0],
		Guid->Data4[1],
		Guid->Data4[2],
		Guid->Data4[3],
		Guid->Data4[4],
		Guid->Data4[5],
		Guid->Data4[6],
		Guid->Data4[7]);
}


VOID PrintAttributeStream(PMFT_SEGMENT_REFERENCE pFrs, RW_BUFFER * pBuffer, ATTRIBUTE_TYPE_CODE Type, WCHAR * szName, LONGLONG ThisOffset, USHORT Indent, ULONG Flags)
{	
	HEX_DUMP h;
	h.DirectOutput(gOutput);

	Indent += IDX_STEP;
	
	switch(Type)
	{
	case $STANDARD_INFORMATION :
		PrintStandardInformation(pBuffer, L"", Indent, Flags);
		break;
	case $ATTRIBUTE_LIST       :
		PrintAttributeList(pBuffer, Indent, Flags);
		break;
	case $FILE_NAME            :
		PrintFilenameInformation(*pFrs, pBuffer, L"", Indent, Flags);
		break;
	case $OBJECT_ID            :
		PrintObjectId(pBuffer, L"", Indent, Flags);
		break;
	case $SECURITY_DESCRIPTOR  :
		PrintObjectSecurity(pBuffer, Indent, Flags);
		break;
	case $VOLUME_NAME          :
		PrintVolumeName(pBuffer, Indent, Flags);
		break;
	case $VOLUME_INFORMATION   :
		PrintVolumeInformation(pBuffer, L"", Indent, Flags);
		break;
	case $INDEX_ROOT           :
		PrintIndexBuffer(*pFrs, pBuffer, szName, Indent, Flags);
		break;
	case $INDEX_ALLOCATION     :
		PrintIndexAllocationBuffer(pBuffer, szName, Indent, Flags);
		if (IS_FLAG_SET(Flags, FL_TERSE_INDEX))
		{
			//No extra CRLF
			return;
		}
		break;
	case $BITMAP               :
		h.HEX_DumpHexDataAsBitmap(pBuffer, ThisOffset, Indent, 0x4);
		break;
	case $REPARSE_POINT        :
		PrintReparsePoint(pBuffer, L"", Indent, Flags);
		break;
	case $DATA                 :
		if (Flags & FL_IS_BITMAP)
			h.HEX_DumpHexDataAsBitmap(pBuffer, ThisOffset, Indent, 0x4);
		else
			PrintDataAttributeEx(*pFrs, pBuffer, szName, ThisOffset, Indent, Flags);
		break;
	case $EA                   :
	case $EA_INFORMATION       :
		h.HEX_DumpHexData(pBuffer,Indent,0x10,ThisOffset);
		break;
	case $LOGGED_UTILITY_STREAM:
		// TODO : dump this for TXF and EFS
		h.HEX_DumpHexData(pBuffer,Indent,0x10,ThisOffset);
		break;
	default:
		break;
	}
	
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"");
}

VOID PrintLsn(PLSN pV, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(pV->QuadPart, szName, Indent, Flags | FL_NOCRLF);
	if (gSeqBits)
	{
		LONGLONG FileOffset = LfsLsnToFileOffset(gSeqBits, *pV);
		PrintAnyString(2, Flags, L"OFF +0x%I64x", FileOffset );
	}
	else
	{
		PrintAnyString(2, Flags, L"");
	}
}


VOID PrintQuadword(LONGLONG Number, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(Number, szName, Indent, Flags);
}

VOID PrintDword(LONG Number, WCHAR * szName, USHORT Indent, ULONG Flags)
{
	PrintStructureMember(Number, szName, Indent, Flags);
}

VOID PrintFullVolumeSummary(PRW_BUFFER pVi, PNTFS_VOLUME_DATA_BUFFER pV, USHORT Indent, ULONG Flags)
{
	PrintVolumeInformation(pVi, L"VOLUME_INFORMATION", Indent, Flags);
	PrintAnyString(0,0,L" ");
	PrintStructureMember(*pV, L"", Indent, Flags);
	Indent += IDX_STEP;
	PrintStructureMember(pV->VolumeSerialNumber.QuadPart, L"VolumeSerialNumber", Indent, Flags);
	PrintStructureMember(pV->NumberSectors.QuadPart, L"NumberSectors", Indent, Flags);
	PrintStructureMember(pV->TotalClusters.QuadPart, L"TotalClusters", Indent, Flags);
	PrintStructureMember(pV->BytesPerSector, L"BytesPerSector", Indent, Flags);
	PrintStructureMember(pV->BytesPerCluster, L"BytesPerCluster", Indent, Flags);
	PrintStructureMember(pV->BytesPerFileRecordSegment, L"BytesPerFileRecordSegment", Indent, Flags);
	PrintStructureMember(pV->ClustersPerFileRecordSegment, L"ClustersPerFileRecordSegment", Indent, Flags);
	PrintStructureMember(pV->MftValidDataLength.QuadPart, L"MftValidDataLength", Indent, Flags);
	PrintStructureMember(pV->MftStartLcn.QuadPart, L"MftStartLcn", Indent, Flags);
	PrintStructureMember(pV->Mft2StartLcn.QuadPart, L"Mft2StartLcn", Indent, Flags);
	PrintStructureMember(pV->TotalClusters, L"", Indent, Flags);
	Indent -= IDX_STEP;
	PrintAnyString(Indent, Flags, L"}");
}
