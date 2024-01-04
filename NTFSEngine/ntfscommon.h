#pragma once

#define BUILD_NUMBER		__COUNTER__ 

#define LCN LONGLONG
#define LBN LONGLONG

// IO Flags
#define FLIO_READ_PAST_VDL 0x80000000

#define VACB_MAPPING_GRANULARITY 0x00040000
#define VACB_MAPPING_OFFSET_MASK 0x0000ffff

#define IS_FLAG_SET(V,F)  ((V) & (F)) ? TRUE : FALSE
#define ADD_PTR(p, q) ((PCHAR) (p) + (q))

#define MAX_FRS_SIZE	0x400
#define LOG_PAGE 0x1000

// If defined, we use our own method of obtaining the FRS & volume info.
#define FRS_HEADER_OFFSET 0xc
#define CopyUchar1(d,s) RtlCopyMemory((d),(s),1)
#define CopyUchar2(d,s) RtlCopyMemory((d),(s),2)
#define CopyUchar4(d,s) RtlCopyMemory((d),(s),4)
#define CopyUchar8(d,s) RtlCopyMemory((d),(s),8)

#define THIS_ALIGNED_VALUE(Addr, Bytes) (((Addr)) & ~((Bytes)-1))
#define NEXT_ALIGNED_VALUE(Addr, Bytes) (((Addr) + (Bytes)) & ~((Bytes)-1))

#define	ALIGN(Addr, Bytes) ((Addr) > THIS_ALIGNED_VALUE((Addr), (Bytes))) ?  NEXT_ALIGNED_VALUE((Addr),(Bytes)) : (Addr)

#define DQ_ALIGN(Addr)    ALIGN((Addr),0x10L)
#define  Q_ALIGN(Addr)    ALIGN((Addr),0x08L)
#define DW_ALIGN(Addr)    ALIGN((Addr),0x04L)
#define  W_ALIGN(Addr)    ALIGN((Addr),0x02L)

#define DQuadAlign(Addr)	DQ_ALIGN((Addr))
#define  QuadAlign(Addr)     Q_ALIGN((Addr))
#define DWordAlign(Addr)    DW_ALIGN((Addr))
#define  WordAlign(Addr)     W_ALIGN((Addr))

#define VCN_IN_RANGE(vcn, maparrayentry) ((vcn) >= (maparrayentry).CurrentVcn && (vcn) < (maparrayentry).NextVcn)
#define VCNS_IN_ATTRIBUTE(pAttr)	((pAttr)->Form.Nonresident.HighestVcn - (pAttr)->Form.Nonresident.LowestVcn + 1)
#define BYTES_REMAINING_IN_NR_ALLOCATION(pAttr, bo)	 (LONG) ((bo) < ((pAttr)->Form.Nonresident.FileSize       )) ? (((pAttr)->Form.Nonresident.FileSize       ) - (bo)) : 0
#define BYTES_REMAINING_IN_NR_ATTRIBUTE(pAttr, bo)	 (LONG) ((bo) < ((pAttr)->Form.Nonresident.ValidDataLength)) ? (((pAttr)->Form.Nonresident.ValidDataLength) - (bo)) : 0

#define BYTES_REMAINING_IN_RES_ATTRIBUTE(pAttr, bo)	 (((pAttribute)->Form.Resident.ValueLength) - (bo))
#define UNITS_REMAINING_FROM_N(n, hi)  ( (hi)-(n) + 1)

#define VCN_FROM_BYTES(offset,bpc)		((offset)/(bpc))
#define BYTES_FROM_VCN(offset,bpc)		((offset)*(bpc))
#define OFFSET_WITHIN_VCN(offset,bpc)	((offset)%(bpc))
#define VCN_START(o, bpc)				( THIS_ALIGNED_VALUE((o), (bpc)) / (bpc) ) 
#define VCN_END(o, bpc)					( NEXT_ALIGNED_VALUE((o), (bpc)) / (bpc) )

#define SAFE_SZ(pSz) (pSz) ? (pSz): L"\"\""
#define SAFE_PV(pVal) (pVal) ? (pVal) : 0

#define FATAL_READ_ERROR(E) ((E) && ((E)!= ERROR_MORE_DATA))

#define MAX_READ (VACB_MAPPING_GRANULARITY * 10)
#define MAX_SID  0x01000000L


//
//  Object ID attribute.
//

typedef struct _OBJID_INDEX_ENTRY_VALUE {
    GUID					key;
    MFT_SEGMENT_REFERENCE   SegmentReference;
    char                    extraInfo[48];
} * POBJID_INDEX_ENTRY_VALUE, OBJID_INDEX_ENTRY_VALUE;

typedef struct _REPARSE_DATA_BUFFER {
    LONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
		struct {
		    ULONG                           ReparsePointFormatVersion;
		    ULONG                           Reserved;
			GUID                            CSid;
			LARGE_INTEGER                   LinkIndex;
			LARGE_INTEGER                   LinkFileNtfsId;
			LARGE_INTEGER                   CSFileNtfsId;
			LARGE_INTEGER                   CSChecksum;
			LARGE_INTEGER                   Checksum;
		} SIS_REPARSE_BUFFER;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

typedef struct _COMPRESSED_DATA_INFO {
    USHORT  CompressionFormatAndEngine;
    UCHAR   CompressionUnitShift;
    UCHAR   ChunkShift;
    UCHAR   ClusterShift;
    UCHAR   Reserved;
    USHORT  NumberOfChunks;
    LONG   CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

typedef struct _MAPPING_PAIRS_ARRAY 
{
	VCN		CurrentVcn;
	VCN		NextVcn;
	LCN		CurrentLcn;
	LONG	Allocated;
} * PMAPPING_PAIRS_ARRAY, MAPPING_PAIRS_ARRAY;


typedef DWORD (CALLBACK * DESCRIBE_CHUNK) (
    IN USHORT       CompressionFormat,
    IN OUT PUCHAR   *CompressedBuffer,
    IN PUCHAR       EndOfCompressedBufferPlus1,
    OUT PUCHAR      *ChunkBuffer,
    OUT PULONG      ChunkSize
);

typedef DWORD (CALLBACK* DECOMPRESS_FN)(
						IN USHORT   CompressionFormat,
						OUT PUCHAR  UncompressedBuffer,
						IN ULONG    UncompressedBufferSize,
						IN PUCHAR   CompressedBuffer,
						IN ULONG    CompressedBufferSize,
						OUT PULONG  FinalUncompressedSize
						);

typedef DWORD (CALLBACK* COMPRESS_FN)(
						IN USHORT   CompressionFormatAndEngine,
						IN PUCHAR   UncompressedBuffer,
						IN ULONG    UncompressedBufferSize,
						OUT PUCHAR  CompressedBuffer,
						IN ULONG    CompressedBufferSize,
						IN ULONG    UncompressedChunkSize,
						OUT PULONG  FinalCompressedSize,
						IN PVOID    WorkSpace
						);

typedef DWORD (CALLBACK* GETCW_FN)(
						ULONG					CompressionFormat, 
						PULONG					pBufferSize, 
						PULONG					pChunkSize);  


typedef enum _LFS_RECORD_TYPE {

    LfsClientRecord = 1,
    LfsClientRestart

} LFS_RECORD_TYPE, *PLFS_RECORD_TYPE;

#define IsLogOperation(prec, opcode) ((pV)->RedoOperation==(opcode) || (pV)->UndoOperation==(opcode))
typedef ULONG TRANSACTION_ID, *PTRANSACTION_ID;
#define three (INT) 0x3
#define GetSeqNumberBits(D) (D)
#define GetLogPageSize(size) (size)
#define LOG_FILE_DATA_BITS(D)      ((sizeof(LSN) * 8) - GetSeqNumberBits(D))
#define LOG_PAGE_MASK(D)        (GetLogPageSize(D) - 1)
#define LOG_PAGE_INVERSE_MASK(D)   (~LOG_PAGE_MASK(D))
#define LfsLogPageOffset(D, i)   ((i) & LOG_PAGE_MASK(D))
#define LfsLsnToPageOffset(D, lsn) (LfsLogPageOffset(D, (lsn).LowPart << three))
#define PAGE_ALIGN(Off, PageSize)   ((Off) & ~((PageSize)-1))

#define LfsLsnToFileOffset(D, LSN)                 \
    /*xxShr*/(((ULONGLONG)/*xxShl*/((LSN).QuadPart << GetSeqNumberBits(D))) >> (GetSeqNumberBits(D) - three))
 
#define LfsTruncateLsnToLogPage(D, lsn, FO) {           \
    *(FO) = LfsLsnToFileOffset((D), (lsn));             \
    *((PULONG)(FO)) &= LOG_PAGE_INVERSE_MASK(D);        \
    }

#define LfsLsnToSeqNumber(D, LSN)                     \
    /*xxShr*/((ULONGLONG)/*xxShl*/((LSN).QuadPart) >> LOG_FILE_DATA_BITS(D))

#define LfsFileOffsetToLsn(D, FO, SN)          \
    ((((ULONGLONG)(FO)) >> three) + ((SN) << LOG_FILE_DATA_BITS(D)))
    

