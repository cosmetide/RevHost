#include "FourKitRuntime.h"
#include "ServerLogger.h"
#include "stdafx.h"

#include <string>
#include <vector>
#include <windows.h>

using ServerRuntime::LogError;

typedef void *hostfxr_handle;

typedef int(__cdecl *hostfxr_initialize_for_runtime_config_fn)(
    const wchar_t *runtime_config_path,
    const void *parameters,
    hostfxr_handle *host_context_handle);

// Self-contained component loading must use the command-line init API.
// hostfxr_initialize_for_runtime_config returns 0x80008093 ("Initialization
// for self-contained components is not supported") for self-contained
// publishes. We never call hostfxr_run_app, so the assembly's Main is
// never invoked; we just use this entry point to start the runtime and
// then ask for the load_assembly_and_get_function_pointer delegate.
typedef int(__cdecl *hostfxr_initialize_for_dotnet_command_line_fn)(
    int argc,
    const wchar_t **argv,
    const void *parameters,
    hostfxr_handle *host_context_handle);

enum hostfxr_delegate_type
{
    hdt_com_activation = 0,
    hdt_load_in_memory_assembly = 1,
    hdt_winrt_activation = 2,
    hdt_com_register = 3,
    hdt_com_unregister = 4,
    hdt_load_assembly_and_get_function_pointer = 5,
    hdt_get_function_pointer = 6,
};

typedef int(__cdecl *hostfxr_get_runtime_delegate_fn)(
    const hostfxr_handle host_context_handle,
    hostfxr_delegate_type type,
    void **delegate);

typedef int(__cdecl *hostfxr_close_fn)(const hostfxr_handle host_context_handle);

struct hostfxr_initialize_parameters
{
    size_t size;
    const wchar_t *host_path;
    const wchar_t *dotnet_root;
};

namespace
{
static hostfxr_initialize_for_runtime_config_fn s_initConfigFn = nullptr;
static hostfxr_initialize_for_dotnet_command_line_fn s_initCmdLineFn = nullptr;
static hostfxr_get_runtime_delegate_fn s_getDelegateFn = nullptr;
static hostfxr_close_fn s_closeFn = nullptr;
static std::wstring s_dotnetRoot;

static std::wstring FindNet10SystemRoot()
{
    std::vector<std::wstring> candidates;
    wchar_t envRoot[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariableW(L"DOTNET_ROOT", envRoot, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        candidates.push_back(std::wstring(envRoot));
    }
    candidates.push_back(L"C:\\Program Files\\dotnet");

    for (const auto &root : candidates)
    {
        std::wstring fxrDir = root + L"\\host\\fxr";
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((fxrDir + L"\\*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
        {
            continue;
        }
        bool has10 = false;
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != L'.')
            {
                if (std::wstring(fd.cFileName).substr(0, 3) == L"10.")
                {
                    has10 = true;
                }
            }
        } while (!has10 && FindNextFileW(h, &fd));
        FindClose(h);
        if (has10)
        {
            return root;
        }
    }

    return L"C:\\Program Files\\dotnet";
}

static bool TryLoadHostfxrFromPath(const std::wstring &path)
{
    HMODULE lib = LoadLibraryW(path.c_str());
    if (!lib)
    {
        return false;
    }

    s_initConfigFn = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(lib, "hostfxr_initialize_for_runtime_config");
    s_initCmdLineFn = (hostfxr_initialize_for_dotnet_command_line_fn)GetProcAddress(lib, "hostfxr_initialize_for_dotnet_command_line");
    s_getDelegateFn = (hostfxr_get_runtime_delegate_fn)GetProcAddress(lib, "hostfxr_get_runtime_delegate");
    s_closeFn = (hostfxr_close_fn)GetProcAddress(lib, "hostfxr_close");

    // We require the command-line init function for self-contained loading.
    // The runtime-config init function is optional (kept for diagnostics).
    if (s_initCmdLineFn && s_getDelegateFn && s_closeFn)
    {
        return true;
    }

    s_initConfigFn = nullptr;
    s_initCmdLineFn = nullptr;
    s_getDelegateFn = nullptr;
    s_closeFn = nullptr;
    FreeLibrary(lib);
    return false;
}

static bool LoadHostfxr()
{
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        exeDir = exeDir.substr(0, lastSlash);
    }

    // Preferred layout: self-contained publish output is staged in "<exeDir>\runtime\"
    // by the CMake POST_BUILD step so the server root stays clean. hostfxr.dll
    // discovers the rest of the runtime relative to its own location.
    if (TryLoadHostfxrFromPath(exeDir + L"\\runtime\\hostfxr.dll"))
    {
        s_dotnetRoot = exeDir + L"\\runtime";
        return true;
    }

    // Legacy fallback: hostfxr.dll dropped directly next to the exe.
    if (TryLoadHostfxrFromPath(exeDir + L"\\hostfxr.dll"))
    {
        s_dotnetRoot = FindNet10SystemRoot();
        return true;
    }

    wchar_t dotnetRoot[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariableW(L"DOTNET_ROOT", dotnetRoot, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        wcscpy_s(dotnetRoot, L"C:\\Program Files\\dotnet");
    }

    std::wstring hostfxrDir = std::wstring(dotnetRoot) + L"\\host\\fxr";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((hostfxrDir + L"\\*").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        std::wstring bestVersion;
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != L'.')
            {
                std::wstring ver(fd.cFileName);
                if (ver.substr(0, 3) == L"10." && ver > bestVersion)
                {
                    bestVersion = ver;
                }
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);

        if (!bestVersion.empty())
        {
            if (TryLoadHostfxrFromPath(hostfxrDir + L"\\" + bestVersion + L"\\hostfxr.dll"))
            {
                s_dotnetRoot = std::wstring(dotnetRoot);
                return true;
            }
        }
    }

    LogError("fourkit", "hostfxr.dll not found. Install the .NET 10 x64 runtime (https://aka.ms/dotnet/download) or copy hostfxr.dll from C:\\Program Files\\dotnet\\host\\fxr\\10.x.x\\ next to the server executable.");
    return false;
}
}

namespace FourKitBridge
{
    bool LoadManagedRuntime(const wchar_t *assemblyPath,
                            const wchar_t *hostPath,
                            load_assembly_fn *outLoadAssembly)
    {
        if (!LoadHostfxr())
        {
            return false;
        }

        hostfxr_initialize_parameters initParams = {};
        initParams.size = sizeof(hostfxr_initialize_parameters);
        initParams.host_path = hostPath;
        initParams.dotnet_root = s_dotnetRoot.c_str();

        // Use the dotnet_command_line API for self-contained component loading.
        // We pass the FourKit assembly path as argv[0]; hostfxr starts the
        // self-contained runtime that ships in the same directory. We never
        // call hostfxr_run_app, so the assembly's Main entry point is never
        // executed -- we only need the load_assembly_and_get_function_pointer
        // delegate to invoke individual [UnmanagedCallersOnly] methods.
        const wchar_t *argv[1] = { assemblyPath };

        hostfxr_handle ctx = nullptr;
        int rc = s_initCmdLineFn(1, argv, &initParams, &ctx);
        // hostfxr returns Success_HostAlreadyInitialized (0x00000001) when the
        // runtime is already up; both 0 and 1 are success for our purposes.
        if ((rc != 0 && rc != 1) || ctx == nullptr)
        {
            char msg[256];
            sprintf_s(msg, "hostfxr_initialize_for_dotnet_command_line failed (0x%08X). Check the FourKit assembly path.", rc);
            LogError("fourkit", msg);
            if (ctx)
            {
                s_closeFn(ctx);
            }
            return false;
        }

        load_assembly_fn loadAssembly = nullptr;
        rc = s_getDelegateFn(ctx, hdt_load_assembly_and_get_function_pointer, (void **)&loadAssembly);
        s_closeFn(ctx);

        if (rc != 0 || loadAssembly == nullptr)
        {
            LogError("fourkit", "Failed to get load_assembly_and_get_function_pointer delegate.");
            return false;
        }

        *outLoadAssembly = loadAssembly;
        return true;
    }

    bool GetManagedEntryPoint(load_assembly_fn loadAssembly,
                              const wchar_t *assemblyPath,
                              const wchar_t *typeName,
                              const wchar_t *methodName,
                              void **outFnPtr)
    {
        int rc = loadAssembly(
            assemblyPath,
            typeName,
            methodName,
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            outFnPtr);

        if (rc != 0 || *outFnPtr == nullptr)
        {
            char methodNarrow[256];
            sprintf_s(methodNarrow, "%S::%S", typeName, methodName);
            LogError("fourkit", (std::string("Failed to resolve managed entry point: ") + methodNarrow).c_str());
            return false;
        }
        return true;
    }
}
