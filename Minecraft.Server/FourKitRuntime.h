#pragma once

#define UNMANAGEDCALLERSONLY_METHOD ((const wchar_t *)-1)

namespace FourKitBridge
{
typedef int(__stdcall *load_assembly_fn)(
    const wchar_t *assembly_path,
    const wchar_t *type_name,
    const wchar_t *method_name,
    const wchar_t *delegate_type_name,
    void *reserved,
    void **delegate);

// Loads the .NET runtime via hostfxr's command-line init API and returns a
// delegate that can resolve individual [UnmanagedCallersOnly] entry points
// from the FourKit assembly. The command-line API is required because
// self-contained components are not supported by hostfxr_initialize_for_runtime_config.
bool LoadManagedRuntime(const wchar_t *assemblyPath,
                        const wchar_t *hostPath,
                        load_assembly_fn *outLoadAssembly);

bool GetManagedEntryPoint(load_assembly_fn loadAssembly,
                          const wchar_t *assemblyPath,
                          const wchar_t *typeName,
                          const wchar_t *methodName,
                          void **outFnPtr);
}
