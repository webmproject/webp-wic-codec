#include <windows.h>
#include <strsafe.h>
#include <msi.h>

typedef void (WINAPI *GetNativeSystemInfoFunc)(__out  LPSYSTEM_INFO lpSystemInfo);

void Fail(DWORD errorCode) {
  LPWSTR fullMessage;
  LPWSTR errorMessage;
  LPWSTR format;

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER,
      NULL, errorCode, 0, (LPWSTR)&errorMessage, 0, NULL);

  LoadStringW(GetModuleHandleW(NULL), 129, (LPWSTR)&format, 0);
  FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY,
      format, 0, 0, (LPWSTR)&fullMessage, 0, (va_list*)&errorMessage);

  MessageBoxW(NULL, fullMessage, NULL, MB_OK|MB_ICONERROR);
  TerminateProcess(GetCurrentProcess(), 1);
}

WORD GetTrueProcessorArch() {
  SYSTEM_INFO systemInfo;
  HMODULE hKernel32 = LoadLibraryExW(L"kernel32.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hKernel32 == NULL)
    Fail(ERROR_MOD_NOT_FOUND);
  GetNativeSystemInfoFunc pGetNativeSystemInfo = (GetNativeSystemInfoFunc)GetProcAddress(hKernel32, "GetNativeSystemInfo");
  if (pGetNativeSystemInfo != NULL) {
    pGetNativeSystemInfo(&systemInfo);
  } else {
    GetSystemInfo(&systemInfo);
  }
  return systemInfo.wProcessorArchitecture;
}

void SaveAndExecuteMsi(LPVOID pMsiData, DWORD_PTR msiSize) {
  WCHAR path[MAX_PATH];
  WCHAR tmpDir[MAX_PATH];
  DWORD bytesWritten;
  DWORD error;
  BOOL b;

  if (!GetTempPathW(MAX_PATH, tmpDir))
    Fail(GetLastError());
  srand(GetTickCount());
  HANDLE hFile = INVALID_HANDLE_VALUE;
  do {
    StringCchPrintfW(path, MAX_PATH, L"%wswebp%04x%04x.msi", tmpDir, rand(), rand());
    hFile = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_NEW, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS)
      Fail(GetLastError());
  } while (hFile == INVALID_HANDLE_VALUE);
  if (!WriteFile(hFile, pMsiData, msiSize, &bytesWritten, NULL))
    Fail(GetLastError());
  if (bytesWritten != msiSize)
    Fail(ERROR_CANNOT_MAKE);
  CloseHandle(hFile);

  MsiSetInternalUI(INSTALLUILEVEL_FULL, NULL);
  error = MsiInstallProductW(path, L"");
  DeleteFileW(path);

  if (error != ERROR_SUCCESS && error != ERROR_INSTALL_USEREXIT)
    Fail(error);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  DWORD_PTR msiSize;
  LPVOID pMsiData;
  HGLOBAL hGlob;
  HRSRC hRsrc;

  hRsrc = FindResourceW(hInstance, MAKEINTRESOURCEW(1+GetTrueProcessorArch()), L"MSIFILE");
  if (hRsrc == NULL)
    Fail(ERROR_INSTALL_PLATFORM_UNSUPPORTED);
  hGlob = LoadResource(hInstance, hRsrc);
  if (hGlob == NULL)
    Fail(GetLastError());
  pMsiData = LockResource(hGlob);
  msiSize = SizeofResource(hInstance, hRsrc);

  SaveAndExecuteMsi(pMsiData, msiSize);
  
  FreeResource(hGlob);
}
