using Minecraft.Server.FourKit.Plugin;
using System.Reflection;
using System.Runtime.Loader;

namespace Minecraft.Server.FourKit;

internal sealed class PluginLoadContext : AssemblyLoadContext
{
    private readonly AssemblyDependencyResolver _resolver;
    private readonly string _pluginDirectory;
    private readonly string _pluginDependenciesDir;

    public PluginLoadContext(string pluginPath, string serverRoot)
        : base(isCollectible: false)
    {
        _pluginDirectory = Path.GetDirectoryName(Path.GetFullPath(pluginPath))!;
        _resolver = new AssemblyDependencyResolver(pluginPath);
        _pluginDependenciesDir = Path.Combine(serverRoot, "plugin-dependencies");
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        if (assemblyName.Name == typeof(ServerPlugin).Assembly.GetName().Name)
            return typeof(ServerPlugin).Assembly;

        string? path = _resolver.ResolveAssemblyToPath(assemblyName);
        if (path != null)
            return LoadFromAssemblyPath(path);

        if (assemblyName.Name != null)
        {
            string fallback = Path.Combine(_pluginDirectory, assemblyName.Name + ".dll");
            if (File.Exists(fallback))
                return LoadFromAssemblyPath(fallback);
        }

        return null;
    }

    protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
    {
        string? path = _resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
        if (path != null)
            return LoadUnmanagedDllFromPath(path);

        if (!string.IsNullOrEmpty(_pluginDependenciesDir) && Directory.Exists(_pluginDependenciesDir))
        {
            string candidate = Path.Combine(_pluginDependenciesDir, unmanagedDllName);
            if (File.Exists(candidate))
                return LoadUnmanagedDllFromPath(candidate);

            if (!unmanagedDllName.EndsWith(".dll", StringComparison.OrdinalIgnoreCase))
            {
                candidate = Path.Combine(_pluginDependenciesDir, unmanagedDllName + ".dll");
                if (File.Exists(candidate))
                    return LoadUnmanagedDllFromPath(candidate);
            }
        }

        string pluginDll = Path.Combine(_pluginDirectory, unmanagedDllName);
        if (File.Exists(pluginDll))
            return LoadUnmanagedDllFromPath(pluginDll);

        return IntPtr.Zero;
    }
}
