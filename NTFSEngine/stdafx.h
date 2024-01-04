#pragma once

#define _CRT_SECURE_NO_DEPRECATE	
#define _CRT_SECURE_NO_WARNINGS
//#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define BUILD_NUMBER __COUNTER__ 


#include <stdio.h>
#include <tchar.h>
#include <typeinfo>
#include <windows.h>
#include <winioctl.h>
#include "RW_BUFFER.h"
#include "hex_dump.h"

#include "ntfs.h"
#include "ntfscommon.h"
#include "dump_types.h"
#include "mapping_pairs.h"
#include "NTFS_VOLUME.h"
#include "cmdline.h"

#define USA_SECTOR_SIZE 0x200

#define ERROR_EXIT(E)								\
if ((E))											\
{													\
	return (E);										\
}

