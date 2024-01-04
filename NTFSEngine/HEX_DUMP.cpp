#include "StdAfx.h"

#define BITS_PER_CHAR 8

HEX_DUMP::HEX_DUMP(VOID)
{

}

DWORD HEX_DUMP::OpenOutput(WCHAR * szFilename)
{
		Output = _wfopen(szFilename, L"w");
		return (Output==0) ? GetLastError() : NO_ERROR;
}

void HEX_DUMP::DirectOutput(FILE * OutputFileStream)
{
	Output = OutputFileStream;
}

DWORD HEX_DUMP::CloseOutput()
{
	FILE * fTmp = Output;
	Output = NULL;
	return fclose(fTmp);
}


HEX_DUMP::HEX_DUMP(RW_BUFFER * pBuffer, LONG Indent)
{	
	HEX_DumpHexData(pBuffer, Indent, 8);
}

HEX_DUMP::~HEX_DUMP(VOID)
{

}

DWORD HEX_DUMP::HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent)
{
	// Default is to print 8 bytes per line.
	return HEX_DumpHexData(pBuffer, Indent, 8, 0);
}

DWORD HEX_DUMP::HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine)
{
	return HEX_DumpHexData(pBuffer, Indent, BytesPerLine, 0);
}

DWORD HEX_DUMP::HEX_DumpHexData(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine, LONGLONG StartDisplayOffset)
{
	LONGLONG BytesRemaining;
	LONG Offset, b;
	WCHAR  szThisByte[255];
	WCHAR  szHex[255], szCharacters[255], szIndentation[255];
    PUCHAR pC;

    for (Offset=0;Offset<Indent;Offset++) 	*(szIndentation+Offset) = L' ';
	*(szIndentation+Indent) = L'\0';
	
	BytesRemaining=pBuffer->_NumberOfBytes;
	pC=(PUCHAR)pBuffer->_pBuffer;
	
	Offset=0;

	if (!BytesRemaining)
	{
		fwprintf(Output, L"%s\n", szIndentation);
		return ERROR_SUCCESS;
	}
    do	// Print a line
	{
		//Clear the strings for new line.
		szCharacters[0] =	L'\0';
		szHex[0] =			L'\0';

		//print the current byte offset without CR LF
		fwprintf(Output, L"%s0x%08I64x   ",szIndentation, (LONGLONG) Offset+StartDisplayOffset);

		// exit for loop if no more bytes.
		if (!BytesRemaining) break;

		for (b=0;b<BytesPerLine;b++)
		{
			if (BytesRemaining)
			{
				//place a byte the szHex output string.
				wsprintf(szThisByte,L"%02x ",*pC);
				
				wcscat(szHex,szThisByte);
				
				//place a byte in the ASCII output string
				switch (*pC)
				{
				case 0x0:
				case 0x1:
				case 0x2:
				case 0x3:
				case 0x4:
				case 0x5:
				case 0x6:
				case 0x7:
				case 0x8:
				case 0x9:
				case 0xa:
				case 0xb:
				case 0xc:
				case 0xd:
				case 0xe:
				case 0xf:
					wsprintf(szThisByte,L"%c",L'.');
					break;
				default:
					wsprintf(szThisByte,L"%c",*pC);
					break;
				}		
				
				wcscat(szCharacters,szThisByte);

				//Get ready for next byte
				pC++;
				BytesRemaining--;
				Offset++;
				
			}
			else
			{
				//place 3 empty spaces in output string.
				wcscat(szHex,L"   ");
			}
		}	//EOL
				
		//print the hex and ASCII strings
		fwprintf(Output, L"%s  %s\n", szHex, szCharacters);

	}	while (BytesRemaining);

	return ERROR_SUCCESS;
}


typedef struct _UC_BITS
{
	UCHAR  b0 : 1;
	UCHAR  b1 : 1;
	UCHAR  b2 : 1;
	UCHAR  b3 : 1;
	UCHAR  b4 : 1;
	UCHAR  b5 : 1;
	UCHAR  b6 : 1;
	UCHAR  b7 : 1;
} UC_BITS, * PUC_BITS;

DWORD HEX_DUMP::HEX_DumpHexDataAsBitmap(RW_BUFFER * pBuffer, LONG Indent, LONG BytesPerLine)
{
	return HEX_DumpHexDataAsBitmap(pBuffer, 0, Indent, BytesPerLine);
}

DWORD HEX_DUMP::HEX_DumpHexDataAsBitmap(RW_BUFFER * pBuffer, LONGLONG StartOffset, LONG Indent, LONG BytesPerLine)
{
	LONGLONG BytesRemaining;
	LONG Offset, b;
	WCHAR  szThisByte[16];
	WCHAR  szHex[80], szIndentation[80];
    PUC_BITS pC;
	
    for (Offset=0;Offset<Indent;Offset++) 	*(szIndentation+Offset) = L' ';
	*(szIndentation+Indent) = L'\0';
             
	BytesRemaining=pBuffer->_NumberOfBytes;
	pC = (PUC_BITS) pBuffer->_pBuffer;
	
	// Print a bitmap border.
	fwprintf(Output, L"%s", szIndentation);
	
	for (b=0;b < (BytesPerLine)-1; b++)
	{
		fwprintf(Output, L"--------+");
	}
	fwprintf(Output, L"--------\n");

	Offset=0;

	if (!BytesRemaining)
	{
		fwprintf(Output, L"%s\n", szIndentation);
		return ERROR_SUCCESS;
	}

    do	// Print a line
	{
		//Clear the strings for new line.
		szHex[0] =			L'\0';

		//print the current byte offset without CR LF
		fwprintf(Output, L"%s0x%I64x   ", szIndentation, (StartOffset+Offset)*BITS_PER_CHAR);
		
		// exit for loop if no more bytes.
		if (!BytesRemaining) break;

		for (b=0;b<BytesPerLine;b++)
		{
			if (BytesRemaining)
			{
				//place a single byte's worth of bits into the line.
				wsprintf(szThisByte, L"%C%C%C%C%C%C%C%C ",
						(pC->b0) ? '1' : '.',
						(pC->b1) ? '1' : '.',
						(pC->b2) ? '1' : '.',
						(pC->b3) ? '1' : '.',
						(pC->b4) ? '1' : '.',
						(pC->b5) ? '1' : '.',
						(pC->b6) ? '1' : '.',
						(pC->b7) ? '1' : '.');
				
				wcscat(szHex,szThisByte);
			
				//Get ready for next byte
				pC++;
				BytesRemaining--;
				Offset++;
			}
		}	//EOL
				
		//print the bitmap string
		fwprintf(Output, L"%s \n", szHex);

	}	while (BytesRemaining);

	// Print a bitmap border.
	fwprintf(Output, L"%s", szIndentation);
	
	for (b=0;b < (BytesPerLine)-1; b++)
	{
		fwprintf(Output, L"--------+");
	}
	fwprintf(Output, L"--------\n");

	return ERROR_SUCCESS;
}


