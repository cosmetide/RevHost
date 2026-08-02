namespace Minecraft.Server.FourKit.Event.Server;

using Minecraft.Server.FourKit.Plugin;

public abstract class PluginEvent : ServerEvent
{
    private readonly ServerPlugin _plugin;

    internal protected PluginEvent(ServerPlugin plugin) : base()
    {
        _plugin = plugin;
    }

    /// <summary>Returns the plugin involved in this event.</summary>
    public ServerPlugin getPlugin() => _plugin;
}
