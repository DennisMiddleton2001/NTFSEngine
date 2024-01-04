#include "StdAfx.h"
#include "rw_buffer.h"

RW_BUFFER::RW_BUFFER(VOID)
{
	_NumberOfBytes=0;
	_AllocatedLength=0;
	_pBuffer=NULL;
}

RW_BUFFER::RW_BUFFER(LONGLONG NumberOfBytes)
{
	_pBuffer=NULL;
	BUFF_CreateManagedBuffer(NumberOfBytes);
}

RW_BUFFER::~RW_BUFFER(VOID)
{
	BUFF_DeleteManagedBuffer();
}

DWORD RW_BUFFER::BUFF_CreateManagedBuffer(LONGLONG NumberOfBytes)
{
	

	if (_pBuffer)
	{
		BUFF_DeleteManagedBuffer();
	}

	if (NumberOfBytes==0)
	{
		return NO_ERROR;
	}

    _pBuffer = (PCHAR) VirtualAlloc(NULL, (size_t) NumberOfBytes, MEM_COMMIT, PAGE_READWRITE);
	
	if (_pBuffer)
	{
		RtlFillMemory(_pBuffer, (size_t) NumberOfBytes,0xaa);
		_NumberOfBytes   = NumberOfBytes;
		_AllocatedLength = NumberOfBytes;
		return ERROR_SUCCESS;
	}
	else
	{
		BUFF_DeleteManagedBuffer();
		return ERROR_NOT_ENOUGH_MEMORY;
	}

}

DWORD RW_BUFFER::BUFF_DeleteManagedBuffer(VOID)
{
	
	if (_pBuffer)
	{
		VirtualFree(_pBuffer, 0, MEM_RELEASE);
	}

	_pBuffer=NULL;
	_NumberOfBytes=0;
	_AllocatedLength=0;
	return ERROR_SUCCESS;
}

DWORD RW_BUFFER::BUFF_CloneBuffer(RW_BUFFER * pBufferToClone)
{
	DWORD Error = BUFF_CreateManagedBuffer(pBufferToClone->_AllocatedLength);

	if (!Error)
	{
        RtlCopyMemory(_pBuffer, pBufferToClone->_pBuffer, (size_t) _AllocatedLength);
	}

	return Error;

}

DWORD RW_BUFFER::BUFF_CopyBufferExt(RW_BUFFER * pSrc, LONGLONG SrcOffset, LONGLONG DstOffset, LONGLONG TotalLength)
{
	DWORD Error		= ERROR_SUCCESS;
	LONGLONG Remainder;
	LONGLONG  Length= (TotalLength == -1) ? pSrc->_NumberOfBytes : TotalLength;
	PCHAR pSrcMin	= pSrc->_pBuffer;
	PCHAR pSrcMax   = ADD_PTR(pSrc->_pBuffer, pSrc->_AllocatedLength - 1);
	PCHAR pSrcStart = pSrcMin + SrcOffset;
	PCHAR pDstMin	= _pBuffer;
	PCHAR pDstMax   = ADD_PTR(_pBuffer, _AllocatedLength - 1);

	if ((pSrcMin + SrcOffset + Length - 1) > pSrcMax)
	{
		Error = ERROR_INVALID_PARAMETER;
		return Error;
	}

	if ((pDstMin + DstOffset + Length - 1) > pDstMax)
	{
		// Grow the destination buffer if needed.
		RW_BUFFER pApp;
		Remainder = (LONGLONG) (pDstMin + DstOffset + Length - 1 - pDstMax);
		Length -= Remainder;
		pApp.BUFF_CreateManagedBuffer(Remainder);
		RtlCopyMemory(pApp._pBuffer, pSrcMax+1, (size_t) Remainder);
		Error = BUFF_Append(&pApp);
		if (Error)	
		{
			BUFF_DeleteManagedBuffer();
			return Error;
		}
	}

	RtlCopyMemory(pDstMin+DstOffset, pSrcMin+SrcOffset, (size_t) Length);
	return Error;
}


DWORD RW_BUFFER::BUFF_CopyStdBuffer(PVOID pBuffer, LONGLONG NumberOfBytes)
{
	DWORD Error=BUFF_CreateManagedBuffer(NumberOfBytes);

	if (!Error)
	{
        RtlCopyMemory(_pBuffer, pBuffer, (size_t) _AllocatedLength);
	}

	return Error;
}

BOOL RW_BUFFER::BUFF_IsAllZeroes()
{		
	for (LONGLONG i=0;i<_NumberOfBytes;i++)
	{
		if (_pBuffer[i]) return FALSE;
	}
	return TRUE;
}

VOID RW_BUFFER::BUFF_Zero()
{

	RtlZeroMemory(_pBuffer, (size_t) _AllocatedLength);

	return;
}

DWORD RW_BUFFER::BUFF_Append(RW_BUFFER * pBufferToAdd)
{
	CHAR* pThisBuffer			= _pBuffer;
	LONGLONG AllocatedLength	= _AllocatedLength;
	LONGLONG NumberOfBytes		= _NumberOfBytes;
	
	// This prevents the class from deleting the original when we allocate the new size.
	_NumberOfBytes = 0;
	_AllocatedLength =0;
	_pBuffer = NULL;

	if (pBufferToAdd->_pBuffer == NULL)
	{
		wprintf(L"Warning.  Appending nonzero length buffer with no contents.\n");
		return 0;
	}
	

	//Allocate memory and adjust valid data length.
	DWORD Error = BUFF_CreateManagedBuffer(AllocatedLength + pBufferToAdd->_AllocatedLength);
	_NumberOfBytes = NumberOfBytes + pBufferToAdd->_NumberOfBytes;
	if (Error) 
	{
		_pBuffer		 = pThisBuffer;
		_NumberOfBytes	 = NumberOfBytes;
		_AllocatedLength = AllocatedLength;
		return Error;
	}

	//Copy original allocated memory into the new buffer.
	if (AllocatedLength)
	{
		RtlCopyMemory(_pBuffer, 
					pThisBuffer, 
					(size_t) AllocatedLength);
	}

	//Append the new buffer memory to the new buffer.
	RtlCopyMemory(	ADD_PTR(_pBuffer,AllocatedLength), 
					pBufferToAdd->_pBuffer, 
					(size_t) pBufferToAdd->_NumberOfBytes);
	
	//Free the original buffer memory.
	VirtualFree(pThisBuffer, 0, MEM_RELEASE);
	
	return ERROR_SUCCESS;

}


VOID  RW_BUFFER::BUFF_FindCompressedLength()
{
	LONG NumberOfQuads	= (LONG) _AllocatedLength / sizeof (LONGLONG);
	PLONGLONG pQuad		= (PLONGLONG) ADD_PTR(_pBuffer, _AllocatedLength);
	
	pQuad--;

	while (pQuad > (PLONGLONG) _pBuffer)
	{
		if (*pQuad)
		{
			pQuad++;
			break;
		}
		pQuad--;
	}
	_NumberOfBytes = (LONGLONG) ((PCHAR) pQuad - _pBuffer);
}

DWORD RW_BUFFER::BUFF_Realign(LONGLONG NewStartOffset)
{
	DWORD Error;
	PVOID OldStartAddress		  = _pBuffer;
	LONGLONG OldNumberOfBytes     = _NumberOfBytes;
	LONGLONG OldAllocatedLength   = _AllocatedLength;
	LONGLONG BytesToCopy   = OldNumberOfBytes - NewStartOffset;

	if (BytesToCopy <0)
	{
		Error = ERROR_INVALID_PARAMETER;
		return Error;
	}

	_pBuffer           = NULL;
	_NumberOfBytes     = 0;
	_AllocatedLength   = 0;

	Error = BUFF_CopyStdBuffer((PVOID) ADD_PTR(OldStartAddress, NewStartOffset), OldNumberOfBytes - NewStartOffset);

	_NumberOfBytes =  OldNumberOfBytes - NewStartOffset;

	VirtualFree(OldStartAddress, 0, MEM_RELEASE);
	return Error;
}
