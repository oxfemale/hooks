/*
первая инструкция перехватываемой функции заменяется инструкцией прерывания INT 3. 
Далее процедура обработки необработанных исключений (unhandled exception handler) подменяет регистр EIP на адрес нашей функции-перехватчика.
*/

static DWORD_PTR m_dwFunction;

static LONG WINAPI MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
    if (pExceptionInfo->ContextRecord->Eip != m_dwFunction)
        return EXCEPTION_CONTINUE_SEARCH;

    // Continue execution from MyMessageBeep
    pExceptionInfo->ContextRecord->Eip = (DWORD_PTR)MyMessageBeep;
    return EXCEPTION_CONTINUE_EXECUTION;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{

    m_dwFunction = (DWORD_PTR)::GetProcAddress(::GetModuleHandle("USER32.dll"), "MessageBeep");
    BYTE nSavedByte = *(LPBYTE)m_dwFunction;
    LPTOP_LEVEL_EXCEPTION_FILTER pOldFilter = ::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

    const BYTE nInt3 = 0xCC;
    // Inject int 3
    HRESULT hr = WriteProtectedMemory(LPVOID(m_dwFunction), &nInt3, sizeof(const BYTE));
    if (SUCCEEDED(hr))
    {
        ::MessageBeep(m_uType);

        // Restore function
        hr = WriteProtectedMemory(LPVOID(m_dwFunction), &nSavedByte, sizeof(BYTE));
    }

    ::SetUnhandledExceptionFilter(pOldFilter);

    return 0;
}
