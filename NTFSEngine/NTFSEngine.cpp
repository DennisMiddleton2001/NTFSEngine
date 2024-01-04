// NTFSEngine.cpp : DeFes the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ntfs_volume.h"
#include "DUMP_TYPES.h"

NTFS_VOLUME * pVolume = NULL;

#define OptTxt 1
#define OptHex 2
#define OptBin 3

VOID PrintVolumeDetails()
{
	PrintAnyString(0,0, L"VOLUME: %s", pVolume->_szDeviceName);
	PrintAnyString(0,0, L"====================================");
}

VOID PrintFileRecordDetails(PMFT_SEGMENT_REFERENCE pFrs)
{
	WCHAR * szPath;
	if (!pVolume->FormatPathString(pFrs, &szPath))
	{		
		PrintAnyString(0,0,L"Path:  %s", szPath);
	}
	delete [] szPath;

}

extern "C" __declspec(dllexport) DWORD __stdcall NtfsOpenVolume(
	IN WCHAR * szPath, 
	IN DWORD DebugFlags, 
	IN LONG RelativeSector,
	IN LONGLONG NumberSectors,
	IN LCN MftStartLcn,
	IN LCN Mft2StartLcn,
	IN LONG SectorsPerCluster,
	IN LONG RawInitFlags)
{	
	if (pVolume != NULL)
	{
		//Sharing violation.
		return 0x20;
	}
	else
	{
		pVolume = new NTFS_VOLUME(DebugFlags);
		return pVolume->Initialize(szPath, RelativeSector, NumberSectors, Mft2StartLcn, Mft2StartLcn, SectorsPerCluster, RawInitFlags);
	}
}

extern "C" __declspec(dllexport) DWORD NtfsCloseVolume()
{
	DWORD Error = pVolume->CloseVolumeHandle();
	if (Error == NO_ERROR)
	{
		delete pVolume;
		pVolume = NULL;
	}
	return Error;
}

DWORD NtfsReadFrsParsed(LONGLONG frs, BOOL IncludeChildren, DWORD Option, WCHAR * szOutputFilename)
{
		RW_BUFFER FrsBuffer, ChildFrs, AttrList;
		MFT_SEGMENT_REFERENCE PreviousFrs;
		HEX_DUMP h;
		FILE * FileOut = NULL;
		DWORD Error;
		USHORT Indent = 0;
		DWORD Flags = 0;

		WCHAR * szFileName = NULL;

		OpenOutput(szOutputFilename);
		PrintVolumeDetails();
		PrintFileRecordDetails((PMFT_SEGMENT_REFERENCE) &frs);
		ERROR_EXIT(pVolume->ReadFrsRaw(&FrsBuffer, (PMFT_SEGMENT_REFERENCE) &frs))
		
		Error = pVolume->ReadAttributeList(&AttrList, &FrsBuffer);
		
		PATTRIBUTE_LIST_ENTRY pEntry = (PATTRIBUTE_LIST_ENTRY)AttrList._pBuffer;
		PATTRIBUTE_LIST_ENTRY pMax = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(AttrList._pBuffer, AttrList._NumberOfBytes);

		PrintQuadword(NtfsFullSegmentNumber(&frs), L"FRS             ", Indent, Flags | FL_UNDECORATED);
		PrintFileRecord((PMFT_SEGMENT_REFERENCE) &frs, &FrsBuffer, L"", Indent, Flags);

		while (IncludeChildren && pEntry < pMax && pEntry->AttributeTypeCode != $END && pEntry->RecordLength)
		{
			if ((NtfsFullSegmentNumber(&pEntry->SegmentReference) != frs) &&
				(NtfsFullSegmentNumber(&pEntry->SegmentReference) != NtfsFullSegmentNumber(&PreviousFrs)))
			{
				PrintQuadword(NtfsFullSegmentNumber(&pEntry->SegmentReference), L"Child File Record ", Indent, Flags | FL_UNDECORATED);

				Error = pVolume->ReadFrsRaw(&ChildFrs, &pEntry->SegmentReference);
				if (!Error)
				{
					PrintFileRecord(&pVolume->_LastReadFrs, &ChildFrs, L"", Indent, Flags);
				}
				else
				{
					PrintAnyString(Indent, Flags, L"Unable to read File Record");
					break;
				}
			}

			NtfsSetSegmentNumber(&PreviousFrs,
				pEntry->SegmentReference.SegmentNumberHighPart,
				pEntry->SegmentReference.SegmentNumberLowPart);

			pEntry = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(pEntry, pEntry->RecordLength);

		}

		CloseOutput();

		return NO_ERROR;
}

DWORD NtfsReadFrsHex(LONGLONG frs, BOOL IncludeChildren, DWORD Option, WCHAR * szOutputFilename)
{
		RW_BUFFER FrsBuffer, ChildFrs, AttrList;
		MFT_SEGMENT_REFERENCE PreviousFrs;
		HEX_DUMP h;
		DWORD Error, Flags=0;
		USHORT Indent=0;

		h.OpenOutput(szOutputFilename);
		DirectOutput(h.Output);
		
		PrintVolumeDetails();
		PrintFileRecordDetails((PMFT_SEGMENT_REFERENCE) &frs);
		
		ERROR_EXIT(pVolume->ReadFrsRaw(&FrsBuffer, (PMFT_SEGMENT_REFERENCE) &frs))
		Error = pVolume->ReadAttributeList(&AttrList, &FrsBuffer);
		PATTRIBUTE_LIST_ENTRY pEntry = (PATTRIBUTE_LIST_ENTRY)AttrList._pBuffer;
		PATTRIBUTE_LIST_ENTRY pMax = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(AttrList._pBuffer, AttrList._NumberOfBytes);
		h.HEX_DumpHexData(&FrsBuffer, 0, 16);

		while (IncludeChildren && pEntry < pMax && pEntry->AttributeTypeCode != $END && pEntry->RecordLength)
		{
			if ((NtfsFullSegmentNumber(&pEntry->SegmentReference) != frs) &&
				(NtfsFullSegmentNumber(&pEntry->SegmentReference) != NtfsFullSegmentNumber(&PreviousFrs)))
			{
				Error = pVolume->ReadFrsRaw(&ChildFrs, &pEntry->SegmentReference);
				if (!Error)
				{
					h.HEX_DumpHexData(&ChildFrs, 0, 16);
				}
				else
				{
					break;
				}
			}

			NtfsSetSegmentNumber(&PreviousFrs,
				pEntry->SegmentReference.SegmentNumberHighPart,
				pEntry->SegmentReference.SegmentNumberLowPart);

			pEntry = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(pEntry, pEntry->RecordLength);

		}

		h.CloseOutput();
		return NO_ERROR;
}

DWORD NtfsReadFrsBin(LONGLONG frs, BOOL IncludeChildren, DWORD Option, WCHAR * szOutputFilename)
{
		RW_BUFFER FrsBuffer, ChildFrs, AttrList;
		MFT_SEGMENT_REFERENCE PreviousFrs;
		HEX_DUMP h;
		DWORD Error, Flags=0;
		USHORT Indent=0;
		FILE * FileOut = _wfopen(szOutputFilename, L"wb");
		if (FileOut == NULL) return GetLastError();
						
		ERROR_EXIT(pVolume->ReadFrsRaw(&FrsBuffer, (PMFT_SEGMENT_REFERENCE) &frs))
		
		Error = pVolume->ReadAttributeList(&AttrList, &FrsBuffer);
		
		PATTRIBUTE_LIST_ENTRY pEntry = (PATTRIBUTE_LIST_ENTRY)AttrList._pBuffer;
		PATTRIBUTE_LIST_ENTRY pMax = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(AttrList._pBuffer, AttrList._NumberOfBytes);

		if (!fwrite(FrsBuffer._pBuffer, sizeof(CHAR), (size_t)FrsBuffer._NumberOfBytes, FileOut))
		{
			DWORD Error = GetLastError();
			fclose(FileOut);
			return Error;
		}

		while (IncludeChildren && pEntry < pMax && pEntry->AttributeTypeCode != $END && pEntry->RecordLength)
		{
			if ((NtfsFullSegmentNumber(&pEntry->SegmentReference) != frs) &&
				(NtfsFullSegmentNumber(&pEntry->SegmentReference) != NtfsFullSegmentNumber(&PreviousFrs)))
			{
				Error = pVolume->ReadFrsRaw(&ChildFrs, &pEntry->SegmentReference);
				if (!Error)
				{
					if (!fwrite(FrsBuffer._pBuffer, sizeof(CHAR), (size_t)FrsBuffer._NumberOfBytes, FileOut))
					{
						DWORD Error = GetLastError();
						fclose(FileOut);
						return Error;
					}
				}
				else
				{
					break;
				}
			}

			NtfsSetSegmentNumber(&PreviousFrs,
				pEntry->SegmentReference.SegmentNumberHighPart,
				pEntry->SegmentReference.SegmentNumberLowPart);

			pEntry = (PATTRIBUTE_LIST_ENTRY)ADD_PTR(pEntry, pEntry->RecordLength);

		}

		fclose(FileOut);
		return NO_ERROR;
}


extern "C" __declspec(dllexport) DWORD NtfsReadFrs(LONGLONG frs, BOOL IncludeChildren, DWORD Option, WCHAR * szOutputFilename)
{
	DWORD Error;
	switch (Option)
	{
	case OptTxt:
		Error=NtfsReadFrsParsed(frs, IncludeChildren, Option, szOutputFilename);
		break;
	case OptHex:
		Error=NtfsReadFrsHex(frs, IncludeChildren, Option, szOutputFilename);
		break;
	case OptBin:
		Error = NtfsReadFrsBin(frs, IncludeChildren, Option, szOutputFilename);
		break;
	}

	return NO_ERROR;
}

DWORD NtfsReadStreamParsed(LONGLONG Frs, DWORD Type, WCHAR * szAttrName, WCHAR * szOutputFilename)
{
	RW_BUFFER FrsBuffer, AttrBuffer;
	LONGLONG StartOffset;
	LONG OffsetStride;
	DWORD Flags=0;
	MFT_SEGMENT_REFERENCE ParentFrs = {0};  // Only used for hyperlink navigation

	ERROR_EXIT(OpenOutput(szOutputFilename))
	PrintVolumeDetails();
	PrintFileRecordDetails((PMFT_SEGMENT_REFERENCE) &Frs);

	if (Type == $DATA && Frs == BIT_MAP_FILE_NUMBER && (szAttrName == NULL || wcslen(szAttrName)==0))
	{
		Flags |= FL_IS_BITMAP;
	}
	
	if ((Type == $DATA && szAttrName && _wcsicmp(szAttrName, L"$SDS") == 0) ||
		Type == $INDEX_ALLOCATION ||
		Type == $INDEX_ROOT)
	{
		OffsetStride = VACB_MAPPING_GRANULARITY;
		Flags |= FLIO_READ_PAST_VDL;
	}
	else
	{
		OffsetStride = USA_SECTOR_SIZE;
	}

	ERROR_EXIT(pVolume->ReadFrsRaw(&FrsBuffer, (PMFT_SEGMENT_REFERENCE)&Frs))

	StartOffset = 0x0;
	BOOL bContinue = false;
	DWORD Error = 0;
	
	// Read each IAB until done.
	do
	{
		Error = pVolume->ReadAttributeByteRangeUsaAligned(
			&AttrBuffer,
			&FrsBuffer,
			Type,
			szAttrName,
			StartOffset,
			&OffsetStride,
			Flags);

		if (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA)
		{
			PrintAttributeStream((PMFT_SEGMENT_REFERENCE)&Frs, &AttrBuffer, Type, szAttrName, StartOffset, 0, Flags);
		}

		StartOffset += OffsetStride;

	} while (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA);

	CloseOutput();
	return 0;
}

extern "C" __declspec(dllexport) DWORD NtfsPrintFolder(LONGLONG Frs, WCHAR * szOutputFilename)
{
	RW_BUFFER FrsBuffer, AttrBuffer, FileNameBuffer;
	LONGLONG StartOffset;
	LONG OffsetStride;
	DWORD IoFlags = FLIO_READ_PAST_VDL;
	DWORD PrintFlags = FL_TERSE_INDEX;
	MFT_SEGMENT_REFERENCE ParentFrs={0};
	ERROR_EXIT(OpenOutput(szOutputFilename))
	PrintVolumeDetails();
	PrintFileRecordDetails((PMFT_SEGMENT_REFERENCE) &Frs);

	OffsetStride = VACB_MAPPING_GRANULARITY;
	
	ERROR_EXIT(pVolume->ReadFrsRaw(&FrsBuffer, (PMFT_SEGMENT_REFERENCE)&Frs))
	ERROR_EXIT(pVolume->ReadAttributeByteRange(&AttrBuffer, &FrsBuffer, $INDEX_ROOT, L"$I30", 0, -1, IoFlags))
	
	if (pVolume->ReadAttributeByteRange(&FileNameBuffer, &FrsBuffer, $FILE_NAME, L"", 0, -1, FILE_NAME_NTFS)==NO_ERROR)
	{
		PFILE_NAME pFn = (PFILE_NAME) FileNameBuffer._pBuffer;
		ParentFrs = pFn->ParentDirectory;
	}

	if (pVolume->ReadAttributeByteRange(&FileNameBuffer, &FrsBuffer, $FILE_NAME, L"", 0, -1, IoFlags)==NO_ERROR)
	{
		PFILE_NAME pFn = (PFILE_NAME) FileNameBuffer._pBuffer;
		ParentFrs = pFn->ParentDirectory;
	}
	
	if (NtfsFullSegmentNumber(&ParentFrs) !=0)
	{
		PrintAnyString(0,0,L"VIEW PARENT FOLDER  <\\\\DIR:%08I64x>",NtfsFullSegmentNumber(&ParentFrs));
	}
	PrintAnyString(0,0,L"VIEW BASE INDEX FRS <\\\\FRS:%08I64x>",Frs);
	PrintAttributeStream((PMFT_SEGMENT_REFERENCE) &Frs, &AttrBuffer, $INDEX_ROOT, L"$I30", 0, 0, PrintFlags);

	StartOffset = 0x0;
	BOOL bContinue = false;
	DWORD Error = 0;
	
	// Read each IAB until done.
	do
	{
		Error = pVolume->ReadAttributeByteRangeUsaAligned(
			&AttrBuffer,
			&FrsBuffer,
			$INDEX_ALLOCATION,
			L"$I30",
			StartOffset,
			&OffsetStride,
			IoFlags);

		if (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA)
		{
			PrintAttributeStream((PMFT_SEGMENT_REFERENCE)&Frs, &AttrBuffer, $INDEX_ALLOCATION, L"$I30", StartOffset, 0, PrintFlags);
		}

		StartOffset += OffsetStride;

	} while (Error == ERROR_SUCCESS || Error == ERROR_MORE_DATA);

	CloseOutput();
	return 0;
}


extern "C" __declspec(dllexport) DWORD NtfsReadStream(LONGLONG Frs, DWORD Type, WCHAR * szAttrName, DWORD Option, LONGLONG Start, LONG Bytes, WCHAR * szOutputFilename)
{
		RW_BUFFER AttributeBuffer;
		HEX_DUMP h;
		FILE * FileOut = NULL;
		DWORD Error = 0;
		LONGLONG Offset = Start;
		BOOL bReadPartial;
		LONG BytesRemaining = Bytes;

		if (Bytes == -1)
			bReadPartial = false;
		else
			bReadPartial = true;
		
		switch (Option)
		{
		case OptTxt:			
			ERROR_EXIT(NtfsReadStreamParsed(Frs, Type, szAttrName, szOutputFilename))
			break;
		case OptHex:
			{
				ERROR_EXIT(h.OpenOutput(szOutputFilename))
				DirectOutput(h.Output);
				PrintVolumeDetails();
				PrintFileRecordDetails((PMFT_SEGMENT_REFERENCE) &Frs);
				do
				{
					Error = pVolume->ReadAttributeByteRange(&AttributeBuffer, (PMFT_SEGMENT_REFERENCE) &Frs, Type, szAttrName, Offset, Bytes, 0);
					if (Error == ERROR_HANDLE_EOF || Error == NO_ERROR)
					{
						h.HEX_DumpHexData(&AttributeBuffer, 0, 16, Offset);
						Offset += AttributeBuffer._NumberOfBytes;
						BytesRemaining -= (LONG) AttributeBuffer._NumberOfBytes;
						if (bReadPartial && BytesRemaining == 0) break;
					}
				} while (Error == NO_ERROR);
				
				h.CloseOutput();		
			}
			break;
		case OptBin:
			{
				FileOut = _wfopen(szOutputFilename, L"wb");
				if (FileOut == NULL) return GetLastError();
				do
				{
					Error = pVolume->ReadAttributeByteRange(&AttributeBuffer, (PMFT_SEGMENT_REFERENCE) &Frs, Type, szAttrName, Offset, Bytes, 0);
					if (Error == ERROR_HANDLE_EOF || Error == NO_ERROR)
					{
						if (!fwrite(AttributeBuffer._pBuffer, sizeof(CHAR), (size_t)AttributeBuffer._NumberOfBytes, FileOut))
						{
							DWORD Error = GetLastError();
							fclose(FileOut);
							return Error;
						}
						Offset += AttributeBuffer._NumberOfBytes;
						BytesRemaining -= (LONG) AttributeBuffer._NumberOfBytes;
						if (bReadPartial && BytesRemaining == 0) break;
					}
				} while (Error == NO_ERROR);
				fclose(FileOut);
			}
			
			break;
		}
		return NO_ERROR;
}


extern "C" __declspec(dllexport) DWORD NtfsPrintLcnRange(LONGLONG Lcn, DWORD Length, DWORD Option, WCHAR * szOutputFilename)
{
		RW_BUFFER Buffer;
		HEX_DUMP h;
		FILE * FileOut;

		ERROR_EXIT(pVolume->ReadNtfsClusters(&Buffer, Lcn, Length))

		if (Option == OptHex)
		{
		ERROR_EXIT(h.OpenOutput(szOutputFilename))
		DirectOutput(h.Output);
		PrintVolumeDetails();	
		
		ERROR_EXIT(h.HEX_DumpHexData(&Buffer, 0, 16))
		ERROR_EXIT(h.CloseOutput());
		}
		else
		{
			FileOut = _wfopen(szOutputFilename, L"wb");
			if (FileOut == NULL) return GetLastError();
			if (!fwrite(Buffer._pBuffer, sizeof(CHAR), (size_t)Buffer._NumberOfBytes, FileOut))
			{
				DWORD Error = GetLastError();
				fclose(FileOut);
				return Error;
			}
			fclose(FileOut);
		}
		return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) DWORD NtfsPrintLbnRange(LONGLONG Lbn, DWORD Length, DWORD Option, WCHAR * szOutputFilename)
{
		RW_BUFFER Buffer;
		HEX_DUMP h;
		FILE * FileOut;

		ERROR_EXIT(pVolume->ReadNtfsBlocks(&Buffer, Lbn, Length, pVolume->_VolumeData.BytesPerSector))

		if (Option == OptHex)
		{
		ERROR_EXIT(h.OpenOutput(szOutputFilename))
		DirectOutput(h.Output);
		PrintVolumeDetails();
		
		ERROR_EXIT(h.HEX_DumpHexData(&Buffer, 0, 16))
		ERROR_EXIT(h.CloseOutput());
		}
		else
		{
			FileOut = _wfopen(szOutputFilename, L"wb");
			if (FileOut == NULL) return GetLastError();
			if (!fwrite(Buffer._pBuffer, sizeof(CHAR), (size_t)Buffer._NumberOfBytes, FileOut))
			{
				DWORD Error = GetLastError();
				fclose(FileOut);
				return Error;
			}
			fclose(FileOut);
		}
		return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) DWORD NtfsCrackPath(ULONGLONG Frs, WCHAR * OutputFile)
{
		RW_BUFFER FileNameBuffer;
		WCHAR * szPath = NULL;
        
		OpenOutput(OutputFile);
		ERROR_EXIT(pVolume->FormatPathString((PMFT_SEGMENT_REFERENCE) &Frs ,&szPath))
		if (szPath)
		{			
			PrintAnyString(0,0,L"FRS 0x%I64x is \"%s\"\n",Frs, szPath);
			delete [] szPath;
		}

		CloseOutput();
		return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) DWORD NtfsGetVolumeInformation(WCHAR * szOutputFilename)
{
	OpenOutput(szOutputFilename);
	PrintVolumeDetails();
	
	RW_BUFFER frsBuffer, viBuffer;
	MFT_SEGMENT_REFERENCE Frs = {0};
	Frs.SegmentNumberLowPart = VOLUME_DASD_NUMBER;
	pVolume->ReadFrsRaw(&frsBuffer, &Frs);
	LONG Bytes = -1;
	DWORD Error = pVolume->ReadAttributeByteRangeUsaAligned(&viBuffer, &frsBuffer, $VOLUME_INFORMATION, L"", 0, &Bytes, 0);
	if (!Error)
	{
		PrintFullVolumeSummary(&viBuffer, &pVolume->_VolumeData, 0, 0);
	}

	PrintAnyString(0, 0, L"\n\n");
	PrintAnyString(0, 0, L"Quick Start Links (Text Output)");
	PrintAnyString(0, 0, L"                            View Parsed                 Save as Binary");
	PrintAnyString(0, 0, L"==============================================================================");
	PrintAnyString(0, 0, L"BOOT AREA           - <\\\\STR:00000007:80:>       <\\\\BIN:00000007:80:>");
	PrintAnyString(0, 0, L"$MFT RECORDS        - <\\\\FRS:00000000>           <\\\\BIN:00000000:80:>");
	PrintAnyString(0, 0, L"ROOT FOLDER         - <\\\\DIR:00000005>           <\\\\BIN:00000005:A0:$I30>");
	PrintAnyString(0, 0, L"SECURITY DATABASE   - <\\\\STR:00000009:80:$SDS>   <\\\\BIN:00000009:80:$SDS>");
	PrintAnyString(0, 0, L"SECURITY HASH INDEX - <\\\\STR:00000009:a0:$SDH>   <\\\\BIN:00000009:a0:$SDH>");
	PrintAnyString(0, 0, L"SECURITY INDEX      - <\\\\STR:00000009:a0:$SII>   <\\\\BIN:00000009:a0:$SII>");
	PrintAnyString(0, 0, L"VOLUME BITMAP       - <\\\\STR:00000006:80:>       <\\\\BIN:00000006:80:>");
	PrintAnyString(0, 0, L"==============================================================================");
	

	CloseOutput();
	return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) ULONG NtfsGetNamedFile(WCHAR ** szFoundFile, ULONGLONG * pThisFrs, ULONG * pProgress, WCHAR * Extension, BOOL OnlyDeleted)
{
	RW_BUFFER BaseMftRecord, MftRunBuffer, FrsBuffer, FileName;
	UCHAR Flags = 0;
	BOOL Isdeleted, IsFolder;
	ULONGLONG MftFrs = 0, Frs = 0;
	DWORD Error = 0;
	WCHAR szFilename[1024];
	double DecimalProgress = 0;
	
	// Allocate the memory for CLR to use for marshaling.
	*szFoundFile = (WCHAR *) CoTaskMemAlloc(2048);
	**szFoundFile = 0;

	// ULONGLONG * pThisFrs = (ULONGLONG*)CoTaskMemAlloc(sizeof(ULONGLONG));
	// ULONG * pProgress = (ULONG*)CoTaskMemAlloc(sizeof(ULONG));
	
	/*
	Find MFT Size and total records.
	*/
	ERROR_EXIT(pVolume->ReadFrsRaw(&BaseMftRecord, (PMFT_SEGMENT_REFERENCE) &MftFrs))
		PATTRIBUTE_RECORD_HEADER MftDataAttribute = (PATTRIBUTE_RECORD_HEADER)pVolume->GetAttributeRecordPointerByType(&BaseMftRecord, $DATA, L"", 0);
	LONG FileRecordSize = pVolume->_VolumeData.BytesPerFileRecordSegment;
	ULONGLONG TotalMftBytes = BYTES_REMAINING_IN_NR_ALLOCATION(MftDataAttribute, 0);
	ULONGLONG TotalFileRecords = TotalMftBytes / FileRecordSize;
	ULONGLONG ThisMftOffset = *pThisFrs * FileRecordSize;
	LONG Bytes = MAX_READ;
	FrsBuffer.BUFF_CreateManagedBuffer(FileRecordSize);

	Error = pVolume->ReadAttributeByteRange(&MftRunBuffer, &BaseMftRecord, $DATA, L"", ThisMftOffset, Bytes, FLIO_READ_PAST_VDL);
	if (Error == NO_ERROR || Error == ERROR_HANDLE_EOF)
	{
		for (LONG SubRecord = 0; SubRecord < (MftRunBuffer._NumberOfBytes / FileRecordSize); SubRecord++)
		{
			// Simulate a read by duplicating a section of the larger read buffer.
			FrsBuffer.BUFF_CopyBufferExt(&MftRunBuffer, SubRecord * FileRecordSize, 0, FileRecordSize);
			// Reading file records from the stream doesn't clean up USA's, so do it now.
			pVolume->CleanupSequenceArray(&FrsBuffer);

			// If filename is there, get it.
			if (pVolume->ReadFileNameAttribute(&FileName, &FrsBuffer, 0) == NO_ERROR)
			{

				PFILE_RECORD_SEGMENT_HEADER pFrsHeader = (PFILE_RECORD_SEGMENT_HEADER)FrsBuffer._pBuffer;

				MFT_SEGMENT_REFERENCE ThisFrsNumber = { 0 };
				ThisFrsNumber.SegmentNumberHighPart = pFrsHeader->SegmentNumberHighPart;
				ThisFrsNumber.SegmentNumberLowPart = pFrsHeader->SegmentNumberLowPart;

				PFILE_NAME pF = (PFILE_NAME)FileName._pBuffer;
				WCHAR * szScratchPath = CreateNameString(pF->FileName, pF->FileNameLength);
				wcscpy(szFilename, szScratchPath);
				delete[] szScratchPath;

				_wcslwr(szFilename);
				_wcslwr(Extension);

				if (wcsstr(szFilename, Extension))
				{
					Isdeleted = (IS_FLAG_SET(pFrsHeader->Flags, FILE_RECORD_SEGMENT_IN_USE)) ? FALSE : TRUE;
					IsFolder = (IS_FLAG_SET(pFrsHeader->Flags, FILE_NAME_INDEX_PRESENT)) ? TRUE : FALSE;

					if ((OnlyDeleted == 1 && Isdeleted == 1) || (OnlyDeleted == 0))
					{
						if (IS_FLAG_SET(pFrsHeader->Flags, FILE_NAME_INDEX_PRESENT))
						{
							wsprintf(*szFoundFile, L"%s %32s (%lu bytes) | FRS <\\\\FRS:%08I64x>  FOLDER <\\\\DIR:%08I64x>",
								(Isdeleted) ? L"DELETED" : L"       ",
								szFilename,
								pF->Info.FileSize,
								NtfsFullSegmentNumber(&ThisFrsNumber),
								NtfsFullSegmentNumber(&pF->ParentDirectory));
							*pThisFrs = NtfsFullSegmentNumber(&ThisFrsNumber);
							DecimalProgress = (DOUBLE)(*pThisFrs * 100 / TotalFileRecords);
							*pProgress = (ULONG) DecimalProgress;
							return Error;
						}
						else
						{
							wsprintf(*szFoundFile, L"%s %20s (%9lu bytes) | FRS <\\\\FRS:%08I64x>  FOLDER <\\\\DIR:%08I64x>  SAVE <\\\\SAVE:%08I64x:%s>",
								(Isdeleted) ? L"DELETED" : L"       ",
								szFilename,
								pF->Info.AllocatedLength,
								NtfsFullSegmentNumber(&ThisFrsNumber),
								NtfsFullSegmentNumber(&pF->ParentDirectory),
								NtfsFullSegmentNumber(&ThisFrsNumber),
								szFilename);
							DecimalProgress = (DOUBLE)(*pThisFrs * 100 / TotalFileRecords);
							*pProgress = (ULONG)DecimalProgress;
							return Error;
						}
					}
				}
			}
			DecimalProgress = (DOUBLE)(*pThisFrs * 100 / TotalFileRecords);
			*pProgress = (ULONG)DecimalProgress;
			(*pThisFrs)++;
		} // For Loop
	}
	return Error;
}


VOID wPrintErrorDetails(DWORD Win32Error)
{
	WCHAR * lpMsgBuf;
	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		Win32Error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL))
	{
		wprintf(lpMsgBuf);
		LocalFree(lpMsgBuf);
	}
	else
	{
		wprintf(L"\nError %lu\n\n", Win32Error);
	}
}
