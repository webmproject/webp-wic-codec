// Copyright 2010 Google Inc.
//
// This code is licensed under the same terms as WebM:
//  Software License Agreement:  http://www.webmproject.org/license/software/
//  Additional IP Rights Grant:  http://www.webmproject.org/license/additional/
// -----------------------------------------------------------------------------
//
// Implemention of debug logging for Debug builds, the class factory and main
// COM entry points.
//
// Author: Mikolaj Zalewski (mikolajz@google.com)


#define INITGUID

#include <windows.h>
#include "main.h"

#include <new>
#include <advpub.h>
#include <shlobj.h>
#include <unknwn.h>
#include "decode_container.h"
#include "utils.h"
#include "webpimg.h"

#include "uuid.h"

// Logging in debug mode.
#ifdef _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static CRITICAL_SECTION debug_file_section;
static FILE* debug_file;

void MAIN_debug_printf(const char* prefix, const char* func, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  EnterCriticalSection(&debug_file_section);
  fprintf(debug_file, "%s:%s ", prefix, func);
  fflush(debug_file);
  vfprintf(debug_file, fmt, ap);
  fflush(debug_file);
  LeaveCriticalSection(&debug_file_section);
  va_end(ap);
}

static void init_debug() {
  char filename[200];
  time_t timestamp;
  time(&timestamp);
  wsprintfA(filename, "D:\\temp\\codec-out-%08x-%08x.txt", (LONGLONG)timestamp,
      GetCurrentProcessId());
  debug_file = fopen(filename, "w");
  InitializeCriticalSection(&debug_file_section);
}

// Returns a pointer to a string representation of a GUID. The results are
// stored in 32 static buffer - the 33rd call to this methods will overwrite
// the first result.
char *debugstr_guid(REFGUID guid)
{
  static char guidbuf[32][128];
  static int pos = 0;
  pos %= 32;
  sprintf(guidbuf[pos], "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
	  guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
	  guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
	  guid.Data4[6], guid.Data4[7]);
  return guidbuf[pos++];
}

#endif

// Object and server locks counters
LONG volatile MAIN_nObjects = 0;
LONG volatile MAIN_nServerLocks = 0;
HINSTANCE MAIN_hSelf;

// Class factory

typedef HRESULT (*ObjectConstructor)(IUnknown** ppvObject);

// A default constructor. Creates and instance of T. T should be a subclass of
// IUnknown with a parameter-less constructor.
template<typename T>
HRESULT CreateComObject(IUnknown** output) {
  T* result = new (std::nothrow) T();
  if (result == NULL)
    return E_OUTOFMEMORY;
  *output = static_cast<IUnknown*>(result);
  return S_OK;
}

class MyClassFactory : public ComObjectBase<IClassFactory>
{
public:
  MyClassFactory(ObjectConstructor ctor);
  // IUnknown:
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
  ULONG STDMETHODCALLTYPE AddRef() { return ComObjectBase::AddRef(); }
  ULONG STDMETHODCALLTYPE Release() { return ComObjectBase::Release(); }
  // IClassFactory:
  HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject);
  HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

private:
  volatile LONG ref_count_;
  ObjectConstructor ctor_;
};

MyClassFactory::MyClassFactory(ObjectConstructor ctor) {
  InterlockedIncrement(&MAIN_nObjects);
  ref_count_ = 0;
  ctor_ = ctor;
}

HRESULT MyClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;

  if (!IsEqualGUID(riid, IID_IUnknown) && !IsEqualGUID(riid, IID_IClassFactory))
    return E_NOINTERFACE;
  this->AddRef();
  *ppvObject = static_cast<IClassFactory*>(this);
  return S_OK;
}

HRESULT MyClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject)
{
  IUnknown* output;
  HRESULT ret;
  TRACE3("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppvObject);

  if (ppvObject == NULL)
    return E_INVALIDARG;
  *ppvObject = NULL;

  if (pUnkOuter != NULL)
    return CLASS_E_NOAGGREGATION;

  ret = ctor_(&output);
  if (FAILED(ret))
    return ret;
  ret = output->QueryInterface(riid, ppvObject);
  output->Release();
  if (FAILED(ret))
    ppvObject = NULL;
  TRACE1("ret=%08x\n", ret);
  return ret;
}

HRESULT MyClassFactory::LockServer(BOOL fLock)
{
  if (fLock)
    InterlockedIncrement(&MAIN_nServerLocks);
  else
    InterlockedDecrement(&MAIN_nServerLocks);
  return S_OK;
}

typedef HRESULT (WINAPI *RegInstallFunc)(HMODULE hm, LPCWSTR pszSection, const STRTABLEW* pstTable);
typedef void (STDAPICALLTYPE *SHChangeNotifyFunc)(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);


static HRESULT RegisterServer(BOOL fInstall) {
  // Manual loading of advpack not to load it when DLL used in normal opertion.
  HMODULE hAdvPack = LoadLibraryExW(L"advpack.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hAdvPack == NULL)
    return E_UNEXPECTED;
  RegInstallFunc pRegInstall = (RegInstallFunc)GetProcAddress(hAdvPack, "RegInstallW");
  if (!pRegInstall)
    return E_UNEXPECTED;

  STRENTRY entries[1] = {
    {L"PhotoDir", L"Windows Photo Viewer"}
  };
  STRTABLE strings;
  strings.cEntries = sizeof(entries)/sizeof(entries[0]);
  strings.pse = entries;
  if (LOWORD(GetVersion()) == 0x0006)
    entries[0].pszValue = L"Windows Photo Gallery";

  LPCWSTR section;
  if (LOBYTE(GetVersion()) < 6) {
    section = (fInstall ? L"PrevistaInstall" : L"PrevistaUninstall");
  } else {
    section = (fInstall ? L"DefaultInstall" : L"DefaultUninstall");
  }
  TRACE3("Registering install=%d (using %ws) v=%x\n", fInstall, section, GetVersion());
  if (FAILED(pRegInstall(MAIN_hSelf, section, &strings)))
    return E_UNEXPECTED;

  // Invalidate caches.
  HMODULE hShell32 = LoadLibraryExW(L"shell32.dll", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  SHChangeNotifyFunc pSHChangeNotify = (SHChangeNotifyFunc)GetProcAddress(hShell32, "SHChangeNotify");
  if (pSHChangeNotify)
    pSHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  return S_OK;
}

STDAPI DllRegisterServer() {
  return RegisterServer(TRUE);
}

STDAPI DllUnregisterServer() {
  return RegisterServer(FALSE);
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppv)
{
  if (ppv == NULL)
    return E_INVALIDARG;
  *ppv=NULL;
  TRACE3("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), ppv);
  if (!IsEqualGUID(iid, IID_IClassFactory))
    return E_INVALIDARG;

  if (IsEqualGUID(clsid, CLSID_WebpWICDecoder)) {
    *ppv = (LPVOID)(new (std::nothrow) MyClassFactory(CreateComObject<DecodeContainer>));
  } else {
    return CLASS_E_CLASSNOTAVAILABLE;
  }

  if (*ppv == NULL)
    return E_OUTOFMEMORY;
  return S_OK;
}

STDAPI DllCanUnloadNow() {
  if (MAIN_nObjects == 0 && MAIN_nServerLocks == 0)
    return S_OK;
  else
    return S_FALSE;
}

BOOL WINAPI DllMain(__in  HINSTANCE hinstDLL, __in  DWORD fdwReason, __in  LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hinstDLL);
    MAIN_hSelf = hinstDLL;
    WebpInitTables();
#ifdef _DEBUG
    init_debug();
#endif
  }
  TRACE1("(%d)\n", fdwReason);
  return TRUE;
}
