namespace Minecraft.Server.FourKit.Event.World;

using Minecraft.Server.FourKit;

public class WorldEvent : Event
{
    internal World? _world;

    internal WorldEvent(World? world) : base()
    {
        _world = world;
    }
}
