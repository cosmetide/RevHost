namespace Minecraft.Server.FourKit.Event.Server;

using Minecraft.Server.FourKit.Plugin;

public class PluginEnableEvent : PluginEvent
{

    internal PluginEnableEvent(ServerPlugin plugin) : base(plugin)
    {
    }
}
