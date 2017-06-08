/*
Модификация таблиц импорта/экспорта

Весь API, доступный из какого-либо модуля, описан в так называемой таблице экспорта этого модуля. С другой стороны, список API, необходимый для нормальной работы опять-таки, любого модуля, находится в его таблице импорта.

Код вызова процедуры из другого модуля выглядит примерно так:

call        dword ptr [__imp__MessageBeep@4 (004404cc)]
И, если изменить значение по этому адресу, можно подменить оригинальную функцию своей. Для этого нам понадобится:

Отыскать таблицу импорта функций для нужного нам модуля
Отыскать там указатель на перехватываемую функцию
Снять с этого участка памяти атрибут ReadOnly
Записать указатель на нашу функцию
Вернуть защиту обратно
*/
#include "stdafx.h"
#include "MainDlg.h"


HRESULT WriteProtectedMemory(LPVOID pDest, LPCVOID pSrc, DWORD dwSize)
{
	// Make it writable
	DWORD dwOldProtect = 0;
	if (::VirtualProtect(pDest, dwSize, PAGE_READWRITE, &dwOldProtect))
	{
		::MoveMemory(pDest, pSrc, dwSize);

		// Restore protection
		::VirtualProtect(pDest, dwSize, dwOldProtect, &dwOldProtect);
		return S_OK;
	}

	return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT ApiHijackImports(
				HMODULE hModule,
				LPSTR szVictim,
				LPSTR szEntry,
				LPVOID pHijacker,
				LPVOID *ppOrig
				)
{
	// Check args
	if (::IsBadStringPtrA(szVictim, -1) ||
		(!IS_INTRESOURCE(szEntry) && ::IsBadStringPtrA(szEntry, -1)) ||
		::IsBadCodePtr(FARPROC(pHijacker)))
	{
		return E_INVALIDARG;
	}

	PIMAGE_DOS_HEADER pDosHeader = PIMAGE_DOS_HEADER(hModule);

	if (::IsBadReadPtr(pDosHeader, sizeof(IMAGE_DOS_HEADER)) ||
		IMAGE_DOS_SIGNATURE != pDosHeader->e_magic)
	{
		return E_INVALIDARG;
	}

	PIMAGE_NT_HEADERS pNTHeaders = 
		MakePtr(PIMAGE_NT_HEADERS, hModule, pDosHeader->e_lfanew);

	if (::IsBadReadPtr(pNTHeaders, sizeof(IMAGE_NT_HEADERS)) ||
		IMAGE_NT_SIGNATURE != pNTHeaders->Signature)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_UNEXPECTED;

	// Locate the victim
	IMAGE_DATA_DIRECTORY& impDir = 
		pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	PIMAGE_IMPORT_DESCRIPTOR pImpDesc = 
		MakePtr(PIMAGE_IMPORT_DESCRIPTOR, hModule, impDir.VirtualAddress),
			pEnd = pImpDesc + impDir.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1;

	while(pImpDesc < pEnd)
	{
		if (0 == ::lstrcmpiA(MakePtr(LPSTR, hModule, pImpDesc->Name), szVictim))
		{
			if (0 == pImpDesc->OriginalFirstThunk)
			{
				// no import names table
				return E_UNEXPECTED;
			}

			// Locate the entry
		    PIMAGE_THUNK_DATA pNamesTable =
				MakePtr(PIMAGE_THUNK_DATA, hModule, pImpDesc->OriginalFirstThunk);

			if (IS_INTRESOURCE(szEntry))
			{
				// By ordinal
				while(pNamesTable->u1.AddressOfData)
				{
					if (IMAGE_SNAP_BY_ORDINAL(pNamesTable->u1.Ordinal) &&
						WORD(szEntry) == IMAGE_ORDINAL(pNamesTable->u1.Ordinal))
					{
						hr = S_OK;
						break;
					}
					pNamesTable++;
				}
			}
			else
			{
				// By name
				while(pNamesTable->u1.AddressOfData)
				{
					if (!IMAGE_SNAP_BY_ORDINAL(pNamesTable->u1.Ordinal))
					{
						PIMAGE_IMPORT_BY_NAME pName = MakePtr(PIMAGE_IMPORT_BY_NAME,
											hModule, pNamesTable->u1.AddressOfData);

						if (0 == ::lstrcmpiA(LPSTR(pName->Name), szEntry))
						{
							hr = S_OK;
							break;
						}
					}
					pNamesTable++;
				}
			}

			if (SUCCEEDED(hr))
			{
				// Get address
				LPVOID *pProc = MakePtr(LPVOID *, pNamesTable,
							pImpDesc->FirstThunk - pImpDesc->OriginalFirstThunk);

				// Save original handler
				if (ppOrig)
					*ppOrig = *pProc;

				// write to write-protected memory
				return WriteProtectedMemory(pProc, &pHijacker, sizeof(LPVOID));
			}
			break;
		}
		pImpDesc++;
	}
	return hr;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	LPVOID pOldHandler = NULL;
	HRESULT hr = ApiHijackImports(::GetModuleHandle(NULL), "USER32.dll", "MessageBeep", MyMessageBeep, &pOldHandler);
	if (SUCCEEDED(hr))
	{
		// Our handler will be used instead of MessageBeep()
		::MessageBeep(m_uType);

		// Restore old handler
		ApiHijackImports(::GetModuleHandle(NULL), "USER32.dll", "MessageBeep", pOldHandler, NULL);
	}
	else
		MessageBox(_T("Failed to hijack ::MessageBeep() imports"), NULL, MB_OK | MB_ICONHAND);

	return 0;
}


