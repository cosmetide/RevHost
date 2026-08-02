namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Block;

/// <summary>
/// This event is fired when the player is leaving a bed.
/// </summary>
public class PlayerBedLeaveEvent : PlayerEvent
{
    private readonly Block _bed;

    internal PlayerBedLeaveEvent(Player player, Block bed) : base(player)
    {
        _bed = bed;
    }

    /// <summary>
    /// Returns the bed block involved in this event.
    /// </summary>
    /// <returns>the bed block involved in this event</returns>
    public Block getBed() => _bed;
}
