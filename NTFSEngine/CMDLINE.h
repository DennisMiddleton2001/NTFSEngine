#pragma once

class CMDLINE
{
public:
	
	CMDLINE(void);
	~CMDLINE(void);
	BOOL	_Initialized;
	INT     _argc;
	WCHAR ** _argv;

	VOID  CMDL_InitCommandLine       (INT argc, WCHAR ** argv);
	INT   CMDL_GetOptionIndex  (WCHAR * szOption);
	DWORD CMDL_GetOption32     (WCHAR * szOptionPrefix, INT IndexDelta, PLONG     pValue);
	DWORD CMDL_GetOption32     (WCHAR * szOptionPrefix, INT IndexDelta, PLONG     pValue, DWORD OptionFlags);

	DWORD CMDL_GetOption64     (WCHAR * szOptionPrefix, INT IndexDelta, PLONGLONG pValue);
	DWORD CMDL_GetOption64     (WCHAR * szOptionPrefix, INT IndexDelta, PLONGLONG pValue, DWORD OptionFlags);
	DWORD CMDL_GetOptionString (WCHAR * szOptionPrefix, INT IndexDelta, WCHAR **    szValue);

private:
	DWORD GetValue64(INT ValueIndex, PLONGLONG pValue, DWORD OptionFlags);
	DWORD GetValue32(INT ValueIndex, PLONG     pValue, DWORD OptionFlags);
	BOOL  IsIndexValid(INT Idx);
};

#define OPT_HEX 0x00000001
#define OPT_DEC 0x00000002