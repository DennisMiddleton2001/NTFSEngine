#pragma once

class RW_BUFFER
{
public:
	PCHAR		_pBuffer;
	LONGLONG	_NumberOfBytes;		
	LONGLONG	_AllocatedLength;
	LONGLONG	_SourceLbn;

	RW_BUFFER(VOID);
	RW_BUFFER(LONGLONG NumberOfBytes);
	~RW_BUFFER(VOID);
	BOOL  BUFF_IsAllZeroes();
	DWORD BUFF_CreateManagedBuffer(LONGLONG NumberOfBytes);
	DWORD BUFF_DeleteManagedBuffer(VOID);
	VOID  BUFF_FindCompressedLength();
	VOID  BUFF_Zero();
	DWORD BUFF_Realign(LONGLONG NewStartOffset);
	DWORD BUFF_CloneBuffer(RW_BUFFER * pBufferToClone);
	DWORD BUFF_CopyBufferExt(RW_BUFFER * pSrc, LONGLONG SrcOffset, LONGLONG DstOffset, LONGLONG TotalLength);
	DWORD BUFF_CopyStdBuffer(PVOID pBuffer, LONGLONG NumberOfBytes);
	DWORD BUFF_Append(RW_BUFFER * pBufferToAdd);
};

#define PRW_BUFFER RW_BUFFER *

