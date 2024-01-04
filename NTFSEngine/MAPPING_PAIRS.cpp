#include "StdAfx.h"

#define DF_ERROR 1
#define DF_WARN  2
#define DF_INFO  4
#define DF_VER   8

VOID
MAPPING_PAIRS::MpDbgPrint(
LONG			MessageImportance,
CHAR			* FunctionName,
LONG			Line,
CHAR			* szFormat,
...
)
{
	if (VolumeDebugFlags & MessageImportance)
	{
		va_list ArgList;
		size_t StringLen = strlen(FunctionName) + strlen(szFormat) + 16;
		CHAR * FormatString = new CHAR[StringLen];
		sprintf(FormatString, "%s (%lu) : %s", FunctionName, Line, szFormat);
		va_start(ArgList, szFormat);
		vprintf(FormatString, ArgList);
		delete[] FormatString;
	}
}


MAPPING_PAIRS::~MAPPING_PAIRS()
{	
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	DeleteMapArray();
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT\n");
}

MAPPING_PAIRS::MAPPING_PAIRS()
{
	VolumeDebugFlags = DF_ERROR | DF_WARN;

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	_Initialized = FALSE;
	_pMapArray = NULL;
	_NumberOfRuns = 0;
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT");
}

MAPPING_PAIRS::MAPPING_PAIRS(DWORD Flags)
{
	VolumeDebugFlags = Flags;

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	
	_Initialized = FALSE;
	_pMapArray = NULL;
	_NumberOfRuns = 0;
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT\n");
}

MAPPING_PAIRS::MAPPING_PAIRS(PATTRIBUTE_RECORD_HEADER pAttribute, DWORD Flags)
{
	VolumeDebugFlags = Flags;

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	_Initialized = FALSE;
	_pMapArray=NULL;
	_NumberOfRuns=0;
	if (AppendRunsToArray(pAttribute)==ERROR_SUCCESS)
	{
		_Initialized = TRUE;
		MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "Mapping Pairs Initialized");
	}
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT");
}

BOOL MAPPING_PAIRS::ArrayIsValid()
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	BOOL bReturn = _Initialized;
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - BOOL %lu\n", bReturn);
	return bReturn;
}

LONG MAPPING_PAIRS::GetNumberOfMappingPairs(PCHAR pMappingPairs, DWORD * pError)
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	PCHAR pThisMappingPair = pMappingPairs;
	LONG Number = 0;
	MAPPING_PAIRS_ARRAY MpArrayEntry;
	RtlZeroMemory(&MpArrayEntry,sizeof(MAPPING_PAIRS_ARRAY));
	
    try
    {
		*pError = ERROR_SUCCESS;

		do
		{
			pThisMappingPair = GetRunInformation(pThisMappingPair, &MpArrayEntry);
			Number++;
		} while (pThisMappingPair);
    }
    catch (...)
    {
		MpDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "Corrupted Mapping Pairs Found.\n");
        *pError = ERROR_INVALID_DATA;
    }

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - NumberOfMappingPairs %lu - Error %lu\n", Number, *pError);
	return Number;
}

LONGLONG	MAPPING_PAIRS::ExtractPositiveValue(PCHAR pMappingPair, CHAR SizeV)
{	
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "ENTER\n");
	LONGLONG Value = 0;
    RtlCopyMemory(&Value,pMappingPair,SizeV);
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "EXIT - Value %I64i\n", Value);
	return Value;
}

LONGLONG	MAPPING_PAIRS::ExtractNegativeValue(PCHAR pMappingPair, CHAR SizeV)
{	
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "ENTER\n");
	LONGLONG Value = -1;
    RtlCopyMemory(&Value,pMappingPair,SizeV);
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "EXIT - Value %I64i\n", Value);
	return Value;
}

PCHAR		MAPPING_PAIRS::GetRunInformation(PCHAR pMappingPairStart, MAPPING_PAIRS_ARRAY * pMpArrayEntry)
{
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "ENTER\n");
	CHAR SizeV, SizeL;
	CHAR CountByte;
	
	PCHAR pMappingPair = pMappingPairStart;
	
	//Extract the count byte and advance pointer 1 byte.
	CountByte = *pMappingPair++;

	//Set CurrentVcn to the old next value.
	pMpArrayEntry->CurrentVcn=pMpArrayEntry->NextVcn;

	// High 4 bits are the SizeL Low 4 bits are SizeV
	SizeV = CountByte%16;
	SizeL = (CountByte)/16;

	if (SizeV > 8 || SizeL > 8)
	{
		MpDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "Invalid Count Byte Size.\n");
		throw "Invalid mapping pairs";
	}

	//Extract the next vcn change bytes.
	if (pMappingPair[SizeV-1] & 0x80 )
	{
		pMpArrayEntry->NextVcn += ExtractNegativeValue(pMappingPair,SizeV);
	}
	else
	{
		pMpArrayEntry->NextVcn += ExtractPositiveValue(pMappingPair,SizeV);
	}

	pMappingPair += SizeV;

	//
	// Extract the lcn change bytes.
	//
	if (pMappingPair[SizeL-1] & 0x80 )
	{
		pMpArrayEntry->CurrentLcn += ExtractNegativeValue(pMappingPair,SizeL);
	}
	else
	{
		pMpArrayEntry->CurrentLcn += ExtractPositiveValue(pMappingPair,SizeL);
	}

	pMappingPair += SizeL;

	// Check to see if the run is allocated.
	if (SizeL==0 || pMpArrayEntry->CurrentLcn == -1)
	{
		pMpArrayEntry->Allocated = 0;
	}
	else
	{
		// Compute the actual number of allocated LCNs.
		pMpArrayEntry->Allocated = (LONG) (pMpArrayEntry->NextVcn - pMpArrayEntry->CurrentVcn);
	}

	// If the count byte is empty, there are no more pairs.
	if (!*pMappingPair)
	{
		MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "EXIT - Count Byte Terminator Found\n");
		return NULL;
	}
		
	MpDbgPrint(DF_VER, __FUNCTION__, __LINE__, "EXIT\n");
	return pMappingPair;
}

DWORD		MAPPING_PAIRS::GrowMapArray(LONG NumberOfRuns)
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	DWORD Error;
	PMAPPING_PAIRS_ARRAY pOldMapArray = _pMapArray;
	LONG OldArraySize = _NumberOfRuns;

	_pMapArray = new MAPPING_PAIRS_ARRAY [OldArraySize+NumberOfRuns+1];

	if (_pMapArray)
	{
		// If we're growing the array from a previously allocated size, then copy the old array to the new memory.
		if (OldArraySize)
		{
			// Copy the array to the new memory.
			RtlCopyMemory((PVOID) _pMapArray, (PVOID) pOldMapArray, OldArraySize * sizeof(MAPPING_PAIRS_ARRAY) );
			// Free the old memory.
			delete [] pOldMapArray;
		}

		_NumberOfRuns = OldArraySize+NumberOfRuns;
		Error = ERROR_SUCCESS;
	}
	else
	{
		_NumberOfRuns = 0;
		delete [] pOldMapArray;
		Error = ERROR_NOT_ENOUGH_MEMORY;
	}

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
	return Error;
}

DWORD MAPPING_PAIRS::DeleteMapArray(VOID)
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	_Initialized = FALSE;
	_NumberOfRuns = 0;
	if (_pMapArray)
	{
		delete _pMapArray;
		_pMapArray = NULL;
	}

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", ERROR_SUCCESS);
	return ERROR_SUCCESS;
}

DWORD MAPPING_PAIRS::AppendRunsToArray(PATTRIBUTE_RECORD_HEADER pAttribute)
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	DWORD Error;
	PCHAR pMappingPairs;
	LONG i;
	LONG RunsThisInstance;
	MAPPING_PAIRS_ARRAY ThisMpArrayEntry;
	
	_CompressionChunkSize = (pAttribute->Form.Nonresident.CompressionUnit) ? (VCN) 1 << pAttribute->Form.Nonresident.CompressionUnit : 0;
	
	RtlZeroMemory(&ThisMpArrayEntry,sizeof(MAPPING_PAIRS_ARRAY));

	// There are no runs in a resident form attribute.
	if (pAttribute->FormCode == RESIDENT_FORM) 
	{
		Error = ERROR_INVALID_PARAMETER;
		MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
		return Error;
	}

	// If this is the first instance, then let's start from scratch.
	if (pAttribute->Form.Nonresident.LowestVcn == 0)
	{
		DeleteMapArray();
		RtlCopyMemory((PVOID) &_BaseAttribute, (PVOID) pAttribute, sizeof(ATTRIBUTE_RECORD_HEADER));
	}
	
	// Find out how many runs are in the mapping pairs.
	pMappingPairs = (PCHAR) ADD_PTR(pAttribute, pAttribute->Form.Nonresident.MappingPairsOffset);
	RunsThisInstance = GetNumberOfMappingPairs(pMappingPairs, &Error);
	if (Error)
	{
		MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
		return Error;
	}
	
	Error=GrowMapArray(RunsThisInstance);
	if (Error)
	{
		MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
		return Error;
	}

	// Start at the old array size and grow from there.
	i=_NumberOfRuns - RunsThisInstance;
	
	try
	{
		//Initialize the map array entry with the next VCN so GetRunInfo knows what vcn to use as a starting point.
		ThisMpArrayEntry.NextVcn = pAttribute->Form.Nonresident.LowestVcn;
		do
		{
			pMappingPairs = GetRunInformation(pMappingPairs, &ThisMpArrayEntry);

			_pMapArray[i].CurrentVcn = ThisMpArrayEntry.CurrentVcn;
			_pMapArray[i].NextVcn    = ThisMpArrayEntry.NextVcn;
			_pMapArray[i].CurrentLcn = ThisMpArrayEntry.CurrentLcn;
			_pMapArray[i].Allocated  = ThisMpArrayEntry.Allocated;
			if (i && (_pMapArray[i-1].NextVcn != _pMapArray[i].CurrentVcn) )
			{
				MpDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "Invalid run length LCN 0x%08I64x.\n", _pMapArray[i].CurrentLcn);
				throw "Invalid Mapping Pairs";
			}
			
			i++;
			if (!pMappingPairs && i<_NumberOfRuns) DebugBreak();

		} while ( pMappingPairs && (i<_NumberOfRuns) );
	}
	catch (...)
	{
		//We had an error working with the mapping pairs.
		DeleteMapArray();		
		Error = ERROR_INVALID_DATA;
		MpDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "Corrupted Mapping Pairs Found.\n");
	}

	// Ensure that high and low VCN make sense according to the mapping pairs and the attribute record.
	if ((_pMapArray[_NumberOfRuns - RunsThisInstance].CurrentVcn != pAttribute->Form.Nonresident.LowestVcn) && (_pMapArray[i-1].NextVcn-1 !=pAttribute->Form.Nonresident.HighestVcn))
	{
		MpDbgPrint(DF_ERROR, __FUNCTION__, __LINE__, "Corrupted Mapping Pairs Found.\n");
		DebugBreak();
	}
	
	// Check for cross linkages.
	for (i = 0; i < _NumberOfRuns; i++)
	{
		for (LONG j = 0; j < _NumberOfRuns; j++)
		{			
			if (_pMapArray[j].Allocated)
			{
				// Go through all LCN's in J and compare with the LCN ranges in I.
				for (LONG k = 0 ; k < _pMapArray[j].Allocated; k++)
				{
					LONGLONG ThisLcn = (LONGLONG) k + _pMapArray[j].CurrentLcn;

					// If two LCN's occupy the same space and not the same starting VCN, then they are cross linked.
					if (ThisLcn >= _pMapArray[i].CurrentLcn &&
						ThisLcn < (_pMapArray[i].CurrentLcn + _pMapArray[i].Allocated) &&
						_pMapArray[j].CurrentVcn != _pMapArray[i].CurrentVcn)
					{
						MpDbgPrint(DF_WARN, __FUNCTION__, __LINE__, "Internal Cross Linkage found in VCN 0x%016I64x LCN 0x%016I64x\n", _pMapArray[j].CurrentVcn+k, ThisLcn);
					}
				}
			}			
		}
	}

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", Error);
	return Error;
}


LONGLONG MAPPING_PAIRS::GetLcnFromVcn(PATTRIBUTE_RECORD_HEADER pAttribute,VCN Vcn, PLONG pLength)
{
	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "ENTER\n");
	LONG ThisRun;
	MAPPING_PAIRS m;
	
	for (ThisRun = 0 ; ThisRun < _NumberOfRuns ; ThisRun++ )
	{
		// Check to see if the Vcn lies within this run.
		if ( (Vcn >= _pMapArray[ThisRun].CurrentVcn) && (Vcn < _pMapArray[ThisRun].NextVcn) )
		{
			//Compute the LCN for this VCN.
			SetLastError(ERROR_SUCCESS);
			*pLength = (LONG) (_pMapArray[ThisRun].NextVcn - _pMapArray[ThisRun].CurrentVcn);
			MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", ERROR_SUCCESS);
			return (_pMapArray[ThisRun].CurrentLcn + (Vcn - (_pMapArray[ThisRun].CurrentVcn)));
		}
	}

	SetLastError(ERROR_NOT_FOUND);

	MpDbgPrint(DF_INFO, __FUNCTION__, __LINE__, "EXIT - Error %lu\n", ERROR_NOT_FOUND);
	return -1;
}
