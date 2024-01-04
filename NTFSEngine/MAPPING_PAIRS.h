class MAPPING_PAIRS
{
public:	
	LONG   _NumberOfRuns;
	LONG   _CompressionChunkSize;
	PMAPPING_PAIRS_ARRAY _pMapArray;
	ATTRIBUTE_RECORD_HEADER _BaseAttribute;
	MAPPING_PAIRS();
	MAPPING_PAIRS(DWORD DebugFlags);
	MAPPING_PAIRS(PATTRIBUTE_RECORD_HEADER pAttribute, DWORD DebugFlags);
	~MAPPING_PAIRS();
	VOID MpDbgPrint(LONG MessageImportance, CHAR * FunctionName, LONG Line, CHAR * szFormat, ...);
	DWORD		VolumeDebugFlags;
	BOOL		ArrayIsValid();
	LONG		GetNumberOfMappingPairs(PCHAR pMappingPairs, DWORD * pError);
	
	DWORD		AppendRunsToArray(PATTRIBUTE_RECORD_HEADER pAttribute);
	DWORD		DeleteMapArray(VOID);
	DWORD		GrowMapArray(LONG NumberOfRuns);
	PCHAR		GetRunInformation(PCHAR pMappingPairStart, MAPPING_PAIRS_ARRAY * pMpArrayEntry);
	LONGLONG	ExtractNegativeValue(PCHAR pMappingPair, CHAR SizeV);
	LONGLONG	ExtractPositiveValue(PCHAR pMappingPair, CHAR SizeV);
	LONGLONG 	GetLcnFromVcn(PATTRIBUTE_RECORD_HEADER pAttribute,VCN Vcn, PLONG pLength);	
private:
	BOOL _Initialized;
};