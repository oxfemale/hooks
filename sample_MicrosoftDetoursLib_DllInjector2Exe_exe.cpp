#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>

void EnableDebugPriv() {
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tkp;

    OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken );

    LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &luid );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges( hToken, false, &tkp, sizeof( tkp ), NULL, NULL );

    CloseHandle( hToken ); 
}

int main( int, char *[] ) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof( PROCESSENTRY32 );

    HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );

    if ( Process32First( snapshot, &entry ) == TRUE ) {
        while ( Process32Next( snapshot, &entry ) == TRUE ) {
            if ( stricmp( entry.szExeFile, "target.exe" ) == 0 ) {
                EnableDebugPriv();

                char dirPath[MAX_PATH];
                char fullPath[MAX_PATH];

                GetCurrentDirectory( MAX_PATH, dirPath );

                sprintf_s( fullPath, MAX_PATH, "%s\\DllToInject.dll", dirPath );

                HANDLE hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, entry.th32ProcessID );
                LPVOID libAddr = (LPVOID)GetProcAddress( GetModuleHandle( "kernel32.dll" ), "LoadLibraryA" );
                LPVOID llParam = (LPVOID)VirtualAllocEx( hProcess, NULL, strlen( fullPath ), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

                WriteProcessMemory( hProcess, llParam, fullPath, strlen( fullPath ), NULL );
                CreateRemoteThread( hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)libAddr, llParam, NULL, NULL );
                CloseHandle( hProcess );
            }
        }
    }

    CloseHandle( snapshot );

    return 0;
}
