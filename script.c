#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <MinHook.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#define WINAPI
#define OutputDebugStringA(msg) fprintf(stderr, "%s\n", msg)
#endif

CRITICAL_SECTION g_crit_sect;
FILE* g_logFile = NULL;

#ifdef _WIN32
typedef HRESULT(WINAPI* LPDIRECTDRAWCREATEEX)(GUID*, LPVOID*, REFIID, IUnknown*);
LPDIRECTDRAWCREATEEX pRealDirectDrawCreateEx = NULL;
#else
typedef int (*DirectDrawCreateExFunc)(void*, void*, void*);
DirectDrawCreateExFunc pRealDirectDrawCreateEx = NULL;
#endif

void SetupLogging()
{
    // Open log file for writing
#ifdef _WIN32
    fopen_s(&g_logFile, "HookLog.txt", "w");
#else
    g_logFile = fopen("HookLog.txt", "w");
#endif

    if (g_logFile == NULL)
    {
        OutputDebugStringA("Failed to open log file");
#ifdef _WIN32
        MH_Uninitialize();
#endif
        exit(1);
    }
}

#ifdef _WIN32
HRESULT WINAPI HookedDirectDrawCreateEx(GUID* lpGuid, LPVOID* lplpDD, REFIID iid, IUnknown* pUnkOuter)
#else
int HookedDirectDrawCreateEx(void* lpGuid, void* lplpDD, void* iid, void* pUnkOuter)
#endif
{
    // Your custom logic before the original function is called
#ifdef _WIN32
    fprintf(g_logFile, "HookedDirectDrawCreateEx called\n");
#else
    fprintf(g_logFile, "HookedDirectDrawCreateEx called (Linux)\n");
#endif

    // Call the original function
#ifdef _WIN32
    HRESULT result = pRealDirectDrawCreateEx(lpGuid, lplpDD, iid, pUnkOuter);
#else
    int result = pRealDirectDrawCreateEx(lpGuid, lplpDD, iid, pUnkOuter);
#endif

    // Your custom logic after the original function is called
#ifdef _WIN32
    fprintf(g_logFile, "Original DirectDrawCreateEx called\n");
#else
    fprintf(g_logFile, "Original DirectDrawCreateEx called (Linux)\n");
#endif

    return result;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
#else
void __attribute__((constructor)) my_load(void)
#endif
{
    if (reason == DLL_PROCESS_ATTACH || reason == 1)  // 1 means dynamic library loaded on Linux
    {
        InitializeCriticalSection(&g_crit_sect);

        // Initialize MinHook (Windows only)
#ifdef _WIN32
        if (MH_Initialize() != MH_OK)
        {
            OutputDebugStringA("Failed to initialize MinHook");
            return FALSE;
        }
#endif

        // Get the address of the original DirectDrawCreateEx function
#ifdef _WIN32
        pRealDirectDrawCreateEx = (LPDIRECTDRAWCREATEEX)GetProcAddress(GetModuleHandle(L"ddraw.dll"), "DirectDrawCreateEx");

        if (pRealDirectDrawCreateEx == NULL)
        {
            OutputDebugStringA("Failed to get the address of DirectDrawCreateEx");
#ifdef _WIN32
            MH_Uninitialize();
#endif
            return FALSE;
        }
#else
        pRealDirectDrawCreateEx = (DirectDrawCreateExFunc)dlsym(RTLD_NEXT, "DirectDrawCreateEx");
#endif

        // Install the hook (Windows only)
#ifdef _WIN32
        if (MH_CreateHook((LPVOID)pRealDirectDrawCreateEx, &HookedDirectDrawCreateEx, reinterpret_cast<LPVOID*>(&pRealDirectDrawCreateEx)) != MH_OK)
        {
            OutputDebugStringA("Failed to create hook");
#ifdef _WIN32
            MH_Uninitialize();
#endif
            return FALSE;
        }

        // Enable the hook (Windows only)
        if (MH_EnableHook((LPVOID)pRealDirectDrawCreateEx) != MH_OK)
        {
            OutputDebugStringA("Failed to enable hook");
#ifdef _WIN32
            MH_Uninitialize();
#endif
            return FALSE;
        }
#endif

        // Setup logging
        SetupLogging();
    }
#ifdef _WIN32
    else if (reason == DLL_PROCESS_DETACH)
    {
        // Uninitialize MinHook (Windows only)
#ifdef _WIN32
        MH_Uninitialize();
#endif

        // Close log file
        if (g_logFile != NULL)
        {
            fclose(g_logFile);
        }
    }
#endif
}
