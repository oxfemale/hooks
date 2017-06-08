/*
Detours is a library for instrumenting arbitrary Win32 functions Windows-compatible processors. 
Detours intercepts Win32 functions by re-writing the in-memory code for target functions. 
The Detours package also contains utilities to attach arbitrary DLLs and data segments (called payloads) to any Win32 binary.
*/
#include <windows.h>
#include <detours.h>

#pragma comment( lib, "Ws2_32.lib" )
#pragma comment( lib, "detours.lib" )
#pragma comment( lib, "detoured.lib" )

int ( WINAPI *Real_Send )( SOCKET s, const char *buf, int len, int flags ) = send;
int ( WINAPI *Real_Recv )( SOCKET s, char *buf, int len, int flags ) = recv;  
int WINAPI Mine_Send( SOCKET s, const char* buf, int len, int flags );
int WINAPI Mine_Recv( SOCKET s, char *buf, int len, int flags );

int WINAPI Mine_Send( SOCKET s, const char *buf, int len, int flags ) {
    // .. do stuff ..

    return Real_Send( s, buf, len, flags );
}

int WINAPI Mine_Recv( SOCKET s, char *buf, int len, int flags ) {
    // .. do stuff ..

    return Real_Recv( s, buf, len, flags );
}

BOOL WINAPI DllMain( HINSTANCE, DWORD dwReason, LPVOID ) {
    switch ( dwReason ) {
        case DLL_PROCESS_ATTACH:       
            DetourTransactionBegin();
            DetourUpdateThread( GetCurrentThread() );
            DetourAttach( &(PVOID &)Real_Send, Mine_Send );
            DetourAttach( &(PVOID &)Real_Recv, Mine_Recv );
            DetourTransactionCommit();
            break;

        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread( GetCurrentThread() );
            DetourDetach( &(PVOID &)Real_Send, Mine_Send );
            DetourDetach( &(PVOID &)Real_Recv, Mine_Recv );
            DetourTransactionCommit(); 
        break;
    }

    return TRUE;
}
