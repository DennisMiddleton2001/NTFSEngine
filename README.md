# Implementation Details
The DLL code included in this repo exports file system structures in a very organized format for using in GUI or command line tools.

It allows low-level analysis of the NTFS file system without actually mounting the volume.  It works entirely on DASD and bypasses all file system security.  This even allows for analysis of VSS differential shadow copies that wouldn't normally be accessible through conventional tools.  This is the equivalent of file system God Mode!

# FSReflector.exe GUI (Example of implementation on a C# GUI Application)
NTFS File System Reflector

https://www.youtube.com/watch?v=J6JS_sg71ok

# NFI.exe Command Line Implementation
I wrote a small c# application named Nfi that implements this DLL via LoadLibrary & PINVOKE as an example.  Feel free to download the public repo if you want to customize the user experience.

https://github.com/DennisMiddleton2001/Nfi

Sample Output:
-----------------------

    _FILE_RECORD_SEGMENT_HEADER  {
    _MULTI_SECTOR_HEADER MultiSectorHeader {
            UInt32     Signature             : 0x454c4946 "FILE"
            UInt16     SequenceArrayOffset   : 0x0030
            UInt16     SequenceArraySize     : 0x0003
    }
    Int64      Lsn                   : 0x00000004319215b3
    UInt16     SequenceNumber        : 0x0005
    UInt16     ReferenceCount        : 0x0001
    UInt16     FirstAttributeOffset  : 0x0038
    UInt16     Flags                 : 0x0003
    UInt32     FirstFreeByte         : 0x000002c0
    UInt32     BytesAvailable        : 0x00000400
    _MFT_SEGMENT_REFERENCE BaseFileRecordSegment {
        UInt64     SegmentNumber         : 0x0000000000000000  <\\FRS:00000000>
        UInt16     SequenceNumber        : 0x0000
    }
    UInt16     NextAttributeInstance : 0x001a
    _MFT_SEGMENT_REFERENCE ThisFileReference {
        UInt64     SegmentNumber         : 0x0000000000000005  <\\FRS:00000005>
        UInt16     SequenceNumber        : 0x0005
    }

    _ATTRIBUTE_RECORD_HEADER  {            $STANDARD_INFORMATION:"" (<\\STR:00000005:10:>)
        UInt32     TypeCode              : 0x00000010
        UInt32     RecordLength          : 0x00000060
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x00
        UInt16     NameOffset            : 0x0018
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0000
        Resident {
            UInt32     ValueLength           : 0x00000048
            UInt16     ValueOffset           : 0x0018
            UCHAR      ResidentFlags         : 0x00
            UCHAR      Reserved              : 0x00
        }
    }
    
        _STANDARD_INFORMATION  {
            Int64      CreationTime          : 0x01d861d1bb3e4d9f 05/07/2022 LCL 01:17:22.491
            Int64      LastModificationTime  : 0x01dacfb549d3f77b 07/06/2024 LCL 11:00:49.550
            Int64      LastChangeTime        : 0x01dacfb549d3f77b 07/06/2024 LCL 11:00:49.550
            Int64      LastAccessTime        : 0x01dacfc49671985f 07/06/2024 LCL 12:50:20.541
            UInt32     FileAttributes        : 0x00000006
            UInt32     MaximumVersions       : 0x00000000
            UInt32     VersionNumber         : 0x00000000
            UInt32     ClassId               : 0x00000000
            UInt32     OwnerId               : 0x00000000
            UInt32     SecurityId            : 0x00000105
            UInt64     QuotaCharged          : 0x0000000000000000
            UInt64     Usn                   : 0x000000003b3a7288
        }
        
        
    _ATTRIBUTE_RECORD_HEADER  {            $FILE_NAME:"" (<\\STR:00000005:30:>)
        UInt32     TypeCode              : 0x00000030
        UInt32     RecordLength          : 0x00000060
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x00
        UInt16     NameOffset            : 0x0018
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0001
        Resident {
            UInt32     ValueLength           : 0x00000044
            UInt16     ValueOffset           : 0x0018
            UCHAR      ResidentFlags         : 0x01
            UCHAR      Reserved              : 0x00
        }
    }
    
        _FILE_NAME  {
            _MFT_SEGMENT_REFERENCE ParentDirectory {
                UInt64     SegmentNumber         : 0x0000000000000005  <\\FRS:00000005>
                UInt16     SequenceNumber        : 0x0005
            }
            _DUPLICATED_INFORMATION Info {
                Int64      CreationTime          : 0x01d861d1bb3e4d9f 05/07/2022 LCL 01:17:22.491
                Int64      LastModificationTime  : 0x01d958b3d54deaac 03/17/2023 LCL 05:35:38.518
                Int64      LastChangeTime        : 0x01d958b3d54deaac 03/17/2023 LCL 05:35:38.518
                Int64      LastAccessTime        : 0x01d958b3d54deaac 03/17/2023 LCL 05:35:38.518
                Int64      AllocatedLength       : 0x0000000000000000
                Int64      FileSize              : 0x0000000000000000
                UInt32     FileAttributes        : 0x10000006
                PackedEaSize          : 0x0000
                UInt32     ReparsePointTag       : 0x00000000
            }
            UCHAR      FileNameLength        : 0x01
            UCHAR      Flags                 : 0x03
            .....      FileName              :   "."  (<\\DIR:00000005>)
        }
        
    _ATTRIBUTE_RECORD_HEADER  {            $OBJECT_ID:"" (<\\STR:00000005:40:>)
        UInt32     TypeCode              : 0x00000040
        UInt32     RecordLength          : 0x00000028
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x00
        UInt16     NameOffset            : 0x0000
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0019
        Resident {
            UInt32     ValueLength           : 0x00000010
            UInt16     ValueOffset           : 0x0018
            UCHAR      ResidentFlags         : 0x00
            UCHAR      Reserved              : 0x00
        }
    }
    
        _FILE_OBJECTID_BUFFER  {
            GUID BirthVolumeId {00000000-0000-0000-0000000000000000}
            GUID BirthObjectId {00000000-0000-0000-0000000000000000}
            GUID DomainId      {00000000-0000-0000-0000000000000000}
            GUID ObjectId      {c6bb5f1f-4a61-11ee-96728a654fd33d61}
            CHAR ExtendedInfo[] {
                ...
            }
        }
        
    _ATTRIBUTE_RECORD_HEADER  {            $INDEX_ROOT:"$I30" (<\\STR:00000005:90:$I30>)
        UInt32     TypeCode              : 0x00000090
        UInt32     RecordLength          : 0x000000b8
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x04
        UInt16     NameOffset            : 0x0018
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0016
        Resident {
            UInt32     ValueLength           : 0x00000098
            UInt16     ValueOffset           : 0x0020
            UCHAR      ResidentFlags         : 0x00
            UCHAR      Reserved              : 0x00
        }
    }
    
        _INDEX_ROOT  {
            UInt32     IndexedAttributeType  : 0x00000030
            UInt32     CollationRule         : 0x00000001
            UInt32     BytesPerIndexBuffer   : 0x00001000
            _INDEX_HEADER  {
                UInt32     FirstIndexEntry       : 0x00000010
                UInt32     FirstFreeByte         : 0x00000088
                UInt32     BytesAvailable        : 0x00000088
                UCHAR      Flags                 : 0x01
                .....      Reserved              : 
            }
        }
        
        _INDEX_ENTRY  {
            _MFT_SEGMENT_REFERENCE FileReference {
                UInt64     SegmentNumber         : 0x0000000000000024  <\\FRS:00000024>
                UInt16     SequenceNumber        : 0x0001
            }
            UInt16     Length                : 0x0060
            UInt16     AttributeLength       : 0x0046
            UInt16     Flags                 : 0x0001
            .....      Reserved              : 
            _FILE_NAME  {
                _MFT_SEGMENT_REFERENCE ParentDirectory {
                    UInt64     SegmentNumber         : 0x0000000000000005  <\\FRS:00000005>
                    UInt16     SequenceNumber        : 0x0005
                }
                _DUPLICATED_INFORMATION Info {
                    Int64      CreationTime          : 0x01d9589ef60128fe 03/17/2023 LCL 03:06:13.948
                    Int64      LastModificationTime  : 0x01d9993009ff5bed 06/07/2023 LCL 07:05:58.935
                    Int64      LastChangeTime        : 0x01d9993009ff5bed 06/07/2023 LCL 07:05:58.935
                    Int64      LastAccessTime        : 0x01dacfc216e39bcc 07/06/2024 LCL 12:32:27.547
                    Int64      AllocatedLength       : 0x0000000000000000
                    Int64      FileSize              : 0x0000000000000000
                    UInt32     FileAttributes        : 0x10000002
                    PackedEaSize          : 0x0000
                    UInt32     ReparsePointTag       : 0x00000000
                }
                UCHAR      FileNameLength        : 0x02
                UCHAR      Flags                 : 0x00
                .....      FileName              :   "hp"  (<\\DIR:00000024>)
            }
        }
        Int64      DownPointerVcn        : 0x0000000000000000
        
        
        _INDEX_ENTRY  {
            UInt16     DataOffset            : 0x0000
            UInt16     DataLength            : 0x0000
            UInt32     ReservedForZero       : 0x00000000
            UInt16     Length                : 0x0018
            UInt16     AttributeLength       : 0x0000
            UInt16     Flags                 : 0x0003
            .....      Reserved              : 
        }
        Int64      DownPointerVcn        : 0x0000000000000001
        
        
    _ATTRIBUTE_RECORD_HEADER  {            $INDEX_ALLOCATION:"$I30" (<\\STR:00000005:a0:$I30>)
        UInt32     TypeCode              : 0x000000a0
        UInt32     RecordLength          : 0x00000050
        UCHAR      FormCode              : 0x01
        UCHAR      NameLength            : 0x04
        UInt16     NameOffset            : 0x0040
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0018
        Nonresident {
            Int64      LowestVcn             : 0x0000000000000000
            Int64      HighestVcn            : 0x0000000000000001
            UInt16     MappingPairsOffset    : 0x0048
            UCHAR      CompressionUnit       : 0x00
            .....      Reserved              : 
            Int64      AllocatedLength       : 0x0000000000002000
            Int64      FileSize              : 0x0000000000002000
            Int64      ValidDataLength       : 0x0000000000002000
        }
    }
    
        Extents  {
            VCN = 0x0 LEN = 0x2 : LCN = 0x31ca    <\\LCN:31ca:2>
        }
        
    _ATTRIBUTE_RECORD_HEADER  {            $BITMAP:"$I30" (<\\STR:00000005:b0:$I30>)
        UInt32     TypeCode              : 0x000000b0
        UInt32     RecordLength          : 0x00000028
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x04
        UInt16     NameOffset            : 0x0018
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0017
        Resident {
            UInt32     ValueLength           : 0x00000008
            UInt16     ValueOffset           : 0x0020
            UCHAR      ResidentFlags         : 0x00
            UCHAR      Reserved              : 0x00
        }
    }
    
        BITMAP {
        --------+--------+--------+--------
        0x0   11...... ........ ........ ........  
        0x20   ........ ........ ........ ........  
        --------+--------+--------+--------
        }
        
    _ATTRIBUTE_RECORD_HEADER  {            $LOGGED_UTILITY_STREAM:"$TXF_DATA" (<\\STR:00000005:100:$TXF_DATA>)
        UInt32     TypeCode              : 0x00000100
        UInt32     RecordLength          : 0x00000068
        UCHAR      FormCode              : 0x00
        UCHAR      NameLength            : 0x09
        UInt16     NameOffset            : 0x0018
        UInt16     Flags                 : 0x0000
        UInt16     Instance              : 0x0009
        Resident {
            UInt32     ValueLength           : 0x00000038
            UInt16     ValueOffset           : 0x0030
            UCHAR      ResidentFlags         : 0x00
            UCHAR      Reserved              : 0x00
        }
    }
    
        0x00000000   05 00 00 00 00 00 05 00 01 00 00 00 01 00 00 00   ................
        0x00000010   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
        0x00000020   00 00 00 00 00 00 00 00 00 f0 9b 00 00 00 00 00   ......... .....
        0x00000030   02 00 00 00 00 00 00 00                           ........
        
