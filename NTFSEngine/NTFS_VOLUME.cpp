#include "StdAfx.h"

#define CHUNK_SIZE (DWORD) 0x40000
#define VOLUME_DEBUGGING

#define DF_ERROR 1
#define DF_WARN  2
#define DF_INFO  4

VOID 
NTFS_VOLUME::VolDbgPrint(
	    LONG			MessageImportance,
		CHAR			* FunctionName,
		LONG			Line,
		CHAR			* szFormat,
		...
		)
{
	VolumeDebugFlags = 0xff;
	if (IS_FLAG_SET(VolumeDebugFlags, MessageImportance))
	{		
		CHAR * Output = new CHAR[MAX_PATH];
		va_list ArgList;
		size_t StringLen = strlen(FunctionName) + strlen(szFormat) + 16;
		CHAR * FormatString  = new CHAR [StringLen];
		sprintf(FormatString, "%s (%lu) : %s", FunctionName, Line, szFormat);
		va_start(ArgList, szFormat );
		vsprintf(Output, FormatString, ArgList);
		OutputDebugStringA(Output);
		delete [] FormatString;
		delete[] Output;
	}
	
}

 
NTFS_VOLUME::NTFS_VOLUME(VOID)
{
	VolDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	ClearVolumeMemberVariables();
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT\n");
}

NTFS_VOLUME::NTFS_VOLUME(LONG DebugFlags)
{
	VolumeDebugFlags = DebugFlags;
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	ClearVolumeMemberVariables();
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT\n");
}

NTFS_VOLUME::~NTFS_VOLUME(VOID)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	CloseVolumeHandle();
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT\n");
}

VOID 
NTFS_VOLUME::ClearVolumeMemberVariables()
{	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	_MaxRead = MAX_READ;
	NtfsSetSegmentNumber(&_LastReadFrs,-1,-1);
	_hDevice=INVALID_HANDLE_VALUE;
	_szDeviceName[0] = L'\0';
	RtlZeroMemory((PVOID)&_VolumeData,sizeof(NTFS_VOLUME_DATA_BUFFER));
	RtlZeroMemory((PVOID)&_Geometry,sizeof(DISK_GEOMETRY));
	_Verbose = FALSE;
}

DWORD 
NTFS_VOLUME::OpenVolumeHandle(IN  LPCWSTR szDeviceName,
								  IN  LONGLONG RelativeSector
								  )
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - Volume Name \"%S\"\n", szDeviceName);
	DWORD Error;
	if (_hDevice!=INVALID_HANDLE_VALUE)
	{
		Error = ERROR_SUCCESS;
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error 0x%lx\n", Error);
		return Error;
	}

	_hDevice = CreateFile(
		szDeviceName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, 
		OPEN_EXISTING, 
		FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, 
		NULL);

	Error=GetLastError();
    	
	if (_hDevice==INVALID_HANDLE_VALUE || Error)
	{
		CloseVolumeHandle();
		Error = ERROR_INVALID_NAME;
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	_RawRelativeSector=RelativeSector; 
	wcscpy(_szDeviceName,szDeviceName);
	
	Error = GetDiskGeometry();
	if (Error)
	{
		// If the geometry could not be detected, this could be a file on the disk.
		LARGE_INTEGER FileSize;		
		if (!GetFileSizeEx(_hDevice,&FileSize))
		{			
			CloseVolumeHandle();
			Error = GetLastError();
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;
		}
		// If szDeviceName is a file, let's fake the geometry to allow the use of a file as a volume.
        _Geometry.BytesPerSector=0x200;
		_Geometry.MediaType=FixedMedia;
		_Geometry.SectorsPerTrack=63;
		_Geometry.TracksPerCylinder=256;
		_Geometry.Cylinders.QuadPart=FileSize.QuadPart/_Geometry.BytesPerSector/_Geometry.SectorsPerTrack/_Geometry.TracksPerCylinder;
	}
	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::CloseVolumeHandle()
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error = ERROR_SUCCESS;
	if (_hDevice!=INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle(_hDevice))
		{
			Error=GetLastError();
		}
	}
	ClearVolumeMemberVariables();
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::GetNtfsVolumeInformation(
								IN  LONG RelativeSector)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD					Error;
	RW_BUFFER				BootSector;
	PACKED_BOOT_SECTOR		* pNtfsBpb;
	BIOS_PARAMETER_BLOCK	BiosParameterBlock;
	RW_BUFFER				Buffer;

	// Read the boot sector.
	Error = ReadNtfsSectors(&BootSector,0,1);
	if (Error) 
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}
	pNtfsBpb = (PACKED_BOOT_SECTOR*) BootSector._pBuffer;
	NtfsUnpackBios(&BiosParameterBlock, &pNtfsBpb->PackedBpb);

	if (BiosParameterBlock.Fats || BiosParameterBlock.BytesPerSector == 0 || BiosParameterBlock.RootEntries) 
	{
		Error = ERROR_UNRECOGNIZED_VOLUME;
		VolDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "ERROR_UNRECOGNIZED_VOLUME\n", Error);
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	// This is what we need to init the volume for simple access.
	BiosParameterBlock.HiddenSectors = RelativeSector;
	Error = SetVolumeDataParameters(&BiosParameterBlock, pNtfsBpb, 0);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);		
	return Error;
}


DWORD 
NTFS_VOLUME::SetVolumeDataParameters(
							IN  PBIOS_PARAMETER_BLOCK pBpb,
							IN  PACKED_BOOT_SECTOR *  pNtfsBpb,
							IN  LONG Flags)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error;
	
	
	LONG BytesPerSector, BytesPerCluster;
	PATTRIBUTE_RECORD_HEADER pAttribute;
	MFT_SEGMENT_REFERENCE MftSegmentNumber;

	_RawRelativeSector						= pBpb->HiddenSectors;
	BytesPerSector							= _Geometry.BytesPerSector;
	BytesPerCluster							= BytesPerSector * pBpb->SectorsPerCluster;
	_VolumeData.VolumeSerialNumber.QuadPart = pNtfsBpb->SerialNumber;
	_VolumeData.NumberSectors.QuadPart		= pNtfsBpb->NumberSectors;
	_VolumeData.TotalClusters.QuadPart		= pNtfsBpb->NumberSectors / pBpb->SectorsPerCluster;
	_VolumeData.BytesPerSector              = BytesPerSector;
	_VolumeData.BytesPerCluster             = BytesPerCluster; 
	_VolumeData.ClustersPerFileRecordSegment = pNtfsBpb->ClustersPerFileRecordSegment;

	if (pNtfsBpb->ClustersPerFileRecordSegment == 0)
	{
		// We shouldn't go here.
		wprintf(L"ERROR:  Your Clusters per Frs equal to zero.\n\n");
		DebugBreak();
	} 
	else if (pNtfsBpb->ClustersPerFileRecordSegment < 0)
	{
		// Cluster size is larger than FRS size, so value is negative.
		// Use negative bit shift method.
		_VolumeData.BytesPerFileRecordSegment =  1 << (-1 * pNtfsBpb->ClustersPerFileRecordSegment);
	}
	else
	{
		// Cluster size is smaller than FRS size, so ClustersPerFileRecordSegment is either 1 or 2.
		_VolumeData.BytesPerFileRecordSegment =  pNtfsBpb->ClustersPerFileRecordSegment * _VolumeData.BytesPerCluster;
	}
	
	_VolumeData.MftStartLcn.QuadPart        = pNtfsBpb->MftStartLcn;
	_VolumeData.Mft2StartLcn.QuadPart       = pNtfsBpb->Mft2StartLcn;

	if (!IS_FLAG_SET(Flags, VOL_RAWMFT))
	{
		//
		// Get the mft allocated length now that we know where the $MFT is.
		//
		NtfsSetSegmentNumber(&MftSegmentNumber,0,MASTER_FILE_TABLE_NUMBER);	

		Error = ReadSystemFrsRaw(&_MftRecord, &MftSegmentNumber);
		if (Error) 
		{		
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;
		}	
		
		pAttribute = GetAttributeRecordPointerByType(&_MftRecord,$DATA,NULL,0);
		if (pAttribute == NULL)
		{
			Error = GetLastError();
			return Error;
		}

		_VolumeData.MftValidDataLength.QuadPart = pAttribute->Form.Nonresident.ValidDataLength;
	}

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::SetRwPointer (	IN  LCN BlockNumber, 
							IN  LONGLONG BytesPerBlock)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error = ERROR_SUCCESS;
	LARGE_INTEGER nByteOffset, nNewOffset;
		
	nByteOffset.QuadPart = BytesPerBlock * BlockNumber + _RawRelativeSector * _Geometry.BytesPerSector;

	if (!SetFilePointerEx(_hDevice, nByteOffset, &nNewOffset, FILE_BEGIN))
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"  FilePointer offset = 0x%016I64x\n", nByteOffset.QuadPart);
		Error = GetLastError();
	}
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD NTFS_VOLUME::ReadNtfsBlocksUsaAligned (
							OUT PRW_BUFFER pBuffer, 
							IN  LCN Offset, 
							OUT  DWORD * pBytes)
{
	DWORD Error;
	DWORD NumberOfBlocks;
	LCN BlockNumber;

	BlockNumber = Offset / USA_SECTOR_SIZE;

	// Statistics say this is the best number to pick.
	if (*pBytes > (DWORD) _MaxRead)
	{
		*pBytes = _MaxRead;
	}

	NumberOfBlocks = *pBytes / USA_SECTOR_SIZE;

	Error = ReadNtfsBlocks(pBuffer, BlockNumber, NumberOfBlocks, USA_SECTOR_SIZE);
	if (pBuffer->_NumberOfBytes)
	{				
		PMULTI_SECTOR_HEADER pHeader = (PMULTI_SECTOR_HEADER) pBuffer->_pBuffer;

		if (IsValidMultiSectorHeader(pHeader))
		{	
			// This is a block containing a USA.  
			NumberOfBlocks = (pHeader->SequenceArraySize - 1);
			*pBytes = NumberOfBlocks * USA_SECTOR_SIZE;
			
			// Adjust the read size.
			if (pBuffer->_AllocatedLength > *pBytes)
			{
				// Our buffer is larger than the atomic unit.  Fake a smaller size.
				pBuffer->_AllocatedLength = *pBytes;
				pBuffer->_NumberOfBytes = *pBytes; 				 
			}
			else
			{
				// Our read was too small.  Read it again.
				Error = ReadNtfsBlocks(pBuffer, BlockNumber, NumberOfBlocks, USA_SECTOR_SIZE);
			}
			// Clean up the multi-sector header before using the read buffer.
			CleanupSequenceArray(pBuffer);
		}
		else
		{
			// If we're reading a generic stream, 256KB works.
			*pBytes = VACB_MAPPING_GRANULARITY;
			Error = ReadNtfsBlocks(pBuffer, BlockNumber, *pBytes / USA_SECTOR_SIZE, USA_SECTOR_SIZE);
		}
	}

	return Error;
}

DWORD 
NTFS_VOLUME::ReadNtfsBlocks(OUT PRW_BUFFER pBuffer, 
									  IN  LCN BlockNumber, 
									  IN  DWORD NumberOfBlocks, 
									  IN  DWORD BytesPerBlock)
{	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - BLK 0x%I64x (%020I64u) QTY 0x%lx SIZE 0x%lx\n", BlockNumber, BlockNumber, NumberOfBlocks, BytesPerBlock);
	LONG i;
	DWORD Error, BytesReturned;
	LONGLONG BytesRemaining;

	Error = pBuffer->BUFF_CreateManagedBuffer(NumberOfBlocks * BytesPerBlock);
	if (Error) 
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	Error = SetRwPointer(BlockNumber, BytesPerBlock);
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	pBuffer->_NumberOfBytes = 0;

	for (BytesRemaining = pBuffer->_AllocatedLength, BytesReturned=0, i=0;
		 BytesRemaining;
		 BytesRemaining -= BytesReturned , i++)
	{
        if (!ReadFile(_hDevice, 
						ADD_PTR(pBuffer->_pBuffer,i*CHUNK_SIZE),
						(DWORD) min(CHUNK_SIZE, BytesRemaining),
						&BytesReturned,
						NULL))
		{
			Error=GetLastError();
			break;
		}

		pBuffer->_NumberOfBytes += BytesReturned;

	}
		
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::WriteNtfsBlocks(OUT PRW_BUFFER pBuffer, 
									   IN  LCN BlockNumber, 
									   IN  LONGLONG BytesPerBlock)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - BLK 0x%I64x QTY 0x%lx SIZE 0x%lx\n", BlockNumber, pBuffer->_AllocatedLength/BytesPerBlock, BytesPerBlock);
	LONG i;
	DWORD Error;
	LONGLONG BytesRemaining;
	DWORD BytesReturned;

	Error = SetRwPointer(BlockNumber, BytesPerBlock);
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}
		
	for (BytesRemaining = pBuffer->_AllocatedLength, BytesReturned=0, i=0;
		 BytesRemaining ;
		 BytesRemaining -= BytesReturned , i++)
	{
        if (!WriteFile( _hDevice, 
						ADD_PTR(pBuffer->_pBuffer, i*CHUNK_SIZE),
						(DWORD) min(CHUNK_SIZE, BytesRemaining),
						&BytesReturned,
						NULL))
		{
			Error=GetLastError();
			break;
		}

		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"WriteFile returned 0x%lx bytes.\n", BytesReturned);
	}
	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadNtfsSectors(OUT PRW_BUFFER pBuffer, 
									   IN  LCN Lbn, 
									   IN  LONG NumberOfLbns)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - LBN 0x%I64x LEN 0x%lx\n", Lbn, NumberOfLbns);
	DWORD Error = ReadNtfsBlocks(pBuffer, Lbn, NumberOfLbns, _Geometry.BytesPerSector);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadNtfsClusters(OUT PRW_BUFFER pBuffer, 
										IN  LCN Lcn, 
										IN  LONG NumberOfLcns)
{	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - LCN 0x%I64x LEN 0x%lx\n", Lcn, NumberOfLcns);
	DWORD Error = ReadNtfsBlocks(pBuffer, Lcn, NumberOfLcns, _VolumeData.BytesPerCluster);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::WriteNtfsBytes(IN  PRW_BUFFER pBuffer, 
									  IN  LONGLONG ByteOffsetStart)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - OFF +0x%I64x LEN 0x%lx\n", ByteOffsetStart, pBuffer->_AllocatedLength);
	DWORD Error = WriteNtfsBlocks(pBuffer, ByteOffsetStart, pBuffer->_AllocatedLength);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::WriteNtfsSectors(OUT PRW_BUFFER pBuffer, 
										IN  LBN Lbn)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - LBN 0x%I64x LEN 0x%lx\n", Lbn, pBuffer->_AllocatedLength/_Geometry.BytesPerSector);
	DWORD Error = WriteNtfsBlocks(pBuffer, Lbn, _Geometry.BytesPerSector);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::WriteNtfsClusters(OUT PRW_BUFFER pBuffer, 
										 IN  LCN Lcn)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - LCN 0x%I64x LEN 0x%lx\n", Lcn, pBuffer->_AllocatedLength/_VolumeData.BytesPerCluster);
	DWORD Error = WriteNtfsBlocks(pBuffer, Lcn, _VolumeData.BytesPerCluster);
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadSystemFrsRaw(OUT PRW_BUFFER pBuffer, 
										IN  PMFT_SEGMENT_REFERENCE pFileId)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - FRS 0x%I64x\n", NtfsFullSegmentNumber(pFileId));
	DWORD Error;
	
	LONG SectorsPerCluster = _VolumeData.BytesPerCluster / _Geometry.BytesPerSector;
	LONG SectorsPerFrs = _VolumeData.BytesPerFileRecordSegment / _Geometry.BytesPerSector;
	LONGLONG ThisLbn = (_VolumeData.MftStartLcn.QuadPart * SectorsPerCluster) + (NtfsFullSegmentNumber(pFileId) * SectorsPerFrs);
	
	Error = ReadNtfsBlocks(pBuffer, ThisLbn, SectorsPerFrs, _Geometry.BytesPerSector);
	if (Error)
	{
		NtfsSetSegmentNumber(&_LastReadFrs, -1, -1);
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}
	
	NtfsSetSegmentNumber(&_LastReadFrs, pFileId->SegmentNumberHighPart, pFileId->SegmentNumberLowPart);
	
	PLONG pSignature = (PLONG) pBuffer->_pBuffer;

	if (*pSignature == 'ELIF')
	{
		CleanupSequenceArray(pBuffer);
	}
	else
	{
		pBuffer->BUFF_DeleteManagedBuffer();
		Error = ERROR_NOT_FOUND;
	}

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadFrsRaw(		OUT PRW_BUFFER pBuffer, 
								IN  PMFT_SEGMENT_REFERENCE pFileId)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - FRS 0x%I64x\n", NtfsFullSegmentNumber(pFileId));
	DWORD Error;
	LONGLONG MftOffset		= NtfsFullSegmentNumber(pFileId) * _VolumeData.BytesPerFileRecordSegment;
	LONG SizeOfFileRecord  = _VolumeData.BytesPerFileRecordSegment;

	NtfsSetSegmentNumber(&_LastReadFrs,-1,-1);
    // System records exist in the first mft fragment.
	if (NtfsFullSegmentNumber(pFileId) < (LONGLONG) FIRST_USER_FILE_NUMBER)
	{
		if (NtfsFullSegmentNumber(pFileId) == (LONGLONG)(MASTER_FILE_TABLE_NUMBER) && _VolumeData.MftValidDataLength.QuadPart)
		{
			Error = pBuffer->BUFF_CloneBuffer(&_MftRecord);
			if (Error)
			{
				VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
				return Error;			
			}
		}
		else
		{
			Error = ReadSystemFrsRaw(pBuffer, pFileId);
			if (Error)
			{
				VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
				return Error;			
			}
		}
	}
	else {
		// We can read all other MFT records by using this method.
		Error = ReadAttributeByteRange(pBuffer, &_MftRecord, $DATA, NULL, MftOffset, SizeOfFileRecord, 0);
		if (Error)
		{
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;			
		}
		// This read method does not clean up the sequence array.
		PLONG pSignature = (PLONG) pBuffer->_pBuffer;
		if (*pSignature == 'ELIF')	
		{
			CleanupSequenceArray(pBuffer);
		}
		else if (*pSignature != 0x0000)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			Error = ERROR_NOT_FOUND;
		}
	}
	// If success save the file ID.	
	if (!Error)
	{
		NtfsSetSegmentNumber(&_LastReadFrs, pFileId->SegmentNumberHighPart, pFileId->SegmentNumberLowPart);		
	}
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::WriteFrsRaw(	OUT PRW_BUFFER pBuffer, 
							IN  PMFT_SEGMENT_REFERENCE pFileId)
{	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - FRS 0x%I64x\n", NtfsFullSegmentNumber(pFileId));

	DWORD Error = ERROR_CALL_NOT_IMPLEMENTED;
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

BOOL  
NTFS_VOLUME::IsValidMultiSectorHeader(
							IN  PMULTI_SECTOR_HEADER pHeader
							)
{	
	ULONG Size;
	BOOL Valid = FALSE;

	if (pHeader == NULL)
	{
		return Valid;
	}

	Size = (pHeader->SequenceArraySize - 1 ) * USA_SECTOR_SIZE;

	switch (pHeader->Signature)
	{
	case 'ELIF':
		{
			//FileRecord
			PFILE_RECORD_SEGMENT_HEADER pMft = (PFILE_RECORD_SEGMENT_HEADER) pHeader;

			if ((pMft->BytesAvailable == _VolumeData.BytesPerFileRecordSegment &&
				pMft->FirstAttributeOffset < pMft->BytesAvailable &&
				pMft->FirstFreeByte < pMft->BytesAvailable &&
				pMft->FirstFreeByte < Size &&
				pMft->FirstAttributeOffset < Size))
			{
				Valid = TRUE;
			}
		}
		break;
	case 0x0000:
		Valid = TRUE;
		break;
	case 'XDNI':
		{
			PINDEX_ALLOCATION_BUFFER pIab = (PINDEX_ALLOCATION_BUFFER) pHeader;
			if (pIab->IndexHeader.FirstFreeByte < Size &&
				pIab->IndexHeader.FirstIndexEntry < Size)
			{
				Valid = TRUE;
			}
		}
		break;
	case 'DRCR':
		Valid = TRUE;
		break;
	case 'RTSR':
		Valid = TRUE;
		break;
	case 'DKHC':
		// TODO: Add additional checks here.
		Valid = TRUE;
		break;	
	case 0xffffffff:
		Valid = FALSE;
		break;	
	default:
		break;
	}
		
	return Valid;
}


VOID 
NTFS_VOLUME::UpdateSequenceArray(PRW_BUFFER pBuffer)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	PUSHORT pUsa;
	PUSHORT pSectorEnd;

	PMULTI_SECTOR_HEADER pHeader   = (PMULTI_SECTOR_HEADER) pBuffer->_pBuffer;

	if (IsValidMultiSectorHeader(pHeader))
	{

		if (pHeader->Signature == -1)
		{
			pHeader->SequenceArrayOffset = 0x30;
			pHeader->SequenceArraySize   = 0x3;
		}

		pUsa   = (PUSHORT) ADD_PTR(pBuffer->_pBuffer, pHeader->SequenceArrayOffset);

		pUsa[0]++;
		pSectorEnd  = (PUSHORT) pBuffer->_pBuffer + 255;

		for (USHORT UsaEntry=1; UsaEntry < pHeader->SequenceArraySize; UsaEntry++)
		{
			pUsa[UsaEntry]=*pSectorEnd;
			*pSectorEnd =  pUsa[0];
			pSectorEnd  +=  256;  
		}
	}
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT\n");
}

VOID 
NTFS_VOLUME::CleanupSequenceArray(PRW_BUFFER pBuffer)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	PUSHORT pUsa;
	PUSHORT pDest;
	LONGLONG SectorsRemaining;
	PMULTI_SECTOR_HEADER pHeader   = (PMULTI_SECTOR_HEADER) pBuffer->_pBuffer;

	// Regardless of physical/logical sector size, USA assumes 512 bytes.
	DWORD   BytesPerSector = USA_SECTOR_SIZE;
	
	try {
		if (IsValidMultiSectorHeader(pHeader))
		{
			SectorsRemaining = min(pHeader->SequenceArraySize - 1, 
								pBuffer->_AllocatedLength / USA_SECTOR_SIZE);

			if (SectorsRemaining > 0)
			{

				while (SectorsRemaining)
				{
					pUsa = (PUSHORT)ADD_PTR(pHeader, pHeader->SequenceArrayOffset);
					pDest = (PUSHORT)pHeader + (BytesPerSector / sizeof(USHORT)) - 1;

					USHORT SeqOffset = pHeader->SequenceArrayOffset;
					USHORT ArraySize = pHeader->SequenceArraySize;

					for (USHORT UsaEntry = 1; UsaEntry < pHeader->SequenceArraySize; UsaEntry++)
					{
						if (*pDest != pUsa[0])
						{
							VolDbgPrint(DF_WARN, __FUNCTION__, __LINE__, "****** WARNING: Update Sequence Mismatch Found!!!\n");
							VolDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT\n");
							return;
						}

						*pDest = pUsa[UsaEntry];
						pDest += (BytesPerSector / sizeof(USHORT));
						SectorsRemaining--;
					}
					pHeader = (PMULTI_SECTOR_HEADER)ADD_PTR(pHeader, BytesPerSector*(ArraySize - 1));
				}
			}
		}
	} 
	catch (...)
	{
		VolDbgPrint(DF_WARN,__FUNCTION__,__LINE__,"Warning!  Invalid MULTI_SECTOR_HEADER.\n");
	}
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT\n");
}

DWORD 
NTFS_VOLUME::Initialize(	OUT WCHAR * szDeviceName, 
							IN  LONG RelativeSector, 
							IN  LONGLONG NumberSectors, 
							IN  LCN MftStartLcn, 
							IN  LCN Mft2StartLcn,
							IN  LONG SectorsPerCluster)
{
	return	Initialize(		szDeviceName, 
							RelativeSector, 
							NumberSectors, 
							MftStartLcn, 
							Mft2StartLcn,
							SectorsPerCluster,
							0);
}

DWORD 
NTFS_VOLUME::Initialize(	OUT WCHAR * szDeviceName, 
							IN  LONG RelativeSector, 
							IN  LONGLONG NumberSectors, 
							IN  LCN MftStartLcn, 
							IN  LCN Mft2StartLcn,
							IN  LONG SectorsPerCluster,
							IN  LONG Flags)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	BIOS_PARAMETER_BLOCK	Bpb;
	PACKED_BOOT_SECTOR		NtfsBpb;
	
	DWORD Error=LoadCompressionApi();
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	Error = OpenVolumeHandle(szDeviceName, RelativeSector);
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}
	//
	// This needs to be initialized because GetNtfsVolumeInformation reads the
	// $MFT base record and we have to know where the volume starts.
	//
	if (!NumberSectors || !MftStartLcn || !SectorsPerCluster )
	{
		Error = GetNtfsVolumeInformation(RelativeSector);
	}
	else
	{		
		RtlZeroMemory(&Bpb,sizeof(BIOS_PARAMETER_BLOCK));
		RtlZeroMemory(&NtfsBpb,sizeof(PACKED_BOOT_SECTOR));
		// Use the supplied parameters.
		Bpb.BytesPerSector		= (USHORT) _Geometry.BytesPerSector;
		Bpb.HiddenSectors		= (USHORT) RelativeSector;
		Bpb.Heads				= (USHORT) _Geometry.TracksPerCylinder;
		Bpb.Media				= 0xf8;
		Bpb.SectorsPerCluster	= (UCHAR) SectorsPerCluster;
		Bpb.SectorsPerTrack		= (USHORT) _Geometry.SectorsPerTrack;
		NtfsBpb.MftStartLcn		= MftStartLcn;
		NtfsBpb.Mft2StartLcn	= Mft2StartLcn;
		NtfsBpb.NumberSectors	= NumberSectors;
		
		Error = SetVolumeDataParameters(&Bpb, &NtfsBpb, Flags);
	}

	// If this fails, don't leak the volume handle.
	if (Error)
	{	
		CloseVolumeHandle();
	}
	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::InitializeRaw(	IN  WCHAR * szDeviceName, 
							IN  LONGLONG RelativeSector)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error;
	Error = OpenVolumeHandle(szDeviceName, RelativeSector);
	if (Error)
	{
		CloseVolumeHandle();
	}
	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::GetDiskGeometry()
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Bytes;
	DWORD Error = ERROR_SUCCESS;

	if (!DeviceIoControl(
		_hDevice,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&_Geometry,
		sizeof(DISK_GEOMETRY),
		&Bytes,
		NULL))
	{		
		Error = GetLastError();
	}
    
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}


PATTRIBUTE_RECORD_HEADER NTFS_VOLUME::GetFirstAttributeRecordPointer(IN  PRW_BUFFER pBuffer)
{	
    PATTRIBUTE_RECORD_HEADER pAttribute;
	PFILE_RECORD_SEGMENT_HEADER pFRS = (PFILE_RECORD_SEGMENT_HEADER) (pBuffer->_pBuffer);
	if ( pFRS->FirstAttributeOffset && (pFRS->FirstAttributeOffset < MAX_FRS_SIZE) )
    {
		pAttribute = (PATTRIBUTE_RECORD_HEADER) NtfsFirstAttribute(pFRS);
    }
    else
    {
        pAttribute = NULL;
    }
    return pAttribute;
}

PATTRIBUTE_RECORD_HEADER NTFS_VOLUME::GetNextAttributeRecordPointer(IN  PATTRIBUTE_RECORD_HEADER pThisAttribute)
{
    PATTRIBUTE_RECORD_HEADER pAttribute = pThisAttribute;
	
	pAttribute = (PATTRIBUTE_RECORD_HEADER) NtfsGetNextRecord(pThisAttribute);

	if (pThisAttribute->RecordLength > 0x1000 || pThisAttribute->RecordLength == 0)
	{
		VolDbgPrint(4,__FUNCTION__,__LINE__,"****** WARNING: Invalid attribute record length.\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
    
	if (pAttribute->TypeCode == $END) 
	{
		SetLastError(NO_ERROR);
        pAttribute = NULL;
	}

	return pAttribute;
}

PATTRIBUTE_RECORD_HEADER NTFS_VOLUME::GetFileNameAttributerRecordPointer(OUT PRW_BUFFER pBuffer, 
																			  IN  UCHAR Flags)
{		    
	PATTRIBUTE_RECORD_HEADER pAttribute = GetFirstAttributeRecordPointer(pBuffer);
	
	while (pAttribute)
	{
		if (IsMatchingAttribute(pAttribute, $FILE_NAME, NULL, 0))
		{
			PFILE_NAME pFileName = (PFILE_NAME) ADD_PTR(pAttribute,pAttribute->Form.Resident.ValueOffset);
			if (Flags == 0 || IS_FLAG_SET(pFileName->Flags, Flags))
			{
				return pAttribute;
			}
		}
		pAttribute = GetNextAttributeRecordPointer(pAttribute);
	}

	SetLastError(ERROR_NOT_FOUND);
    return NULL;
}

PATTRIBUTE_RECORD_HEADER NTFS_VOLUME::GetAttributeRecordPointerByType(OUT RW_BUFFER * pBuffer, 
																		   IN  ATTRIBUTE_TYPE_CODE Type, 
																		   IN  WCHAR * szName,
																		   IN  VCN LowestVcn)
{
	if (pBuffer->_AllocatedLength==0)
	{
		SetLastError(ERROR_NOT_FOUND);
		return NULL;
	}
    
	PATTRIBUTE_RECORD_HEADER pAttribute = GetFirstAttributeRecordPointer(pBuffer);
	while (pAttribute)
    {
		if (IsMatchingAttribute(pAttribute, Type, szName, LowestVcn))
		{
			SetLastError(ERROR_SUCCESS);
			return pAttribute;
		}
		pAttribute = GetNextAttributeRecordPointer(pAttribute);
    }

	SetLastError(ERROR_NOT_FOUND);
    return NULL;
}

BOOL 
NTFS_VOLUME::IsMatchingAttribute(	OUT PATTRIBUTE_RECORD_HEADER pAttribute, 
											IN  ATTRIBUTE_TYPE_CODE Type, 
											IN  WCHAR * szName,
											IN  VCN	LowestVcn)
{
	size_t NameLength = 0;
	
	if (pAttribute->TypeCode == Type)
    {
		if (szName) 
		{
			NameLength = wcslen(szName);
		}
		else 
		{
			NameLength=0;
		}
		
		if	(NameLength == pAttribute->NameLength && !wcsncmp(szName, (WCHAR*) ADD_PTR(pAttribute, pAttribute->NameOffset), NameLength))

		{
			switch (pAttribute->FormCode)
			{
			case RESIDENT_FORM:
				return TRUE;
			case NONRESIDENT_FORM:
				if (pAttribute->Form.Nonresident.LowestVcn <= LowestVcn)
				{					
					return TRUE;
				}
				break;
			}
		}
    }

	return FALSE;
}

DWORD 
NTFS_VOLUME::ReadResidentAttributeFromAttributeInstance(	OUT PRW_BUFFER pBuffer, 
																	IN  PATTRIBUTE_RECORD_HEADER pAttribute, 
																	IN  LONGLONG StartOffset, 
																	IN  LONG Length)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error;
	RW_BUFFER AttributeBuffer;
	
	if (pAttribute->FormCode != RESIDENT_FORM)
	{
		Error = ERROR_INVALID_PARAMETER;
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	Error = AttributeBuffer.BUFF_CopyStdBuffer(
								ADD_PTR(pAttribute,pAttribute->Form.Resident.ValueOffset),
								pAttribute->Form.Resident.ValueLength);
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}
	
	Error = pBuffer->BUFF_CreateManagedBuffer(min(0x1000, AttributeBuffer._NumberOfBytes));
	if (Error)
	{
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	Error = pBuffer->BUFF_CopyBufferExt(
								&AttributeBuffer, 
								StartOffset, 
								0, 
								min(0x1000, AttributeBuffer._NumberOfBytes));

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}


DWORD 
NTFS_VOLUME::ReadChildFrsFromAttributeList(IN  PRW_BUFFER pAttributeListBuffer, 
													 IN  ATTRIBUTE_TYPE_CODE Type,
													 IN  WCHAR * szName, 
													 IN  VCN LowestVcn, 
													 OUT PRW_BUFFER pChildFrsBuffer, 
													 OUT PMFT_SEGMENT_REFERENCE pChildFileId, 
													 OUT PATTRIBUTE_RECORD_HEADER * ppAttributeMatch)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error;
	PATTRIBUTE_LIST_ENTRY pEntry;

	Error = ERROR_NOT_FOUND;
	pChildFrsBuffer->BUFF_DeleteManagedBuffer();
	
	NtfsSetSegmentNumber(pChildFileId, -1,-1);
	pEntry    = (PATTRIBUTE_LIST_ENTRY) pAttributeListBuffer->_pBuffer;

	if (!pEntry)
	{
		return ERROR_INSUFFICIENT_BUFFER;
	}

	while (pEntry->RecordLength)
	{		
		// Do a light search before wasting time on the heavy search.
		if (pEntry->AttributeTypeCode == Type)
		{
			Error = GetChildFrsIfMatchingAttrListEntry(pEntry, pChildFrsBuffer, pChildFileId, Type, szName, LowestVcn, ppAttributeMatch);
			// 8/7/15 part 2
			if (!Error)
				break;
		}
		pEntry = (PATTRIBUTE_LIST_ENTRY) NtfsGetNextRecord(pEntry);
	}
	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::GetChildFrsIfMatchingAttrListEntry(IN  PATTRIBUTE_LIST_ENTRY pEntry, 
														  OUT PRW_BUFFER pChildFrsBuffer, 
														  OUT PMFT_SEGMENT_REFERENCE pChildFileId, 
														  IN  ATTRIBUTE_TYPE_CODE Type, 
														  IN  WCHAR * szName, 
														  IN  VCN LowestVcn, 
														  OUT PATTRIBUTE_RECORD_HEADER *ppAttributeMatch)
{
	DWORD Error;
	PATTRIBUTE_RECORD_HEADER pAttribute;
	WCHAR * pName = (WCHAR *) ADD_PTR(pEntry, pEntry->AttributeNameOffset);
	
	pChildFrsBuffer->BUFF_DeleteManagedBuffer();
	NtfsSetSegmentNumber(pChildFileId, (SHORT) -1, -1);

	if (pEntry->AttributeTypeCode == Type)
	{
		if ((!pEntry->AttributeNameLength && !szName) || (szName && !wcsncmp(pName, szName, min(pEntry->AttributeNameLength, wcslen(szName))) ) )
		{
			if (LowestVcn >= pEntry->LowestVcn)
			{
				NtfsSetSegmentNumber(pChildFileId, pEntry->SegmentReference.SegmentNumberHighPart, pEntry->SegmentReference.SegmentNumberLowPart);
				Error = ReadFrsRaw(pChildFrsBuffer,pChildFileId);
				if (pChildFrsBuffer->_NumberOfBytes == 0)
				{					
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					return Error;
				}

				pAttribute	= GetAttributeRecordPointerByType(pChildFrsBuffer, Type, szName, LowestVcn);
				if (pAttribute==NULL)
				{
					Error = GetLastError();
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					return Error;
				}
				
				if (LowestVcn <= pAttribute->Form.Nonresident.HighestVcn)
				{
					//
					// A match was found.
					//
					*ppAttributeMatch = pAttribute;
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					return Error;			
				}
			}
		}
	}

	Error = ERROR_NOT_FOUND;
	pChildFrsBuffer->BUFF_DeleteManagedBuffer();
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}


DWORD 
NTFS_VOLUME::ReadFileNameAttribute(OUT PRW_BUFFER pBuffer, 
											  IN  PRW_BUFFER pFrsBuffer, 
											  IN  UCHAR Flags)
{
	DWORD Error;
	PATTRIBUTE_RECORD_HEADER pAttribute = GetFileNameAttributerRecordPointer(pFrsBuffer, Flags);
	if (!pAttribute)
	{
		Error = ERROR_NOT_FOUND;
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	Error = ReadResidentAttributeFromAttributeInstance(pBuffer,pAttribute, 0, 0x1000);

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadAttributeByteRange(OUT PRW_BUFFER pBuffer, 
											   IN  PMFT_SEGMENT_REFERENCE pBaseFileId, 
											   IN  ATTRIBUTE_TYPE_CODE Type, 
											   IN  PWCHAR szName, 
											   IN  LONGLONG StartOffset, 
											   IN  LONG NumberOfBytes,
											   IN  LONG Flags)
{
	DWORD Error;
	RW_BUFFER BaseFrsBuffer;
	Error = ReadFrsRaw(&BaseFrsBuffer, pBaseFileId);
	if (Error==ERROR_SUCCESS)
	{
		Error = ReadAttributeByteRange(pBuffer, &BaseFrsBuffer, Type, szName, StartOffset, NumberOfBytes, Flags);
	}
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}

DWORD 
NTFS_VOLUME::ReadAttributeByteRangeUsaAligned(OUT PRW_BUFFER pBuffer, 
											   IN  PRW_BUFFER pBaseFrsBuffer, 
											   IN  ATTRIBUTE_TYPE_CODE Type, 
											   IN  PWCHAR szName, 
											   IN  LONGLONG StartOffset, 
											   OUT  LONG * pNumberOfBytes,
											   IN  LONG Flags)
{
	DWORD Error; 
	PMULTI_SECTOR_HEADER pHeader;

	Error = ReadAttributeByteRange(pBuffer,
							pBaseFrsBuffer, 
							Type, 
							szName, 
							StartOffset, 
							*pNumberOfBytes,
							Flags);

	pHeader = (PMULTI_SECTOR_HEADER ) pBuffer->_pBuffer;
	
	if (pBuffer->_AllocatedLength == 0)
		return ERROR_HANDLE_EOF;

	if (pHeader && IsValidMultiSectorHeader(pHeader) && pHeader->Signature != 0x0000)
	{
		*pNumberOfBytes = (pHeader->SequenceArraySize - 1) * USA_SECTOR_SIZE;		 

		if (*pNumberOfBytes < pBuffer->_NumberOfBytes)
		{
			pBuffer->_NumberOfBytes = (LONGLONG) *pNumberOfBytes;
			pBuffer->_AllocatedLength = (LONGLONG) *pNumberOfBytes;
		}
		else
		{
			Error = ReadAttributeByteRange(pBuffer, pBaseFrsBuffer, Type, szName, StartOffset, *pNumberOfBytes, Flags);
		}

		if (pBuffer->_NumberOfBytes == *pNumberOfBytes)
		{
			CleanupSequenceArray(pBuffer);
		}
	}	

	return Error;
}


DWORD 
NTFS_VOLUME::ReadAttributeByteRange(OUT PRW_BUFFER pBuffer, 
											   IN  PRW_BUFFER pBaseFrsBuffer, 
											   IN  ATTRIBUTE_TYPE_CODE Type, 
											   IN  PWCHAR szName, 
											   IN  LONGLONG StartOffset, 
											   IN  LONG NumberOfBytes,
											   IN  LONG Flags)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - TYPE 0x%lx OFFSET +0x%I64x LEN 0x%lx\n", Type, StartOffset, NumberOfBytes);
	DWORD Error;
	VCN ThisVcn;
	LONG BytesPerCluster, BytesRemaining;
	LONGLONG ThisOffset, VcnBufferOffset;
	RW_BUFFER AttrList, FirstInstanceBuffer, TempBuffer, ChildBuffer;
	MFT_SEGMENT_REFERENCE ChildFileId;
	PATTRIBUTE_RECORD_HEADER pAttribute, pBaseAttribute;
	MAPPING_PAIRS Extents(VolumeDebugFlags);
	//
	// Clear the output buffer.
	//
	pBuffer->BUFF_DeleteManagedBuffer();

	if (NumberOfBytes > _MaxRead || NumberOfBytes < 0)
	{
		NumberOfBytes = _MaxRead;
	}

	if (NumberOfBytes == 0)
	{
		Error = ERROR_SUCCESS;
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	BytesPerCluster = _VolumeData.BytesPerCluster;

	// Read the first matching attribute record from this frs or a child frs.
	Error = ReadAttributeList(&AttrList, pBaseFrsBuffer);
	if (AttrList._NumberOfBytes == 0)
	{
		// No $ATTRIBUTE_LIST is present.
		// Obtain the base attribute from the base file record.
		pAttribute=GetAttributeRecordPointerByType(pBaseFrsBuffer, Type, szName, 0);
		if (!pAttribute)
		{
			Error = GetLastError();
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;
		}
		Error = ChildBuffer.BUFF_CloneBuffer(pBaseFrsBuffer);
		if (ChildBuffer._AllocatedLength == 0)
			Error = ERROR_HANDLE_EOF;

		if (Error)
		{
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;
		}
	}
	else
	{
		if (Type == $ATTRIBUTE_LIST)
		{
			//Find the attribute list in the current record.
			pAttribute = GetAttributeRecordPointerByType(pBaseFrsBuffer, Type, szName, 0);
		}
		else
		{ 			
			// $ATTRIBUTE_LIST is present.
			// Obtain the base attribute from the list.
			Error = ReadChildFrsFromAttributeList(&AttrList, Type, szName, 0, &FirstInstanceBuffer, &ChildFileId, &pAttribute);
			if (Error)
			{
				VolDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
				return Error;
			}
		}
	}

	// At this point, we have a pointer to the first attribute instance.  
	// IMPORTANT:  This pointer is backed by the pBaseFrsBuffer or the FirstInstanceBuffer.
	pBaseAttribute = pAttribute;

	// If resident, read the byte range and we're done.
	if (pBaseAttribute->FormCode == RESIDENT_FORM)
	{
		
		NumberOfBytes=min(NumberOfBytes, 
						   (LONG) BYTES_REMAINING_IN_RES_ATTRIBUTE(pBaseAttribute, StartOffset));
		
		if (!BYTES_REMAINING_IN_RES_ATTRIBUTE(pBaseAttribute, 0)) 
		{
			Error = ERROR_SUCCESS;
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			return Error;
		}
		Error = ReadResidentAttributeFromAttributeInstance(pBuffer,pAttribute, StartOffset, NumberOfBytes);
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	// Obtain the maximum length from the first attribute instance.
	// This is the largest possible read size for the stream.
	if (NumberOfBytes >= _MaxRead ||
		NumberOfBytes > BYTES_REMAINING_IN_NR_ATTRIBUTE(pBaseAttribute,StartOffset)
		)
	{
		if (IS_FLAG_SET(FLIO_READ_PAST_VDL, Flags))
		{			
			NumberOfBytes = (LONG) min(NumberOfBytes, BYTES_REMAINING_IN_NR_ALLOCATION(pBaseAttribute,StartOffset));
		}
		else
		{
			NumberOfBytes = (LONG) min(NumberOfBytes, BYTES_REMAINING_IN_NR_ATTRIBUTE(pBaseAttribute,StartOffset));
		}

	}
	
	if (NumberOfBytes == 0)
	{
		Error = ERROR_HANDLE_EOF;
		pBuffer->BUFF_DeleteManagedBuffer();
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}


	// Set start offset and read length
	ThisOffset = StartOffset;

	// If we truncate this read because of max read size, then we will return ERROR_MORE_DATA.
	BytesRemaining = (LONG) min( (LONGLONG) NumberOfBytes, _MaxRead);

	// Read the child records and make a complete LCN/VCN mapping array of the attribute extents we're interested in.

	Extents.AppendRunsToArray(pBaseAttribute);			
	ThisVcn = 0;
	//
	// Cache all of the stream's VCN/LCN mappings.
	//	
	while (AttrList._pBuffer && !Error)
	{
		// Find and read the child record that defines the VCN range we want.
		Error = ReadChildFrsFromAttributeList(&AttrList, Type, szName, ThisVcn, &ChildBuffer, &ChildFileId,	&pAttribute);
		if (Error) 
		{
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			break;
		}
		Error = Extents.AppendRunsToArray(pAttribute);
		ThisVcn = pAttribute->Form.Nonresident.HighestVcn+1;
	}

	while (BytesRemaining)
	{
		LONGLONG HighestValidByteInStream;
		// Get the length of the entire stream.
		if (IS_FLAG_SET(FLIO_READ_PAST_VDL, Flags))
		{
			HighestValidByteInStream = BYTES_REMAINING_IN_NR_ALLOCATION(pBaseAttribute,1)-1;
		}
		else
		{
            HighestValidByteInStream = BYTES_REMAINING_IN_NR_ATTRIBUTE(pBaseAttribute,0)-1;
		}
		

		// Limit the number of bytes to the valid vcn range for the instance and the entire file.
		LONGLONG LastByteInRange	= ThisOffset + BytesRemaining - 1;
		LONGLONG FirstByteInRange	= ThisOffset;
		LastByteInRange = min(LastByteInRange, HighestValidByteInStream);

		// Compute the read size in terms of VCNs.
		LONG ThisReadSize = (LONG)(LastByteInRange - FirstByteInRange)+1;

		LONG VcnsToRead	= (LONG)(VCN_END((FirstByteInRange+ThisReadSize-1), BytesPerCluster) - VCN_START(FirstByteInRange, BytesPerCluster));

		ThisVcn = ThisOffset / BytesPerCluster;

		// Read the VCN range.
		Error = ReadAttributeVcnRange(&TempBuffer, &Extents, ThisVcn, VcnsToRead, Flags);
		if (Error) 
		{
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			break;
		}

		// Update the file pointers.
		VcnBufferOffset = OFFSET_WITHIN_VCN(FirstByteInRange, BytesPerCluster);
		ThisOffset		+= ThisReadSize;
		BytesRemaining	-= ThisReadSize;

		// If we read more than we asked for, something is wrong.
		if (BytesRemaining < 0)
		{			
			throw "IO alignment error.";
		}
		// Align this buffer for the append operation.
		if (VcnBufferOffset)
		{
			// Not VCN aligned.  Re-align the buffer and append to the existing buffer.
			Error = TempBuffer.BUFF_Realign(VcnBufferOffset);
			if ( Error )	
			{
				VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
				break;
			}
		}

		// Report the valid length to the temp buffer so we know how many bytes are valid.
		TempBuffer._NumberOfBytes = ThisReadSize;

		// If the read size is zero, then the read was out of the allocated range of the file.
		if (ThisReadSize == 0)
		{
			Error = ERROR_HANDLE_EOF;
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			break;
		}

		// VCN aligned.  Just append to the existing buffer.
		Error = pBuffer->BUFF_Append(&TempBuffer);
		if ( Error )
		{
			VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
			break;
		}

	}

	if (Error && Error != ERROR_HANDLE_EOF)
	{
		pBuffer->BUFF_DeleteManagedBuffer();
	}

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	
	return Error;
}


DWORD 
NTFS_VOLUME::ReadAttributeVcnRange (OUT	PRW_BUFFER pBuffer, 
														   IN   MAPPING_PAIRS * Extents,
														   IN	LONGLONG StartVcn, 
														   IN	LONG NumberOfClusters,
														   IN	LONG Flags)
{
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER - VCN 0x%I64x LEN 0x%lx\n", StartVcn, NumberOfClusters);
	//
	// Read a vcn range within a single attribute instance.
	//
	DWORD Error = ERROR_SUCCESS;
	VCN ThisVcn;
	LONG i, j;
	LONGLONG	ClustersRemaining;
	LONG		ThisChunkSize;
	LONGLONG	RunOffset;		
	VCN			MaxCompressionUnitVcn;
	LONG		CompressionChunkSize = 0;
	VCN			VcnsPerCompressionChunk = 0;
	
	LONG	CompressionUnit	= 0;
	USHORT	CompressionFormat = 0;
	BOOL	Sparse = FALSE;
	BOOL    Compressed = FALSE;
	LONG	RunStep = 1;

	PATTRIBUTE_RECORD_HEADER pAttribute = &Extents->_BaseAttribute;
	
	pBuffer->BUFF_DeleteManagedBuffer();

	if (IS_FLAG_SET(pAttribute->Flags, ATTRIBUTE_FLAG_SPARSE))	
	{
		Sparse = TRUE;
		CompressionChunkSize	= _VolumeData.BytesPerCluster << pAttribute->Form.Nonresident.CompressionUnit;
		VcnsPerCompressionChunk	= (VCN) 1 << pAttribute->Form.Nonresident.CompressionUnit;
	}
		
	// Detect the compression method.
	if (pAttribute->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK)
	{
		Compressed				= TRUE;
		CompressionFormat		= (pAttribute->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) + 1;
		VcnsPerCompressionChunk	= (VCN) 1 << pAttribute->Form.Nonresident.CompressionUnit;
		CompressionChunkSize	= (LONG) VcnsPerCompressionChunk * _VolumeData.BytesPerCluster;
	}

	//
	// I forgot how this code works. In any case it works so don't change it unless it breaks.
	//
	for (	i=0, ThisVcn=StartVcn, ClustersRemaining = NumberOfClusters;
			ClustersRemaining && i < Extents->_NumberOfRuns;
			i += RunStep)
	{		
		MaxCompressionUnitVcn = 0;
		
		if (Compressed)
		{			
			LONG AllocatedVcns = 0;
			LONG FreeVcns = 0;

			// Find out how many runs and VCNs make up this CU.
			for ( j=0 ; (i+j) < (Extents->_NumberOfRuns) ; j++ )
			{
				if (Extents->_pMapArray[i+j].Allocated)	AllocatedVcns += Extents->_pMapArray[i+j].Allocated;
				else									FreeVcns      += (LONG) (Extents->_pMapArray[i+j].NextVcn - Extents->_pMapArray[i+j].CurrentVcn);
				
				// Continue until a compression chunk boundary is found.
				if ( !((AllocatedVcns+FreeVcns) % VcnsPerCompressionChunk) )
				{
					RunStep = j + 1;
					MaxCompressionUnitVcn =  Extents->_pMapArray[i].CurrentVcn + AllocatedVcns + FreeVcns - 1;
					break;
				}
			}

		}

		if (Sparse)
		{
			// Sparse is easy since everything lines up with the CU boundaries and nothing is compressed.
			MaxCompressionUnitVcn = Extents->_pMapArray[i].NextVcn - 1;
		}

		// If this read is in range, then do it.
		if (ThisVcn >= Extents->_pMapArray[i].CurrentVcn && (ThisVcn < Extents->_pMapArray[i+RunStep-1].NextVcn ||ThisVcn <= MaxCompressionUnitVcn))
		{
			RW_BUFFER Temp;

			// Check to see if this read is not aligned with this VCN range.
			RunOffset = ThisVcn - Extents->_pMapArray[i].CurrentVcn;
			
			if (Compressed || Sparse)
			{
				ThisChunkSize = (LONG) min(ClustersRemaining, (MaxCompressionUnitVcn - ThisVcn + 1) );
			}
			else
			{
				ThisChunkSize = (LONG) min(ClustersRemaining, (Extents->_pMapArray[i].Allocated - RunOffset) );
			}
			
			if ( Compressed )
			{
				// Read and decompress this run.
				Error = ReadCompressedAllocation(&Temp, i, RunStep, pAttribute, Extents);
				if (Error)
				{
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					pBuffer->BUFF_DeleteManagedBuffer();
					return Error;
				}

				if (RunOffset)
				{
					// Align the temp buffer so that the buffer begins with ThisVcn.
					Error = Temp.BUFF_Realign(RunOffset * _VolumeData.BytesPerCluster);
					if (Error)
					{
						VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
						pBuffer->BUFF_DeleteManagedBuffer();
						return Error;
					}
				}

			} else if (Sparse && !Extents->_pMapArray[i].Allocated)
			{
				// If the file is sparse and the range is not allocated.
				Error = Temp.BUFF_CreateManagedBuffer( ThisChunkSize * _VolumeData.BytesPerCluster);
				if (Error)
				{
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					pBuffer->BUFF_DeleteManagedBuffer();
					return Error;
				}

				Temp.BUFF_Zero();
			}		// All other cases use no compression.
			else 
			{
				// All other cases use no compression.
				Error = ReadNtfsClusters(&Temp, Extents->_pMapArray[i].CurrentLcn+RunOffset, ThisChunkSize);
				if (Error)
				{
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					pBuffer->BUFF_DeleteManagedBuffer();
					return Error;
				}
			}
	
			if (ThisChunkSize)
			{
				VCN VcnsRead = Temp._NumberOfBytes / _VolumeData.BytesPerCluster;
				if (Temp._NumberOfBytes % _VolumeData.BytesPerCluster)
					VcnsRead++;
				
				// Update progress and do the next run.
				ClustersRemaining -= ThisChunkSize;
				ThisVcn += ThisChunkSize;
				Error = pBuffer->BUFF_Append(&Temp);
				if (Error)
				{
					VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
					pBuffer->BUFF_DeleteManagedBuffer();
					return Error;
				}
			}
		}
	}

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}


DWORD 
NTFS_VOLUME::ReadAttributeList(OUT PRW_BUFFER pAttributeListBuffer, 
										  IN  PRW_BUFFER pFrsBuffer)
{	
	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"ENTER\n");
	DWORD Error;
	
	pAttributeListBuffer->BUFF_DeleteManagedBuffer();

	PATTRIBUTE_RECORD_HEADER pAttribute = GetAttributeRecordPointerByType(pFrsBuffer, $ATTRIBUTE_LIST, NULL, 0);
	if (pAttribute==NULL) 
	{
		Error = GetLastError();
		VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
		return Error;
	}

	switch (pAttribute->FormCode)
	{
	case RESIDENT_FORM:
		Error = pAttributeListBuffer->BUFF_CopyStdBuffer(ADD_PTR(pAttribute, pAttribute->Form.Resident.ValueOffset), pAttribute->Form.Resident.ValueLength);
		break;
	case NONRESIDENT_FORM:
		{
			MAPPING_PAIRS m(pAttribute, VolumeDebugFlags);
			if (!m.ArrayIsValid())
			{
				Error = ERROR_INVALID_PARAMETER;						
				VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
				return Error;
			}
			
			LONGLONG Start = pAttribute->Form.Nonresident.LowestVcn;
			LONG Length = (LONG) VCNS_IN_ATTRIBUTE(pAttribute);
			Error=ReadAttributeVcnRange(pAttributeListBuffer, &m, Start, Length, 0);
		}
		break;
	default:
		Error = ERROR_INVALID_PARAMETER;
		break;
	}

	VolDbgPrint(DF_INFO,__FUNCTION__,__LINE__,"EXIT - Error %lu\n", Error);
	return Error;
}
		
DWORD 
NTFS_VOLUME::LoadCompressionApi()
{
	_hLibrary = 0;
	DWORD Error = 0;

	if (_hLibrary = LoadLibrary(L"ntdll.dll"))
	{
		RtlDecompressBuffer            = (DECOMPRESS_FN ) GetProcAddress(_hLibrary,"RtlDecompressBuffer");
		RtlCompressBuffer	           = (COMPRESS_FN   ) GetProcAddress(_hLibrary,"RtlCompressBuffer");
		RtlGetCompressionWorkSpaceSize = (GETCW_FN      ) GetProcAddress(_hLibrary,"RtlGetCompressionWorkSpaceSize");
	}

	if (RtlCompressBuffer && RtlDecompressBuffer && RtlGetCompressionWorkSpaceSize)
	{
		return 0;
	}
	else 
	{
		FreeLibrary(_hLibrary);
		return ERROR_FILE_NOT_FOUND;
	}
}


DWORD 
NTFS_VOLUME::ReadCompressedAllocation(
						PRW_BUFFER pBuffer,
						LONG RunIndex,
						LONG RunsInCompressionUnit,
						PATTRIBUTE_RECORD_HEADER pAttribute,
						MAPPING_PAIRS * pExtents)
{
	DWORD Error;
	RW_BUFFER CompressedBuffer, DecompressedBuffer;
	USHORT CompressionFormat		=  (pAttribute->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) + 1;
	LONG VcnsPerCompressionUnit = 1 << pAttribute->Form.Nonresident.CompressionUnit;
	LONG	 j;
	ULONG	 DecompressedSize;
	LONGLONG BytesPerCu			= VcnsPerCompressionUnit * _VolumeData.BytesPerCluster;
	LONGLONG EmptyVcns          = 0;
	LONGLONG CompressedVcns		= 0;
	LONGLONG CompressionUnits;
	LONG TotalVcns =0;
	VCN FirstCompressedVcn;
	
	CompressedBuffer.BUFF_DeleteManagedBuffer();
	
	// Read the number of runs needed to hit a CU boundary.
	for (j=RunIndex ; j<(RunIndex+RunsInCompressionUnit); j++)
	{
		RW_BUFFER ScratchBuff;
		Error = ReadNtfsClusters(&ScratchBuff, pExtents->_pMapArray[j].CurrentLcn, pExtents->_pMapArray[j].Allocated);
		if (Error)
		{
			goto ERROR_EXIT;
		}
		//
		// BUGBUG: Crash decompressing empty run.  Not sure if this is right.
		//
		if (ScratchBuff._NumberOfBytes == 0)
		{
			ScratchBuff.BUFF_CreateManagedBuffer((pExtents->_pMapArray[j].NextVcn - pExtents->_pMapArray[j].CurrentVcn)* this->_VolumeData.BytesPerCluster);
			ScratchBuff.BUFF_Zero();
		}


		Error = CompressedBuffer.BUFF_Append(&ScratchBuff);
		if (Error)
		{
			goto ERROR_EXIT;
		}
		
		//Count how many compressed VCNs are in this run.
		if (pExtents->_pMapArray[j].Allocated)	CompressedVcns += pExtents->_pMapArray[j].Allocated;
		else EmptyVcns += (pExtents->_pMapArray[j].NextVcn - pExtents->_pMapArray[j].CurrentVcn);

		if ( !((CompressedVcns+EmptyVcns)% VcnsPerCompressionUnit) ) break;
		
	}
	

	// Find out how many CU's are in this run.
	CompressionUnits	= (CompressedVcns+EmptyVcns) / VcnsPerCompressionUnit;

	// Compute the first compressed VCN since there could be many non-compressed CU's followed by a single compressed CU.
	FirstCompressedVcn = pExtents->_pMapArray[RunIndex].CurrentVcn + THIS_ALIGNED_VALUE(CompressedVcns, VcnsPerCompressionUnit);
	
	// Decompress if needed.
	if (EmptyVcns)
	{
		// Get the compression offset and estimate the compressed size.
		LONGLONG  FirstCompressedOffset = (FirstCompressedVcn - pExtents->_pMapArray[RunIndex].CurrentVcn) * _VolumeData.BytesPerCluster;
		LONGLONG  CompressedSize		= CompressedBuffer._NumberOfBytes - FirstCompressedOffset;
		
		// Create a buffer that is the size of the uncompressed portion.
		Error = pBuffer->BUFF_CreateManagedBuffer(FirstCompressedOffset);
		if (Error)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			goto ERROR_EXIT;
		}

		// Copy the uncompressed portion into the destination buffer.
		Error = pBuffer->BUFF_CopyBufferExt(&CompressedBuffer, 0, 0, FirstCompressedOffset);
		if (Error)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			goto ERROR_EXIT;
		}

		// Realign the buffer so it begins with the first compressed byte.
		Error =	CompressedBuffer.BUFF_Realign(FirstCompressedOffset) | DecompressedBuffer.BUFF_CreateManagedBuffer(BytesPerCu);
		if (Error)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			goto ERROR_EXIT;
		}
				
		// Determine the real compressed size.
		CompressedBuffer.BUFF_FindCompressedLength();
		
		// Decompress the unit buffer.
		Error = RtlDecompressBuffer(
					CompressionFormat,
					(PUCHAR) DecompressedBuffer._pBuffer,
					(ULONG)  DecompressedBuffer._NumberOfBytes,
					(PUCHAR) CompressedBuffer._pBuffer,
					(ULONG)  CompressedBuffer._NumberOfBytes,
					(PULONG) &DecompressedSize);
		if (Error)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			goto ERROR_EXIT;
		}

		Error = pBuffer->BUFF_Append(&DecompressedBuffer);
		if (Error)
		{
			pBuffer->BUFF_DeleteManagedBuffer();
			goto ERROR_EXIT;
		}
	}
	else
	{
		Error = pBuffer->BUFF_CloneBuffer(&CompressedBuffer);
	}

ERROR_EXIT:
	if (Error)
	{
		pBuffer->BUFF_DeleteManagedBuffer();
	}

	return Error;
}

DWORD 
NTFS_VOLUME::FormatPathString(PMFT_SEGMENT_REFERENCE pFrs, WCHAR **szPath)
{
	DWORD Error;
	RW_BUFFER Buffer1, Buffer2;
	RW_BUFFER ThisFrsBuffer;
	WCHAR * szFullPath = NULL;
	WCHAR ** szScratchPath;
	size_t PathLen;
	ULONG Depth = 0;
	MFT_SEGMENT_REFERENCE ThisFrs;
	LONG i;	

	NtfsSetSegmentNumber(&ThisFrs, pFrs->SegmentNumberHighPart, pFrs->SegmentNumberLowPart);
	
	szScratchPath = new WCHAR * [50];
	if (!szScratchPath)
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto EXIT;
	}
		
	Error = ReadFrsRaw(&ThisFrsBuffer, &ThisFrs);
	if (Error) goto EXIT;

	Error = (IsFileRecord(&ThisFrsBuffer)) ? NO_ERROR : ERROR_FILE_NOT_FOUND;
	if (Error) goto EXIT;

	PATTRIBUTE_RECORD_HEADER pLongName;
	PATTRIBUTE_RECORD_HEADER pShortName;
	PATTRIBUTE_RECORD_HEADER pNoflagName;

	while ( NtfsFullSegmentNumber(&ThisFrs)!=5 && 
			((pLongName  = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,FILE_NAME_NTFS))  ||
			 (pShortName = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,FILE_NAME_DOS ))  ||
			 (pNoflagName = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,0 ))
			 )
		  )
	{
		PFILE_NAME pf;
		pLongName  = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,FILE_NAME_NTFS);
		pShortName = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,FILE_NAME_DOS );
		pNoflagName = GetFileNameAttributerRecordPointer(&ThisFrsBuffer,0 );

		if (pLongName)
		{
			pf = (PFILE_NAME) ADD_PTR(pLongName,pLongName->Form.Resident.ValueOffset);
		}
		else if (pShortName)
		{
			pf = (PFILE_NAME) ADD_PTR(pShortName,pShortName->Form.Resident.ValueOffset);
		}
		else
		{
			pf = (PFILE_NAME) ADD_PTR(pNoflagName,pNoflagName->Form.Resident.ValueOffset);
		}
		
		szScratchPath[Depth] = CreateNameString(pf->FileName, pf->FileNameLength);
		
		NtfsSetSegmentNumber(&ThisFrs, pf->ParentDirectory.SegmentNumberHighPart, pf->ParentDirectory.SegmentNumberLowPart);

		Error = ReadFrsRaw(&ThisFrsBuffer, &ThisFrs);
		if (Error)	
			goto EXIT;

		if (!IsFileRecord(&ThisFrsBuffer)) 
		{
			Error = ERROR_FILE_NOT_FOUND;
			goto EXIT;
		}

		Depth++;
	}

	PathLen=2;
	if (Depth)
	{
		for (i=Depth-1; i>=0; i--)
		{
			PathLen+=wcslen(szScratchPath[i]) + 2 * sizeof(WCHAR);        
		}
	}

	szFullPath = new WCHAR [PathLen];
	if (!szFullPath)
	{
		Error = ERROR_NOT_ENOUGH_MEMORY;
		goto EXIT;
	}

	wcscpy(szFullPath, L"\\");

	if (Depth)
	{
		for (i=Depth-1; i>=0; i--)
		{
			wcscat(szFullPath,szScratchPath[i]);
			if (i)	wcscat(szFullPath,L"\\");
		}
	}

EXIT:

	if (Error && szFullPath)
	{
		if (szScratchPath)	delete[] szScratchPath;
		if (szFullPath)		delete[] szFullPath;

		*szPath = NULL;
	}
	else
	{
		*szPath = szFullPath;
	}

	for (i=Depth-1;i>0;i--)
	{
		if (szScratchPath[i]) 
		{
			delete [] szScratchPath[i];
		}
	}
	
	delete [] szScratchPath;
	return Error;
}

BOOL 
NTFS_VOLUME::IsFileRecord(RW_BUFFER * pBuffer)
{
	PFILE_RECORD_SEGMENT_HEADER pHeader = (PFILE_RECORD_SEGMENT_HEADER) pBuffer->_pBuffer;

	if ( (pHeader->MultiSectorHeader.Signature != 'ELIF' ||
		  pHeader->MultiSectorHeader.Signature != 0x0000) &&
		IsValidMultiSectorHeader(&pHeader->MultiSectorHeader)==FALSE) 
	{
		return FALSE;
	}

	PATTRIBUTE_RECORD_HEADER pAttribute = GetFirstAttributeRecordPointer(pBuffer);

	if ( pAttribute && 
		pAttribute->RecordLength < pHeader->BytesAvailable &&
		(pAttribute->TypeCode & 0xf) == 0 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
