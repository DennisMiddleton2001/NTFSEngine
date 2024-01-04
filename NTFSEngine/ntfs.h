#ifndef _NTFS_
#define _NTFS_

#pragma pack(4)

typedef LONGLONG LCN;
typedef LCN *PLCN;

typedef LARGE_INTEGER LSN, *PLSN;
//typedef LONGLONG LSN;
typedef LSN *PLSN;

typedef LONGLONG VCN;
typedef VCN *PVCN;

typedef LONGLONG LBO;
typedef LBO *PLBO;

typedef LONGLONG VBO;
typedef VBO *PVBO;

typedef ULONG COLLATION_RULE;
typedef ULONG DISPLAY_RULE;
typedef CHAR  UPDATE_SEQUENCE_ARRAY;

typedef struct _MULTI_SECTOR_HEADER
{
	ULONG Signature;
	USHORT SequenceArrayOffset;
	USHORT SequenceArraySize;
} MULTI_SECTOR_HEADER, * PMULTI_SECTOR_HEADER;

#define NTFS_CHUNK_SIZE                  (0x1000)
#define NTFS_CHUNK_SHIFT                 (12)
#define NTFS_CLUSTERS_PER_COMPRESSION    (4)
#define NTFS_SPARSE_FILE_UNIT            (0x10000)
#define COLLATION_BINARY                 (0)
#define COLLATION_FILE_NAME              (1)
#define COLLATION_UNICODE_STRING         (2)
#define COLLATION_NUMBER_RULES           (3)
#define COLLATION_NTOFS_FIRST            (16)
#define COLLATION_NTOFS_ULONG            (16)
#define COLLATION_NTOFS_SID              (17)
#define COLLATION_NTOFS_SECURITY_HASH    (18)
#define COLLATION_NTOFS_ULONGS           (19)
#define COLLATION_NTOFS_LAST             (19)

//
//  The following macros are used to set and query with respect to
//  the update sequence arrays.
//

#define UpdateSequenceStructureSize(MSH) (                         \
    ((((PMULTI_SECTOR_HEADER)(MSH))->UpdateSequenceArraySize-1) *  \
                                           SEQUENCE_NUMBER_STRIDE) \
)

#define UpdateSequenceArraySize(STRUCT_SIZE) (   \
    ((STRUCT_SIZE) / SEQUENCE_NUMBER_STRIDE + 1) \
)


//
//  The MFT Segment Reference is an address in the MFT tagged with
//  a circularly reused sequence number set at the time that the MFT
//  Segment Reference was valid.  Note that this format limits the
//  size of the Master File Table to 2**48 segments.  So, for
//  example, with a 1KB segment size the maximum size of the master
//  file would be 2**58 bytes, or 2**28 gigabytes.
//

typedef struct _MFT_SEGMENT_REFERENCE {

    //
    //  First a 48 bit segment number.
    //

    ULONG SegmentNumberLowPart;                                    //  offset = 0x000
    USHORT SegmentNumberHighPart;                                  //  offset = 0x004

    //
    //  Now a 16 bit nonzero sequence number.  A value of 0 is
    //  reserved to allow the possibility of a routine accepting
    //  0 as a sign that the sequence number check should be
    //  repressed.
    //

    USHORT SequenceNumber;                                          //  offset = 0x006

} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;                   //  sizeof = 0x008

//
//  A file reference in NTFS is simply the MFT Segment Reference of
//  the Base file record.
//

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;

//
//  While the format allows 48 bits worth of segment number, the current
//  implementation restricts this to 32 bits.  Using NtfsUnsafeSegmentNumber
//  results in a performance win.  When the implementation changes, the
//  unsafe segment numbers must be cleaned up.  NtfsFullSegmentNumber is
//  used in a few spots to guarantee integrity of the disk.
//

#define NtfsSegmentNumber(fr)       NtfsUnsafeSegmentNumber( fr )
#define NtfsFullSegmentNumber(fr)   ( (*(ULONGLONG UNALIGNED *)(fr)) & 0xFFFFFFFFFFFF )
#define NtfsUnsafeSegmentNumber(fr) ((fr)->SegmentNumberLowPart)

#define NtfsSetSegmentNumber(fr,high,low)   \
    ((fr)->SegmentNumberHighPart = (high), (fr)->SegmentNumberLowPart = (low))

#define NtfsEqualMftRef(X,Y)    ( NtfsSegmentNumber( X ) == NtfsSegmentNumber( Y ) )

#define NtfsLtrMftRef(X,Y)      ( NtfsSegmentNumber( X ) <  NtfsSegmentNumber( Y ) )

#define NtfsGtrMftRef(X,Y)      ( NtfsSegmentNumber( X ) >  NtfsSegmentNumber( Y ) )                                               \

#define NtfsLeqMftRef(X,Y)      ( NtfsSegmentNumber( X ) <= NtfsSegmentNumber( Y ) )

#define NtfsGeqMftRef(X,Y)      ( NtfsSegmentNumber( X ) >= NtfsSegmentNumber( Y ) )


#define MASTER_FILE_TABLE_NUMBER         (0)   //  $Mft
#define MASTER_FILE_TABLE2_NUMBER        (1)   //  $MftMirr
#define LOG_FILE_NUMBER                  (2)   //  $LogFile
#define VOLUME_DASD_NUMBER               (3)   //  $Volume
#define ATTRIBUTE_DEF_TABLE_NUMBER       (4)   //  $AttrDef
#define ROOT_FILE_NAME_INDEX_NUMBER      (5)   //  .
#define BIT_MAP_FILE_NUMBER              (6)   //  $BitMap
#define BOOT_FILE_NUMBER                 (7)   //  $Boot
#define BAD_CLUSTER_FILE_NUMBER          (8)   //  $BadClus
#define SECURITY_FILE_NUMBER             (9)   //  $Secure
#define UPCASE_TABLE_NUMBER              (10)  //  $UpCase
#define EXTEND_NUMBER                    (11)  //  $Extend

#define LAST_SYSTEM_FILE_NUMBER          (11)
#define FIRST_USER_FILE_NUMBER           (16)

#define BITMAP_EXTEND_GRANULARITY               (64)
#define MFT_HOLE_GRANULARITY                    (32)
#define MFT_EXTEND_GRANULARITY                  (16)

#define MFT_DEFRAG_UPPER_THRESHOLD      (3)     //  Defrag if 1/8 of free space
#define MFT_DEFRAG_LOWER_THRESHOLD      (4)     //  Stop at 1/16 of free space

typedef ULONG ATTRIBUTE_TYPE_CODE;
typedef ATTRIBUTE_TYPE_CODE *PATTRIBUTE_TYPE_CODE;

#define $UNUSED                          (0X0)
#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $REPARSE_POINT                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $LOGGED_UTILITY_STREAM           (0x100) // defined in ntfsexp.h
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x1000)
#define $END                             (0xFFFFFFFF)

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {

    UCHAR  BytesPerSector[2];                               //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                            //  offset = 0x002
    UCHAR  ReservedSectors[2];                              //  offset = 0x003 (zero)
    UCHAR  Fats[1];                                         //  offset = 0x005 (zero)
    UCHAR  RootEntries[2];                                  //  offset = 0x006 (zero)
    UCHAR  Sectors[2];                                      //  offset = 0x008 (zero)
    UCHAR  Media[1];                                        //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                                //  offset = 0x00B (zero)
    UCHAR  SectorsPerTrack[2];                              //  offset = 0x00D
    UCHAR  Heads[2];                                        //  offset = 0x00F
    UCHAR  HiddenSectors[4];                                //  offset = 0x011 (zero)
    UCHAR  LargeSectors[4];                                 //  offset = 0x015 (zero)

} PACKED_BIOS_PARAMETER_BLOCK;                              //  sizeof = 0x019

typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct BIOS_PARAMETER_BLOCK {

    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;

} BIOS_PARAMETER_BLOCK;

typedef BIOS_PARAMETER_BLOCK *PBIOS_PARAMETER_BLOCK;

#define NtfsUnpackBios(Bios,Pbios) {                                       \
    CopyUchar2(&((Bios)->BytesPerSector),    &(Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), &(Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   &(Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              &(Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       &(Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           &(Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             &(Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     &(Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   &(Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             &(Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     &(Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      &(Pbios)->LargeSectors     ); \
}

typedef struct _PACKED_BOOT_SECTOR {

    UCHAR Jump[3];                                                              //  offset = 0x000
    UCHAR Oem[8];                                                               //  offset = 0x003
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;                                      //  offset = 0x00B
    UCHAR Unused[4];                                                            //  offset = 0x024
    LONGLONG NumberSectors;                                                     //  offset = 0x028
    LCN MftStartLcn;                                                            //  offset = 0x030
    LCN Mft2StartLcn;                                                           //  offset = 0x038
    CHAR ClustersPerFileRecordSegment;                                          //  offset = 0x040
    UCHAR Reserved0[3];
    CHAR DefaultClustersPerIndexAllocationBuffer;                               //  offset = 0x044
    UCHAR Reserved1[3];
    LONGLONG SerialNumber;                                                      //  offset = 0x048
    ULONG Checksum;                                                             //  offset = 0x050
    UCHAR BootStrap[0x200-0x054];                                               //  offset = 0x054

} PACKED_BOOT_SECTOR;                                                           //  sizeof = 0x200

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

typedef struct _FILE_RECORD_SEGMENT_HEADER {

    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000
    LSN Lsn;                                                        //  offset = 0x008
    USHORT SequenceNumber;                                          //  offset = 0x010
    USHORT ReferenceCount;                                          //  offset = 0x012
    USHORT FirstAttributeOffset;                                    //  offset = 0x014
    USHORT Flags;                                                   //  offset = 0x016
    ULONG FirstFreeByte;                                            //  offset = x0018
    ULONG BytesAvailable;                                           //  offset = 0x01C
    FILE_REFERENCE BaseFileRecordSegment;                           //  offset = 0x020
    USHORT NextAttributeInstance;                                   //  offset = 0x028
    USHORT SegmentNumberHighPart;                                  //  offset = 0x02A
    ULONG SegmentNumberLowPart;                                    //  offset = 0x02C
    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;                 //  offset = 0x030
} FILE_RECORD_SEGMENT_HEADER;
typedef FILE_RECORD_SEGMENT_HEADER *PFILE_RECORD_SEGMENT_HEADER;

typedef struct _FILE_RECORD_SEGMENT_HEADER_V0 {
    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000
    LSN Lsn;                                                        //  offset = 0x008
    USHORT SequenceNumber;                                          //  offset = 0x010
    USHORT ReferenceCount;                                          //  offset = 0x012
    USHORT FirstAttributeOffset;                                    //  offset = 0x014
    USHORT Flags;                                                   //  offset = 0x016
    ULONG FirstFreeByte;                                            //  offset = x0018
    ULONG BytesAvailable;                                           //  offset = 0x01C
    FILE_REFERENCE BaseFileRecordSegment;                           //  offset = 0x020
    USHORT NextAttributeInstance;                                   //  offset = 0x028
    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;                 //  offset = 0x02A
} FILE_RECORD_SEGMENT_HEADER_V0;

#define FILE_RECORD_SEGMENT_IN_USE       (0x0001)
#define FILE_NAME_INDEX_PRESENT		     (0x0002)
#define FILE_SYSTEM_FILE                 (0x0004)
#define FILE_VIEW_INDEX_PRESENT          (0x0008)

#define NtfsMaximumAttributeSize(FRSS) (                                               \
    (FRSS) - QuadAlign(sizeof(FILE_RECORD_SEGMENT_HEADER)) -                           \
    QuadAlign((((FRSS) / SEQUENCE_NUMBER_STRIDE) * sizeof(UPDATE_SEQUENCE_NUMBER))) -  \
    QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE))                                             \
)

typedef struct _ATTRIBUTE_RECORD_HEADER {

    ATTRIBUTE_TYPE_CODE TypeCode;                                   //  offset = 0x000
    ULONG RecordLength;                                             //  offset = 0x004
    UCHAR FormCode;                                                 //  offset = 0x008
    UCHAR NameLength;                                               //  offset = 0x009
    USHORT NameOffset;                                              //  offset = 0x00A
    USHORT Flags;                                                   //  offset = 0x00C
    USHORT Instance;                                                //  offset = 0x00E
    union {
        struct {
            ULONG ValueLength;                                      //  offset = 0x010
            USHORT ValueOffset;                                     //  offset = 0x014
            UCHAR ResidentFlags;                                    //  offset = 0x016
            UCHAR Reserved;                                         //  offset = 0x017
        } Resident;
        struct {
            VCN LowestVcn;                                          //  offset = 0x010
            VCN HighestVcn;                                         //  offset = 0x018
            USHORT MappingPairsOffset;                              //  offset = 0x020
            UCHAR CompressionUnit;                                  //  offset = 0x022
            UCHAR Reserved[5];                                      //  offset = 0x023
            LONGLONG AllocatedLength;                               //  offset = 0x028
            LONGLONG FileSize;                                      //  offset = 0x030
            LONGLONG ValidDataLength;                               //  offset = 0x038
            LONGLONG TotalAllocated;                                //  offset = 0x040
        } Nonresident;
    } Form;

} ATTRIBUTE_RECORD_HEADER , * PATTRIBUTE_RECORD_HEADER;

#define RESIDENT_FORM                    (0x00)
#define NONRESIDENT_FORM                 (0x01)

#define ATTRIBUTE_FLAG_COMPRESSION_MASK  (0x00FF)
#define ATTRIBUTE_FLAG_SPARSE            (0x8000)
#define ATTRIBUTE_FLAG_ENCRYPTED         (0x4000)
#define RESIDENT_FORM_INDEXED            (0x01)
#define NTFS_MAX_ATTR_NAME_LEN           (255)

#define SIZEOF_RESIDENT_ATTRIBUTE_HEADER (                         \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Resident.Reserved)+1 \
)

#define SIZEOF_FULL_NONRES_ATTR_HEADER (    \
    sizeof(ATTRIBUTE_RECORD_HEADER)         \
)

#define SIZEOF_PARTIAL_NONRES_ATTR_HEADER (                                 \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Nonresident.TotalAllocated)   \
)


typedef struct _STANDARD_INFORMATION {

    LONGLONG CreationTime;                                          //  offset = 0x000
    LONGLONG LastModificationTime;                                  //  offset = 0x008
    LONGLONG LastChangeTime;                                        //  offset = 0x010
    LONGLONG LastAccessTime;                                        //  offset = 0x018
    ULONG FileAttributes;                                           //  offset = 0x020
    ULONG MaximumVersions;                                          //  offset = 0x024
    ULONG VersionNumber;                                            //  offset = 0x028
    ULONG ClassId;                                                  //  offset = 0x02c
    ULONG OwnerId;                                                  //  offset = 0x030
    ULONG SecurityId;                                               //  offset = 0x034
    ULONGLONG QuotaCharged;                                         //  offset = 0x038
    ULONGLONG Usn;                                                  //  offset = 0x040
} STANDARD_INFORMATION;                                             //  sizeof = 0x048
typedef STANDARD_INFORMATION *PSTANDARD_INFORMATION;

typedef struct LARGE_STANDARD_INFORMATION {
    LONGLONG CreationTime;                                          //  offset = 0x000
    LONGLONG LastModificationTime;                                  //  offset = 0x008
    LONGLONG LastChangeTime;                                        //  offset = 0x010
    LONGLONG LastAccessTime;                                        //  offset = 0x018
    ULONG FileAttributes;                                           //  offset = 0x020
    ULONG MaximumVersions;                                          //  offset = 0x024
    ULONG VersionNumber;                                            //  offset = 0x028
    ULONG UnusedUlong;
    ULONG OwnerId;                                                  //  offset = 0x030
    ULONG SecurityId;                                               //  offset = 0x034

} LARGE_STANDARD_INFORMATION;
typedef LARGE_STANDARD_INFORMATION *PLARGE_STANDARD_INFORMATION;

//
//  This was the size of standard information prior to NT4.0
//

#define SIZEOF_OLD_STANDARD_INFORMATION  (0x30)

//
//  Define the file attributes, starting with the Fat attributes.
//

#define FAT_DIRENT_ATTR_READ_ONLY        (0x01)
#define FAT_DIRENT_ATTR_HIDDEN           (0x02)
#define FAT_DIRENT_ATTR_SYSTEM           (0x04)
#define FAT_DIRENT_ATTR_VOLUME_ID        (0x08)
#define FAT_DIRENT_ATTR_ARCHIVE          (0x20)
#define FAT_DIRENT_ATTR_DEVICE           (0x40)

typedef struct _ATTRIBUTE_LIST_ENTRY {
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;                          //  offset = 0x000
    USHORT RecordLength;                                            //  offset = 0x004
    UCHAR AttributeNameLength;                                      //  offset = 0x006
    UCHAR AttributeNameOffset;                                      //  offset = 0x007
    VCN LowestVcn;                                                  //  offset = 0x008
    MFT_SEGMENT_REFERENCE SegmentReference;                         //  offset = 0x010
    USHORT Instance;                                                //  offset = 0x018
    WCHAR AttributeName[1];                                         //  offset = 0x01A
} ATTRIBUTE_LIST_ENTRY;
typedef ATTRIBUTE_LIST_ENTRY *PATTRIBUTE_LIST_ENTRY;


typedef struct _DUPLICATED_INFORMATION {
    LONGLONG CreationTime;                                          //  offset = 0x000
    LONGLONG LastModificationTime;                                  //  offset = 0x008
    LONGLONG LastChangeTime;                                        //  offset = 0x010
    LONGLONG LastAccessTime;                                        //  offset = 0x018
    LONGLONG AllocatedLength;                                       //  offset = 0x020
    LONGLONG FileSize;                                              //  offset = 0x028
    ULONG FileAttributes;                                           //  offset = 0x030
    union {
        struct {
            USHORT  PackedEaSize;                                   //  offset = 0x034
            USHORT  Reserved;                                       //  offset = 0x036
        };
        ULONG  ReparsePointTag;                                     //  offset = 0x034
    };
} DUPLICATED_INFORMATION;                                           //  sizeof = 0x038
typedef DUPLICATED_INFORMATION *PDUPLICATED_INFORMATION;

#define DUP_FILE_NAME_INDEX_PRESENT      (0x10000000)
#define DUP_VIEW_INDEX_PRESENT        (0x20000000)

#define IsDirectory( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             DUP_FILE_NAME_INDEX_PRESENT ))

#define IsViewIndex( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             DUP_VIEW_INDEX_PRESENT ))

#define IsReadOnly( DUPLICATE )                                         \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_READONLY ))

#define IsHidden( DUPLICATE )                                           \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_HIDDEN ))

#define IsSystem( DUPLICATE )                                           \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_SYSTEM ))

#define IsEncrypted( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_ENCRYPTED ))

#define IsCompressed( DUPLICATE )                                       \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_COMPRESSED ))

#define BooleanIsDirectory( DUPLICATE )                                        \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    DUP_FILE_NAME_INDEX_PRESENT ))

#define BooleanIsReadOnly( DUPLICATE )                                         \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_READONLY ))

#define BooleanIsHidden( DUPLICATE )                                           \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_HIDDEN ))

#define BooleanIsSystem( DUPLICATE )                                           \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_SYSTEM ))

#define HasReparsePoint( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_REPARSE_POINT ))


typedef struct _FILE_NAME {
    FILE_REFERENCE ParentDirectory;                                 //  offset = 0x000
    DUPLICATED_INFORMATION Info;                                    //  offset = 0x008
    UCHAR FileNameLength;                                           //  offset = 0x040
    UCHAR Flags;                                                    //  offset = 0x041
    WCHAR FileName[1];                                              //  offset = 0x042
} FILE_NAME;
typedef FILE_NAME *PFILE_NAME;

#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)
#define NTFS_MAX_FILE_NAME_LENGTH       (255)
#define NTFS_MAX_LINK_COUNT             (1024)
#define FILE_NAME_IGNORE_DOS_ONLY        (0x80)

#define NtfsFileNameSizeFromLength(LEN) (                   \
    (sizeof( FILE_NAME ) + LEN - sizeof( WCHAR ))           \
)

#define NtfsFileNameSize(PFN) (                                             \
    (sizeof( FILE_NAME ) + ((PFN)->FileNameLength - 1) * sizeof( WCHAR ))   \
)

#define OBJECT_ID_KEY_LENGTH      16
#define OBJECT_ID_EXT_INFO_LENGTH 48

typedef struct _NTFS_OBJECTID_INFORMATION {
    FILE_REFERENCE FileSystemReference;
    UCHAR ExtendedInfo[OBJECT_ID_EXT_INFO_LENGTH];
} NTFS_OBJECTID_INFORMATION, *PNTFS_OBJECTID_INFORMATION;

#define OBJECT_ID_FLAG_CORRUPT           (0x00000001)

typedef struct _SECURITY_HASH_KEY
{
    ULONG   Hash;                           //  Hash value for descriptor
    ULONG   SecurityId;                     //  Security Id (guaranteed unique)
} SECURITY_HASH_KEY, *PSECURITY_HASH_KEY;

typedef struct _SECURITY_DESCRIPTOR_HEADER
{
    SECURITY_HASH_KEY HashKey;              //  Hash value for the descriptor
    ULONGLONG Offset;                       //  offset to beginning of header
    ULONG   Length;                         //  Length in bytes
} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

typedef struct _SII_ENTRY
{
	ULONG						SecuirtyId;
	SECURITY_DESCRIPTOR_HEADER	Header;
} SII_ENTRY, * PSII_ENTRY;

typedef struct _SDH_ENTRY
{
	SECURITY_HASH_KEY			Hash;
	SECURITY_DESCRIPTOR_HEADER	Header;
} SDH_ENTRY, * PSDH_ENTRY;

typedef struct _SECURITY_ENTRY {
    SECURITY_DESCRIPTOR_HEADER  security_descriptor_header;
    SECURITY_DESCRIPTOR         security;
} SECURITY_ENTRY, *PSECURITY_ENTRY;


#define GETSECURITYDESCRIPTORLENGTH(HEADER)         \
    ((HEADER)->Length - sizeof( SECURITY_DESCRIPTOR_HEADER ))

#define SetSecurityDescriptorLength(HEADER,LENGTH)  \
    ((HEADER)->Length = (LENGTH) + sizeof( SECURITY_DESCRIPTOR_HEADER ))

#define SECURITY_ID_INVALID              (0x00000000)
#define SECURITY_ID_FIRST                (0x00000100)

typedef struct _VOLUME_INFORMATION {

    LONGLONG Reserved;

    //
    //  Major and minor version number of NTFS on this volume,
    //  starting with 1.0.  The major and minor version numbers are
    //  set from the major and minor version of the Format and NTFS
    //  implementation for which they are initialized.  The policy
    //  for incementing major and minor versions will always be
    //  decided on a case by case basis, however, the following two
    //  paragraphs attempt to suggest an approximate strategy.
    //
    //  The major version number is incremented if/when a volume
    //  format change is made which requires major structure changes
    //  (hopefully never?).  If an implementation of NTFS sees a
    //  volume with a higher major version number, it should refuse
    //  to mount the volume.  If a newer implementation of NTFS sees
    //  an older major version number, it knows the volume cannot be
    //  accessed without performing a one-time conversion.
    //
    //  The minor version number is incremented if/when minor
    //  enhancements are made to a major version, which potentially
    //  support enhanced functionality through additional file or
    //  attribute record fields, or new system-defined files or
    //  attributes.  If an older implementation of NTFS sees a newer
    //  minor version number on a volume, it may issue some kind of
    //  warning, but it will proceed to access the volume - with
    //  presumably some degradation in functionality compared to the
    //  version of NTFS which initialized the volume.  If a newer
    //  implementation of NTFS sees a volume with an older minor
    //  version number, it may issue a warning and proceed.  In this
    //  case, it may choose to increment the minor version number on
    //  the volume and begin full or incremental upgrade of the
    //  volume on an as-needed basis.  It may also leave the minor
    //  version number unchanged, until some sort of explicit
    //  directive from the user specifies that the minor version
    //  should be updated.
    //

    UCHAR MajorVersion;                                             //  offset = 0x000

    UCHAR MinorVersion;                                             //  offset = 0x001

    //
    //  VOLUME_xxx flags.
    //

    USHORT VolumeFlags;                                             //  offset = 0x002

} VOLUME_INFORMATION;                                               //  sizeof = 0x004
typedef VOLUME_INFORMATION *PVOLUME_INFORMATION;

//
//  Volume is Dirty
//

#define VOLUME_DIRTY                     (0x0001)
#define VOLUME_RESIZE_LOG_FILE           (0x0002)
#define VOLUME_UPGRADE_ON_MOUNT          (0x0004)
#define VOLUME_MOUNTED_ON_40             (0x0008)
#define VOLUME_DELETE_USN_UNDERWAY       (0x0010)
#define VOLUME_REPAIR_OBJECT_ID          (0x0020)

#define VOLUME_CHKDSK_RAN_ONCE           (0x4000)   // this bit is used by autochk/chkdsk only
#define VOLUME_MODIFIED_BY_CHKDSK        (0x8000)


//
//  Common Index Header for Index Root and Index Allocation Buffers.
//  This structure is used to locate the Index Entries and describe
//  the free space in either of the two structures above.
//

typedef struct _INDEX_HEADER {

    //
    //  Offset from the start of this structure to the first Index
    //  Entry.
    //

    ULONG FirstIndexEntry;                                          //  offset = 0x000

    //
    //  Offset from the start of the first index entry to the first
    //  (quad-word aligned) free byte.
    //

    ULONG FirstFreeByte;                                            //  offset = 0x004

    //
    //  Total number of bytes available, from the start of the first
    //  index entry.  In the Index Root, this number must always be
    //  equal to FirstFreeByte, as the total attribute record will
    //  be grown and shrunk as required.
    //

    ULONG BytesAvailable;                                           //  offset = 0x008

    //
    //  INDEX_xxx flags.
    //

    UCHAR Flags;                                                    //  offset = 0x00C

    //
    //  Reserved to round up to quad word boundary.
    //

    UCHAR Reserved[3];                                              //  offset = 0x00D

} INDEX_HEADER;                                                     //  sizeof = 0x010
typedef INDEX_HEADER *PINDEX_HEADER;

//
//  INDEX_xxx flags
//

//
//  This Index or Index Allocation buffer is an intermediate node,
//  as opposed to a leaf in the Btree.  All Index Entries will have
//  a block down pointer.
//

#define INDEX_NODE                       (0x01)

//
//  Index Root attribute.  The index attribute consists of an index
//  header record followed by one or more index entries.
//

typedef struct _INDEX_ROOT {

    //
    //  Attribute Type Code of the attribute being indexed.
    //

    ATTRIBUTE_TYPE_CODE IndexedAttributeType;                       //  offset = 0x000

    //
    //  Collation rule for this index.
    //

    COLLATION_RULE CollationRule;                                   //  offset = 0x004

    //
    //  Size of Index Allocation Buffer in bytes.
    //

    ULONG BytesPerIndexBuffer;                                      //  offset = 0x008

    //
    //  Size of Index Allocation Buffers in units of blocks.
    //  Blocks will be clusters when index buffer is equal or
    //  larger than clusters and log blocks for large
    //  cluster systems.
    //

    UCHAR BlocksPerIndexBuffer;                                     //  offset = 0x00C

    //
    //  Reserved to round to quad word boundary.
    //

    UCHAR Reserved[3];                                              //  offset = 0x00D

    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;                                       //  offset = 0x010

} INDEX_ROOT;                                                       //  sizeof = 0x020
typedef INDEX_ROOT *PINDEX_ROOT;

//
//  Index Allocation record is used for non-root clusters of the
//  b-tree.  Each non root cluster is contained in the data part of
//  the index allocation attribute.  Each cluster starts with an
//  index allocation list header and is followed by one or more
//  index entries.
//

typedef struct _INDEX_ALLOCATION_BUFFER {

    //
    //  Multi-Sector Header as defined by the Cache Manager.  This
    //  structure will always contain the signature "INDX" and a
    //  description of the location and size of the Update Sequence
    //  Array.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000

    //
    //  Log File Sequence Number of last logged update to this Index
    //  Allocation Buffer.
    //

    LSN Lsn;                                                        //  offset = 0x008

    //
    //  We store the index block of this Index Allocation buffer for
    //  convenience and possible consistency checking.
    //

    VCN ThisBlock;                                                  //  offset = 0x010

    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;                                       //  offset = 0x018

    //
    //  Update Sequence Array to protect multi-sector transfers of
    //  the Index Allocation Buffer.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;                      //  offset = 0x028

} INDEX_ALLOCATION_BUFFER;
typedef INDEX_ALLOCATION_BUFFER *PINDEX_ALLOCATION_BUFFER;

//
//  Default size of index buffer and index blocks.
//

#define DEFAULT_INDEX_BLOCK_SIZE        (0x200)
#define DEFAULT_INDEX_BLOCK_BYTE_SHIFT  (9)

//
//  Index Entry.  This structure is common to both the resident
//  index list attribute and the Index Allocation records
//

typedef struct _INDEX_ENTRY {

    //
    //  Define a union to distinguish directory indices from view indices
    //

    union {

        //
        //  Reference to file containing the attribute with this
        //  attribute value.
        //

        FILE_REFERENCE FileReference;                               //  offset = 0x000

        //
        //  For views, describe the Data Offset and Length in bytes
        //

        struct {

            USHORT DataOffset;                                      //  offset = 0x000
            USHORT DataLength;                                      //  offset = 0x001
            ULONG ReservedForZero;                                  //  offset = 0x002
        };
    };

    //
    //  Length of this index entry, in bytes.
    //

    USHORT Length;                                                  //  offset = 0x008

    //
    //  Length of attribute value, in bytes.  The attribute value
    //  immediately follows this record.
    //

    USHORT AttributeLength;                                         //  offset = 0x00A

    //
    //  INDEX_ENTRY_xxx Flags.
    //

    USHORT Flags;                                                   //  offset = 0x00C

    //
    //  Reserved to round to quad-word boundary.
    //

    USHORT Reserved;                                                //  offset = 0x00E

    //
    //  If this Index Entry is an intermediate node in the tree, as
    //  determined by the INDEX_xxx flags, then a VCN  is stored at
    //  the end of this entry at Length - sizeof(VCN).
    //

} INDEX_ENTRY;                                                      //  sizeof = 0x010
typedef INDEX_ENTRY *PINDEX_ENTRY;

//
//  This entry is currently in the intermediate node form, i.e., it
//  has a Vcn at the end.
//

#define INDEX_ENTRY_NODE                 (0x0001)

//
//  This entry is the special END record for the Index or Index
//  Allocation buffer.
//

#define INDEX_ENTRY_END                  (0x0002)

//
//  This flag is *not* part of the on-disk structure.  It is defined
//  and reserved here for the convenience of the implementation to
//  help avoid allocating buffers from the pool and copying.
//

#define INDEX_ENTRY_POINTER_FORM         (0x8000)

#define NtfsIndexEntryBlock(IE) (                                       \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG))  \
    )

#define NtfsSetIndexEntryBlock(IE,IB) {                                         \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG)) = (IB);  \
    }

#define NtfsFirstIndexEntry(IH) (                       \
    (PINDEX_ENTRY)((PCHAR)(IH) + (IH)->FirstIndexEntry) \
    )

#define NtfsNextIndexEntry(IE) (                        \
    (PINDEX_ENTRY)((PCHAR)(IE) + (ULONG)(IE)->Length)   \
    )

#define NtfsCheckIndexBound(IE, IH) {                                                               \
    if (((PCHAR)(IE) < (PCHAR)(IH)) ||                                                              \
        (((PCHAR)(IE) + sizeof( INDEX_ENTRY )) > ((PCHAR)Add2Ptr((IH), (IH)->BytesAvailable)))) {   \
        NtfsRaiseStatus(IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );                        \
    }                                                                                               \
}


//
//  MFT Bitmap attribute
//
//  The MFT Bitmap is simply a normal attribute stream in which
//  there is one bit to represent the allocation state of each File
//  Record Segment in the MFT.  Bit clear means free, and bit set
//  means allocated.
//
//  Whenever the MFT Data attribute is extended, the MFT Bitmap
//  attribute must also be extended.  If the bitmap is still in a
//  file record segment for the MFT, then it must be extended and
//  the new bits cleared.  When the MFT Bitmap is in the Nonresident
//  form, then the allocation should always be sufficient to store
//  enough bits to describe the MFT, however ValidDataLength insures
//  that newly allocated space to the MFT Bitmap has an initial
//  value of all 0's.  This means that if the MFT Bitmap is extended,
//  the newly represented file record segments are automatically in
//  the free state.
//
//  No structure definition is required; the positional offset of
//  the file record segment is exactly equal to the bit offset of
//  its corresponding bit in the Bitmap.
//


//
//  USN Journal Instance
//
//  The following describe the current instance of the Usn journal.
//

typedef struct _USN_JOURNAL_INSTANCE {

#ifdef __cplusplus
    CREATE_USN_JOURNAL_DATA JournalData;
#else   // __cplusplus
    CREATE_USN_JOURNAL_DATA;
#endif  // __cplusplus

    ULONGLONG JournalId;
    USN LowestValidUsn;

} USN_JOURNAL_INSTANCE, *PUSN_JOURNAL_INSTANCE;

//
//  Reparse point index keys.
//
//  The index with all the reparse points that exist in a volume at a
//  given time contains entries with keys of the form
//                        <reparse tag, file record id>.
//  The data part of these records is empty.
//

typedef struct _REPARSE_INDEX_KEY {

    //
    //  The tag of the reparse point.
    //

    ULONG FileReparseTag;

    //
    //  The file record Id where the reparse point is set.
    //

    MFT_SEGMENT_REFERENCE FileId;

} REPARSE_INDEX_KEY, *PREPARSE_INDEX_KEY;



//
//  Ea Information attribute
//
//  This attribute is only present if the file/directory also has an
//  EA attribute.  It is used to store common EA query information.
//

typedef struct _EA_INFORMATION {

    //
    //  The size of buffer needed to pack these Ea's
    //

    USHORT PackedEaSize;                                            //  offset = 0x000

    //
    //  This is the count of Ea's with their NEED_EA
    //  bit set.
    //

    USHORT NeedEaCount;                                             //  offset = 0x002

    //
    //  The size of the buffer needed to return all Ea's
    //  in their unpacked form.
    //

    ULONG UnpackedEaSize;                                           //  offset = 0x004

}  EA_INFORMATION;                                                  //  sizeof = 0x008
typedef EA_INFORMATION *PEA_INFORMATION;


//
//  Define the struture of the quota data in the quota index.  The key for
//  the quota index is the 32 bit owner id.
//

typedef struct _QUOTA_USER_DATA {
    ULONG QuotaVersion;
    ULONG QuotaFlags;
    ULONGLONG QuotaUsed;
    ULONGLONG QuotaChangeTime;
    ULONGLONG QuotaThreshold;
    ULONGLONG QuotaLimit;
    ULONGLONG QuotaExceededTime;
    SID QuotaSid;
} QUOTA_USER_DATA, *PQUOTA_USER_DATA;

//
//  Define the size of the quota user data structure without the quota SID.
//

#define SIZEOF_QUOTA_USER_DATA FIELD_OFFSET(QUOTA_USER_DATA, QuotaSid)

//
//  Define the current version of the quote user data.
//

#define QUOTA_USER_VERSION 2

//
//  Define the quota flags.
//

#define QUOTA_FLAG_DEFAULT_LIMITS           (0x00000001)
#define QUOTA_FLAG_LIMIT_REACHED            (0x00000002)
#define QUOTA_FLAG_ID_DELETED               (0x00000004)
#define QUOTA_FLAG_USER_MASK                (0x00000007)

//
//  The following flags are only stored in the quota defaults index entry.
//

#define QUOTA_FLAG_TRACKING_ENABLED         (0x00000010)
#define QUOTA_FLAG_ENFORCEMENT_ENABLED      (0x00000020)
#define QUOTA_FLAG_TRACKING_REQUESTED       (0x00000040)
#define QUOTA_FLAG_LOG_THRESHOLD            (0x00000080)
#define QUOTA_FLAG_LOG_LIMIT                (0x00000100)
#define QUOTA_FLAG_OUT_OF_DATE              (0x00000200)
#define QUOTA_FLAG_CORRUPT                  (0x00000400)
#define QUOTA_FLAG_PENDING_DELETES          (0x00000800)

//
//  Define special quota owner ids.
//

#define QUOTA_INVALID_ID        0x00000000
#define QUOTA_DEFAULTS_ID       0x00000001
#define QUOTA_FISRT_USER_ID     0x00000100


//
//  Attribute Definition Table
//
//  The following struct defines the columns of this table.
//  Initially they will be stored as simple records, and ordered by
//  Attribute Type Code.
//

typedef struct _ATTRIBUTE_DEFINITION_COLUMNS {

    //
    //  Unicode attribute name.
    //

    WCHAR AttributeName[64];                                        //  offset = 0x000

    //
    //  Attribute Type Code.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;                          //  offset = 0x080

    //
    //  Default Display Rule for this attribute
    //

    DISPLAY_RULE DisplayRule;                                       //  offset = 0x084

    //
    //  Default Collation rule
    //

    COLLATION_RULE CollationRule;                                   //  offset = 0x088

    //
    //  ATTRIBUTE_DEF_xxx flags
    //

    ULONG Flags;                                                    //  offset = 0x08C

    //
    //  Minimum Length for attribute, if present.
    //

    LONGLONG MinimumLength;                                         //  offset = 0x090

    //
    //  Maximum Length for attribute.
    //

    LONGLONG MaximumLength;                                         //  offset = 0x098

} ATTRIBUTE_DEFINITION_COLUMNS;                                     //  sizeof = 0x0A0
typedef ATTRIBUTE_DEFINITION_COLUMNS *PATTRIBUTE_DEFINITION_COLUMNS;

//
//  ATTRIBUTE_DEF_xxx flags
//

//
//  This flag is set if the attribute may be indexed.
//

#define ATTRIBUTE_DEF_INDEXABLE          (0x00000002)

//
//  This flag is set if the attribute may occur more than once, such
//  as is allowed for the File Name attribute.
//

#define ATTRIBUTE_DEF_DUPLICATES_ALLOWED (0x00000004)

//
//  This flag is set if the value of the attribute may not be
//  entirely null, i.e., all binary 0's.
//

#define ATTRIBUTE_DEF_MAY_NOT_BE_NULL    (0x00000008)

//
//  This attribute must be indexed, and no two attributes may exist
//  with the same value in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_INDEXED    (0x00000010)

//
//  This attribute must be named, and no two attributes may exist
//  with the same name in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_NAMED      (0x00000020)

//
//  This attribute must be in the Resident Form.
//

#define ATTRIBUTE_DEF_MUST_BE_RESIDENT   (0x00000040)

//
//  Modifications to this attribute should be logged even if the
//  attribute is nonresident.
//

#define ATTRIBUTE_DEF_LOG_NONRESIDENT    (0X00000080)



//
//  MACROS
//
//  Define some macros that are helpful for manipulating NTFS on
//  disk structures.
//

//
//  The following macro returns the first attribute record in a file
//  record segment.
//
//      PATTRIBUTE_RECORD_HEADER
//      NtfsFirstAttribute (
//          IN PFILE_RECORD_SEGMENT_HEADER FileRecord
//          );
//
//  The following macro takes a pointer to an attribute record (or
//  attribute list entry) and returns a pointer to the next
//  attribute record (or attribute list entry) in the list
//
//      PVOID
//      NtfsGetNextRecord (
//          IN PATTRIB_RECORD or PATTRIB_LIST_ENTRY Struct
//          );
//
//
//  The following macro takes as input a attribute record or
//  attribute list entry and initializes a string variable to the
//  name found in the record or entry.  The memory used for the
//  string buffer is the memory found in the attribute.
//
//      VOID
//      NtfsInitializeStringFromAttribute (
//          IN OUT PUNICODE_STRING Name,
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      VOID
//      NtfsInitializeStringFromEntry (
//          IN OUT PUNICODE_STRING Name,
//          IN PATTRIBUTE_LIST_ENTRY Entry
//          );
//
//
//  The following two macros assume resident form and should only be
//  used when that state is known.  They return a pointer to the
//  value a resident attribute or a pointer to the byte one beyond
//  the value.
//
//      PVOID
//      NtfsGetValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      PVOID
//      NtfsGetBeyondValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//  The following two macros return a boolean value indicating if
//  the input attribute record is of the specified type code, or the
//  indicated value.  The equivalent routine to comparing attribute
//  names cannot be defined as a macro and is declared in AttrSup.c
//
//      BOOLEAN
//      NtfsEqualAttributeTypeCode (
//          IN PATTRIBUTE_RECORD_HEADER Attribute,
//          IN ATTRIBUTE_TYPE_CODE Code
//          );
//
//      BOOLEAN
//      NtfsEqualAttributeValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute,
//          IN PVOID Value,
//          IN ULONG Length
//          );
//

#define NtfsFirstAttribute(FRS) (                                          \
    (PATTRIBUTE_RECORD_HEADER)((PCHAR)(FRS) + (FRS)->FirstAttributeOffset) \
)

#define NtfsGetNextRecord(STRUCT) (                    \
    (PVOID)((PUCHAR)(STRUCT) + (STRUCT)->RecordLength) \
)

#define NtfsCheckRecordBound(PTR, SPTR, SIZ) {                                          \
    if (((PCHAR)(PTR) < (PCHAR)(SPTR)) || ((PCHAR)(PTR) >= ((PCHAR)(SPTR) + (SIZ)))) {  \
        NtfsRaiseStatus(IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );            \
    }                                                                                   \
}

#define NtfsInitializeStringFromAttribute(NAME,ATTRIBUTE) {                \
    (NAME)->Length = (USHORT)(ATTRIBUTE)->NameLength << 1;                 \
    (NAME)->MaximumLength = (NAME)->Length;                                \
    (NAME)->Buffer = (PWSTR)Add2Ptr((ATTRIBUTE), (ATTRIBUTE)->NameOffset); \
}

#define NtfsInitializeStringFromEntry(NAME,ENTRY) {                        \
    (NAME)->Length = (USHORT)(ENTRY)->AttributeNameLength << 1;            \
    (NAME)->MaximumLength = (NAME)->Length;                                \
    (NAME)->Buffer = (PWSTR)((ENTRY) + 1);                                 \
}

#define NtfsGetValue(ATTRIBUTE) (                                \
    Add2Ptr((ATTRIBUTE), (ATTRIBUTE)->Form.Resident.ValueOffset) \
)

#define NtfsGetBeyondValue(ATTRIBUTE) (                                      \
    Add2Ptr(NtfsGetValue(ATTRIBUTE), (ATTRIBUTE)->Form.Resident.ValueLength) \
)

#define NtfsEqualAttributeTypeCode(A,C) ( \
    (C) == (A)->TypeCode                  \
)

#define NtfsEqualAttributeValue(A,V,L) (     \
    NtfsIsAttributeResident(A) &&            \
    (A)->Form.Resident.ValueLength == (L) && \
    RtlEqualMemory(NtfsGetValue(A),(V),(L))  \
)

#pragma pack()

#endif //  _NTFS_


