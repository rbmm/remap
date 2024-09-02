#include "stdafx.h"

typedef NTSTATUS
(NTAPI * MapViewOfSection)(
						   _In_ HANDLE SectionHandle,
						   _In_ HANDLE ProcessHandle,
						   _Outptr_result_bytebuffer_(*ViewSize) PVOID *BaseAddress,
						   _In_ ULONG_PTR ZeroBits,
						   _In_ SIZE_T CommitSize,
						   _Inout_opt_ PLARGE_INTEGER SectionOffset,
						   _Inout_ PSIZE_T ViewSize,
						   _In_ SECTION_INHERIT InheritDisposition,
						   _In_ ULONG AllocationType,
						   _In_ ULONG Win32Protect
						   );

void RemapSelf_I(PVOID ImageBase, HANDLE hSection, MapViewOfSection Map)
{
	if (0 <= ZwUnmapViewOfSection(NtCurrentProcess(), ImageBase))
	{
		SIZE_T ViewSize = 0;
		if (0 > Map(hSection, NtCurrentProcess(), &ImageBase, 0, 0, 0, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE_READWRITE))
		{
			__debugbreak();
		}
	}
}

struct DTP {
	HANDLE hSection;
	union {
		PVOID pfn;
		void (*remap) (PVOID, HANDLE, MapViewOfSection);
	};
};

NTSTATUS NTAPI RemapSelf_T(DTP * params)
{
	params->remap(&__ImageBase, params->hSection, ZwMapViewOfSection);
	return 0;
}

void RemapSelf()
{
	if (PIMAGE_NT_HEADERS pinth = RtlImageNtHeader(&__ImageBase))
	{
		ULONG SizeOfImage = pinth->OptionalHeader.SizeOfImage;
		DTP params;
		LARGE_INTEGER size = { SizeOfImage };
		if (0 <= ZwCreateSection(&params.hSection, SECTION_ALL_ACCESS, 0, &size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, 0))
		{
			PVOID BaseAddress = 0;
			SIZE_T ViewSize = 0;
			if (0 <= ZwMapViewOfSection(params.hSection, NtCurrentProcess(), &BaseAddress, 
				0, 0, 0, &ViewSize, ViewUnmap, 0, PAGE_EXECUTE_READWRITE))
			{
				memcpy(BaseAddress, &__ImageBase, SizeOfImage);

				params.pfn = RtlOffsetToPointer(BaseAddress, RtlPointerToOffset(&__ImageBase, RemapSelf_I));

				if (IsDebuggerPresent())
				{
					HANDLE hThread;
					if (0 <= RtlCreateUserThread(NtCurrentProcess(), 0, TRUE, 0, 0x1000, 0x1000, 
						(PUSER_THREAD_START_ROUTINE)RemapSelf_T, &params, &hThread, 0))
					{
						ZwSetInformationThread(hThread, ThreadHideFromDebugger, 0, 0);
						if (0 > ZwResumeThread(hThread, 0) ||
							WAIT_OBJECT_0 != ZwWaitForSingleObject(hThread, FALSE, 0))
						{
							__debugbreak();
						}

						NtClose(hThread);
					}
				}
				else
				{
					RemapSelf_T(&params);
				}

				ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
			}

			NtClose(params.hSection);
		}
	}
}
