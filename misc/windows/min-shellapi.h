#define SHELLAPI DECLSPEC_IMPORT

SHELLAPI HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR operation, LPCWSTR file, LPCWSTR parameters, LPCWSTR directory, INT showCmd);
SHELLAPI HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR operation,  LPCSTR file,  LPCSTR  parameters, LPCSTR  directory, INT showCmd);