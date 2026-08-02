using Minecraft.Server.FourKit.Event.Server;
using Minecraft.Server.FourKit.Plugin;
using System.Reflection;

namespace Minecraft.Server.FourKit;

internal sealed class FailedPluginInfo
{
    public string Name { get; }
    public string Error { get; }

    public FailedPluginInfo(string name, string error)
    {
        Name = name;
        Error = error;
    }
}

internal sealed class PluginLoader
{
    private static readonly BindingFlags DeclaredPublic =
        BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly;

    private readonly List<ServerPlugin> _plugins = new();
    private readonly List<FailedPluginInfo> _failedPlugins = new();
    private string _serverRoot = string.Empty;
    private string _pluginsDirectory = string.Empty;

    public IReadOnlyList<ServerPlugin> Plugins => _plugins.AsReadOnly();
    public IReadOnlyList<FailedPluginInfo> FailedPlugins => _failedPlugins.AsReadOnly();

    public void LoadPlugins(string pluginsDirectory, string serverRoot)
    {
        _serverRoot = serverRoot;
        _pluginsDirectory = pluginsDirectory;

        if (!Directory.Exists(pluginsDirectory))
        {
            ServerLog.Info("fourkit", $"Creating plugins directory: {pluginsDirectory}");
            Directory.CreateDirectory(pluginsDirectory);
            return;
        }

        var rootDlls = Directory.GetFiles(pluginsDirectory, "*.dll");
        if (rootDlls.Length > 0)
        {
            ServerLog.Info("fourkit", $"Found {rootDlls.Length} DLL(s) in plugins root.");
            foreach (var dll in rootDlls)
            {
                try { LoadPluginAssembly(dll); }
                catch (Exception ex)
                {
                    ServerLog.Error("fourkit", $"Failed to load {Path.GetFileName(dll)}: {ex.Message}");
                    _failedPlugins.Add(new FailedPluginInfo(Path.GetFileName(dll), ex.Message));
                }
            }
        }

        foreach (var subDir in Directory.GetDirectories(pluginsDirectory))
        {
            string folderName = Path.GetFileName(subDir);
            var allDlls = Directory.GetFiles(subDir, "*.dll");
            if (allDlls.Length == 0)
                continue;

            string? mainDll = allDlls.FirstOrDefault(f =>
                string.Equals(Path.GetFileNameWithoutExtension(f), folderName, StringComparison.OrdinalIgnoreCase));

            if (mainDll != null)
            {
                try
                { 
                    LoadPluginAssembly(mainDll);
                }
                catch (Exception ex) 
                { 
                    ServerLog.Error("fourkit", $"Failed to load {Path.GetFileName(mainDll)}: {ex.Message}");
                    FourKit.FireEvent(new PluginLoadFailedEvent(mainDll, ex.Message));
                    _failedPlugins.Add(new FailedPluginInfo(Path.GetFileName(mainDll), ex.Message));
                }
            }
            else
            {
                foreach (var dll in allDlls)
                {
                    try
                    {
                        LoadPluginAssembly(dll);
                    }
                    catch (Exception ex)
                    {
                        ServerLog.Error("fourkit", $"Failed to load {Path.GetFileName(dll)}: {ex.Message}");
                        FourKit.FireEvent(new PluginLoadFailedEvent(dll, ex.Message));
                        _failedPlugins.Add(new FailedPluginInfo(Path.GetFileName(dll), ex.Message));

                    }
                }
            }
        }
        FourKit.FireEvent(new PluginsLoadedEvent());
    }

    private void LoadPluginAssembly(string dllPath)
    {
        var context = new PluginLoadContext(dllPath, _serverRoot);
        var assembly = context.LoadFromAssemblyPath(Path.GetFullPath(dllPath));

        int found = 0;
        foreach (var type in assembly.GetTypes())
        {
            if (type.IsAbstract || type.IsInterface)
                continue;

            if (!typeof(ServerPlugin).IsAssignableFrom(type))
                continue;

            var plugin = (ServerPlugin?)Activator.CreateInstance(type);
            if (plugin == null)
                continue;

            _plugins.Add(plugin);
            found++;

            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);
            string pVersion = GetPluginString(plugin, "version", "getVersion", "GetVersion", "1.0");
            string pAuthor = GetPluginString(plugin, "author", "getAuthor", "GetAuthor", "Unknown");

            if (!HasDeclaredMember(type, "name"))
                ServerLog.Warn("fourkit", $"Plugin {type.Name} does not declare a 'name' property.");
            if (!HasDeclaredMember(type, "version"))
                ServerLog.Warn("fourkit", $"Plugin {type.Name} does not declare a 'version' property.");
            if (!HasDeclaredMember(type, "author"))
                ServerLog.Warn("fourkit", $"Plugin {type.Name} does not declare an 'author' property.");

            ServerLog.Info("fourkit", $"Loaded plugin: {pName} v{pVersion} by {pAuthor}");
        }

        if (found == 0)
        {
            ServerLog.Warn("fourkit", $"No ServerPlugin classes found in {Path.GetFileName(dllPath)}");
        }
    }

    public void EnableAll()
    {
        foreach (var plugin in _plugins)
        {
            EnablePlugin(plugin);
        }
    }

    public void DisableAll()
    {
        foreach (var plugin in _plugins)
        {
            DisablePlugin(plugin);
        }
    }

    public void EnablePlugin(ServerPlugin plugin)
    {
        try
        {
            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);

            plugin.serverDirectory = _serverRoot;
            string dataDir = Path.Combine(_pluginsDirectory, pName);
            if (!Directory.Exists(dataDir))
                Directory.CreateDirectory(dataDir);
            plugin.dataDirectory = dataDir;

            InvokePluginMethod(plugin, "onEnable", "OnEnable");
            ServerLog.Info("fourkit", $"Enabled: {pName}");

            FourKit.FireEvent(new PluginEnableEvent(plugin));
        }
        catch (Exception ex)
        {
            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);
            ServerLog.Error("fourkit", $"Error enabling {pName}: {ex.Message}");
            _failedPlugins.Add(new FailedPluginInfo(pName, ex.Message));
        }
    }

    public void DisablePlugin(ServerPlugin plugin)
    {
        try
        {
            InvokePluginMethod(plugin, "onDisable", "OnDisable");
            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);
            ServerLog.Info("fourkit", $"Disabled: {pName}");

            FourKit.FireEvent(new PluginDisableEvent(plugin));
        }
        catch (Exception ex)
        {
            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);
            ServerLog.Error("fourkit", $"Error disabling {pName}: {ex.Message}");
        }
    }

    public void BuildPluginList(System.Text.StringBuilder sb)
    {
        foreach (var plugin in _plugins)
        {
            string pName = GetPluginString(plugin, "name", "getName", "GetName", plugin.GetType().Name);
            string pVersion = GetPluginString(plugin, "version", "getVersion", "GetVersion", "1.0");
            string pAuthor = GetPluginString(plugin, "author", "getAuthor", "GetAuthor", "Unknown");
            sb.Append("loaded\t").Append(pName).Append('\t').Append(pVersion).Append('\t').Append(pAuthor).Append('\t').Append('\n');
        }

        foreach (var failed in _failedPlugins)
        {
            sb.Append("failed\t").Append(failed.Name).Append('\t').Append('\t').Append('\t').Append(failed.Error).Append('\n');
        }
    }

    private static void InvokePluginMethod(ServerPlugin plugin, string camelName, string pascalName)
    {
        Type type = plugin.GetType();

        MethodInfo? method = type.GetMethod(camelName, DeclaredPublic, Type.EmptyTypes)
                          ?? type.GetMethod(pascalName, DeclaredPublic, Type.EmptyTypes);

        if (method != null)
        {
            method.Invoke(plugin, null);
        }
    }

    private static bool HasDeclaredMember(Type type, string name)
    {
        return type.GetProperty(name, DeclaredPublic) != null
            || type.GetMethod("get" + char.ToUpper(name[0]) + name[1..], DeclaredPublic, Type.EmptyTypes) != null;
    }

    private static string GetPluginString(
        ServerPlugin plugin, string propertyName,
        string getterCamel, string getterPascal,
        string fallback)
    {
        Type type = plugin.GetType();

        PropertyInfo? prop = type.GetProperty(propertyName, DeclaredPublic);
        if (prop != null && prop.PropertyType == typeof(string))
        {
            return (string?)prop.GetValue(plugin) ?? fallback;
        }

        MethodInfo? getter = type.GetMethod(getterCamel, DeclaredPublic, Type.EmptyTypes);
        if (getter != null && getter.ReturnType == typeof(string))
        {
            return (string?)getter.Invoke(plugin, null) ?? fallback;
        }

        getter = type.GetMethod(getterPascal, DeclaredPublic, Type.EmptyTypes);
        if (getter != null && getter.ReturnType == typeof(string))
        {
            return (string?)getter.Invoke(plugin, null) ?? fallback;
        }

        return fallback;
    }
}
