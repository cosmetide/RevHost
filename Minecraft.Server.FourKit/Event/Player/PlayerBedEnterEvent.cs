namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Block;

/// <summary>
/// This event is fired when the player is almost about to enter the bed.
/// </summary>
public class PlayerBedEnterEvent : PlayerEvent, Cancellable
{
    private readonly Block _bed;
    private bool _cancelled;

    internal PlayerBedEnterEvent(Player player, Block bed) : base(player)
    {
        _bed = bed;
    }

    /// <summary>
    /// Returns the bed block involved in this event.
    /// </summary>
    /// <returns>the bed block involved in this event</returns>
    public Block getBed() => _bed;

    /// <inheritdoc />
    public bool isCancelled() => _cancelled;

    /// <inheritdoc />
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
