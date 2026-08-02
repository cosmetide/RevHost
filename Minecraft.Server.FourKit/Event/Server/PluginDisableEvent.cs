namespace Minecraft.Server.FourKit.Event.Server;

using Minecraft.Server.FourKit.Plugin;

public class PluginDisableEvent : PluginEvent
{

    internal PluginDisableEvent(ServerPlugin plugin) : base(plugin)
    {
    }
}
