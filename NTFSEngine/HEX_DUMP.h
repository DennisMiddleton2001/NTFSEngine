#pragma once

class  HEX_DUMP
{
public:
	
	HEX_DUMP();
	HEX_DUMP(RW_BUFFER * pBuffer, LONG Indent);
	~HEX_DUMP();
	DWORD HEX_DUMP::OpenOutput(WCHAR * szFilename);
	void HEX_DUMP::DirectOutput(FILE * OutputFileStream);
	DWORD HEX_DUMP::CloseOutput();
	DWORD HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine, LONGLONG StartOffset);
	DWORD HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine);
	DWORD HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent);
	DWORD HEX_DumpHexDataAsBitmap(RW_BUFFER * pBuffer, LONGLONG thisOffset, LONG Indent, LONG BytesPerLine);
	DWORD HEX_DumpHexDataAsBitmap(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine);


	FILE * Output;
};
