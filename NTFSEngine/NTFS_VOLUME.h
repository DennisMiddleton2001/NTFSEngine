#pragma once

class NTFS_VOLUME 
{
public:
	//
	// Members
	//
	LONG					VolumeDebugFlags;
	WCHAR					_szDeviceName[NTFS_MAX_ATTR_NAME_LEN];
	HANDLE					_hDevice;
	NTFS_VOLUME_DATA_BUFFER _VolumeData;
	DISK_GEOMETRY			_Geometry;
	MFT_SEGMENT_REFERENCE	_LastReadFrs;
	BOOL					_Verbose;
	LONGLONG				_RawRelativeSector;
	RW_BUFFER				_MftRecord;
	HMODULE					_hLibrary;
	LONG					_MaxRead;
	
	// Class Methods
	NTFS_VOLUME  ();
	NTFS_VOLUME  (LONG DebugFlags);
   ~NTFS_VOLUME  ();
	DECOMPRESS_FN			RtlDecompressBuffer;
	COMPRESS_FN				RtlCompressBuffer;
	GETCW_FN				RtlGetCompressionWorkSpaceSize;

	DWORD	LoadCompressionApi();

	VOID VolDbgPrint(
	    LONG			MessageImportance,
		CHAR			* FunctionName,
		LONG			Line,
		CHAR			* szFormat,
		...
		);

	DWORD	Initialize(		IN WCHAR * szDeviceName, 
							IN LONG RelativeSector, 
							IN LONGLONG NumberSectors, 
							IN LCN MftStartLcn, 
							IN LCN Mft2StartLcn, 
							IN LONG SectorsPerCluster);

	DWORD	Initialize(		IN WCHAR * szDeviceName, 
							IN LONG RelativeSector, 
							IN LONGLONG NumberSectors, 
							IN LCN MftStartLcn, 
							IN LCN Mft2StartLcn, 
							IN LONG SectorsPerCluster,
							IN LONG Flags);
	
	DWORD	CloseVolumeHandle();




	DWORD ReadNtfsBlocksUsaAligned (
							OUT PRW_BUFFER pBuffer,
							IN  LCN Offset,
							OUT  DWORD * pBytes);
	
	DWORD ReadNtfsBlocks (
							OUT PRW_BUFFER pBuffer, 
							IN  LCN BlockNumber, 
							IN  DWORD NumberOfBlocks, 
							IN  DWORD BytesPerBlock);	
	
	DWORD WriteNtfsBlocks(
							IN  PRW_BUFFER pBuffer, 
							IN  LCN BlockNumber, 
							IN  LONGLONG BytesPerBlock);
	
	DWORD ReadNtfsSectors(
							OUT  PRW_BUFFER pBuffer, 
							IN  LCN Sector, 
							IN  LONG NumberOfSectors);
	
	DWORD WriteNtfsSectors(
							IN  PRW_BUFFER pBuffer, 
							IN  LBN Lbn);
	
	DWORD ReadNtfsClusters(
							OUT PRW_BUFFER pBuffer, 
							IN  LCN Lcn, 
							IN  LONG NumberOfLcns);
	
	DWORD WriteNtfsClusters(
							IN  PRW_BUFFER pBuffer,
							IN  LCN Lcn);

	DWORD WriteNtfsBytes(
							IN  PRW_BUFFER pBuffer, 
							IN  LONGLONG ByteOffsetStart);

	
	DWORD ReadCompressedAllocation(
						PRW_BUFFER pBuffer,
						LONG RunIndex,
						LONG RunsInCompressionUnit,
						PATTRIBUTE_RECORD_HEADER pAttribute,
						MAPPING_PAIRS * pExtents);



	VOID  CleanupSequenceArray(
							IN  PRW_BUFFER pBuffer);
	
	VOID  UpdateSequenceArray(
							IN  PRW_BUFFER pBuffer);

	BOOL  IsValidMultiSectorHeader(
							IN  PMULTI_SECTOR_HEADER pHeader
							);
	
	DWORD ReadFrsRaw(	OUT PRW_BUFFER pBuffer, 
							IN  PMFT_SEGMENT_REFERENCE pFileId);

	DWORD WriteFrsRaw(	IN  PRW_BUFFER pBuffer, 
							IN  PMFT_SEGMENT_REFERENCE pFileId);

	PATTRIBUTE_RECORD_HEADER GetAttributeRecordPointerByType(
							IN  PRW_BUFFER pBuffer, 
							IN  ATTRIBUTE_TYPE_CODE Type, 
							IN  WCHAR * szName, 
							IN  VCN LowestVcn);
	
	PATTRIBUTE_RECORD_HEADER GetFileNameAttributerRecordPointer(
							IN   PRW_BUFFER pBuffer, 
							IN   UCHAR Flags);		
	
	PATTRIBUTE_RECORD_HEADER GetFirstAttributeRecordPointer(
							IN   PRW_BUFFER pBuffer);
	
	PATTRIBUTE_RECORD_HEADER GetNextAttributeRecordPointer(
							IN   PATTRIBUTE_RECORD_HEADER pThisAttribute);	
	
	BOOL IsMatchingAttribute(
							OUT  PATTRIBUTE_RECORD_HEADER pAttribute, 
							IN   ATTRIBUTE_TYPE_CODE Type, 
							IN   WCHAR * szName, 
							IN   VCN LowestVcn);	
	
	DWORD ReadAttributeList(
							OUT PRW_BUFFER pAttributeListBuffer, 
							IN  PRW_BUFFER pFrsBuffer);

	DWORD ReadChildFrsFromAttributeList(
							IN  PRW_BUFFER pAttributeListBuffer, 
							IN  ATTRIBUTE_TYPE_CODE Type,
							IN  WCHAR * szName, 
							IN  VCN LowestVcn, 
							OUT PRW_BUFFER pChildFrsBuffer, 
							OUT PMFT_SEGMENT_REFERENCE pChildFileId, 
							OUT PATTRIBUTE_RECORD_HEADER * ppAttributeMatch);
	
	DWORD GetChildFrsIfMatchingAttrListEntry(
							IN  PATTRIBUTE_LIST_ENTRY pEntry, 
							OUT PRW_BUFFER pChildFrsBuffer, 
							OUT PMFT_SEGMENT_REFERENCE pChildFileId, 
							IN  ATTRIBUTE_TYPE_CODE Type, 
							IN  WCHAR * szName, 
							IN  VCN LowestVcn, 
							OUT PATTRIBUTE_RECORD_HEADER * ppAttributeMatch);

	DWORD ReadFileNameAttribute(
							OUT PRW_BUFFER pBuffer, 
							IN  PRW_BUFFER pFrsBuffer, 
							IN  UCHAR Flags);

	DWORD ReadAttributeByteRange(OUT PRW_BUFFER pBuffer,
		IN  PMFT_SEGMENT_REFERENCE pBaseFileId,
		IN  ATTRIBUTE_TYPE_CODE Type,
		IN  PWCHAR szName,
		IN  LONGLONG StartOffset,
		IN  LONG NumberOfBytes,
		IN  LONG Flags);

	
	DWORD ReadAttributeByteRangeUsaAligned(OUT PRW_BUFFER pBuffer, 
											   IN  PRW_BUFFER pBaseFrsBuffer, 
											   IN  ATTRIBUTE_TYPE_CODE Type, 
											   IN  PWCHAR szName, 
											   IN  LONGLONG StartOffset, 
											   OUT  LONG * pNumberOfBytes,
											   IN  LONG Flags);

	DWORD ReadAttributeByteRange(
							OUT PRW_BUFFER pBuffer, 
							IN  PRW_BUFFER pBaseFrsBuffer, 
							IN  ATTRIBUTE_TYPE_CODE Type, 
							IN  PWCHAR szName, 
							IN  LONGLONG StartOffset, 
							IN  LONG NumberOfBytes,
							IN  LONG Flags);

	DWORD ReadResidentAttributeFromAttributeInstance(
							OUT PRW_BUFFER pBuffer, 
							IN  PATTRIBUTE_RECORD_HEADER pAttribute, 
							IN  LONGLONG StartOffset, 
							IN  LONG Length);
	
	DWORD ReadAttributeVcnRange (
							OUT	PRW_BUFFER pBuffer, 
							IN   MAPPING_PAIRS * Extents,
							IN	LONGLONG StartVcn, 
							IN	LONG NumberOfClusters,
							IN	LONG Flags);

	DWORD OpenVolumeHandle(
							IN  LPCWSTR szDeviceName, 
							IN  LONGLONG RelativeSector);

	DWORD FormatPathString(
							IN  PMFT_SEGMENT_REFERENCE pFrs, 
							OUT WCHAR **szPath);

	BOOL IsFileRecord(RW_BUFFER * pBuffer);

	DWORD SetRwPointer (IN  LCN BlockNumber, 
							IN  LONGLONG BytesPerBlock);


	VOID  ClearVolumeMemberVariables();

	DWORD GetNtfsVolumeInformation(
							IN  LONG RelativeSector);
	
	DWORD GetDiskGeometry();
	
	DWORD InitializeRaw(IN  WCHAR * szDeviceName, 
							IN  LONGLONG RelativeSector);
	
	DWORD SetVolumeDataParameters(
							IN  PBIOS_PARAMETER_BLOCK pBpb, 
							IN  PACKED_BOOT_SECTOR *  pNtfsBpb,
							IN  LONG Flags);
	
	DWORD ReadSystemFrsRaw(
							OUT PRW_BUFFER pBuffer, 
							IN  PMFT_SEGMENT_REFERENCE pFileId);
};



// Volume init flags.
#define VOL_RAWMFT				 0x00000001L
