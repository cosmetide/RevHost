namespace Minecraft.Server.FourKit.Plugin;

/// <summary>
/// Base class that every plugin must extend.
/// <code>
/// public string name    => "MyPlugin";
/// public string version => "1.0.0";
/// public string author  => "Me";
///
/// public void onEnable()  { /* startup logic  */ }
/// public void onDisable() { /* shutdown logic */ }
/// </code>
/// </summary>
public abstract class ServerPlugin
{
    /// <summary>
    /// The name of this plugin. <b>Must be declared in your plugin class.</b>
    /// </summary>
    public virtual string name { get; } = string.Empty;

    /// <summary>
    /// The version of this plugin.
    /// </summary>
    public virtual string version { get; } = "1.0.0";

    /// <summary>
    /// The author of this plugin.
    /// </summary>
    public virtual string author { get; } = "Unknown";

    /// <summary>
    /// The server's root directory (where the server executable lives).
    /// Use this instead of <c>AppContext.BaseDirectory</c> which points
    /// to the .NET runtime subfolder. Set automatically before
    /// <see cref="onEnable"/> is called.
    /// </summary>
    public string serverDirectory { get; internal set; } = string.Empty;

    /// <summary>
    /// A per-plugin data directory (<c>plugins/&lt;PluginName&gt;/</c>).
    /// Created automatically if it does not exist. Use this for config
    /// files, logs, databases, or any plugin-specific storage.
    /// Set automatically before <see cref="onEnable"/> is called.
    /// </summary>
    public string dataDirectory { get; internal set; } = string.Empty;

    /// <summary>
    /// Called when this plugin is enabled
    /// </summary>
    public virtual void onEnable() { }

    /// <summary>
    /// Called when this plugin is disabled
    /// </summary>
    public virtual void onDisable() { }
}
