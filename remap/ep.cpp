#include "stdafx.h"

void RemapSelf();

VOID NTAPI OnApc(
	_In_ ULONG_PTR Parameter
)
{
	DbgPrint("%hs(%hs)\n", __FUNCTION__, Parameter);
}

LONG OnException(PEXCEPTION_POINTERS pep)
{
	switch (pep->ExceptionRecord->ExceptionCode)
	{
	case DBG_RIPEXCEPTION:
		return EXCEPTION_CONTINUE_EXECUTION;
	case STATUS_ACCESS_VIOLATION:
		return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

STDAPI DllUnregisterServer()
{
	OBJECT_ATTRIBUTES oa = { sizeof(oa), 0, 0, OBJ_CASE_INSENSITIVE };

	NTSTATUS status = STATUS_NO_MEMORY;

	if (oa.ObjectName = (PUNICODE_STRING)LocalAlloc(LMEM_FIXED, 0x10000))
	{
		SIZE_T s;
		if (0 <= (status = ZwQueryVirtualMemory(NtCurrentProcess(), &__ImageBase, 
			MemoryMappedFilenameInformation, oa.ObjectName, 0x10000  - sizeof(WCHAR), &s)))
		{
			*(PWSTR)RtlOffsetToPointer(oa.ObjectName->Buffer, oa.ObjectName->Length) = 0;
			MessageBoxW(0, oa.ObjectName->Buffer, L"RemapSelf", MB_ICONINFORMATION);
			if (IsDebuggerPresent()) __debugbreak();
			RemapSelf();
			status = ZwDeleteFile(&oa);
		}
		LocalFree(oa.ObjectName);
	}

	WCHAR sz[64];

	swprintf_s(sz, _countof(sz), L"DeleteFile=%x", status);
	MessageBoxW(0, sz, sz, 0 > status ? MB_ICONWARNING : MB_ICONINFORMATION);

	if (0 <= status)
	{
		// check exceptions
		__try 
		{
			RaiseException(DBG_RIPEXCEPTION, 0, 0, 0);
			DbgPrint("...\n");
			*(int*)0 = 0;
		}
		__except (OnException(exception_info())) 
		{
			DbgPrint("!! %x\r", exception_code());
		}

		// check CFG still work
		QueueUserAPC(OnApc, NtCurrentThread(), (ULONG_PTR)"Parameter");
		NtTestAlert();
	}
	
	return status;
}