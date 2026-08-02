namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Fired when a player moves. Plugins may modify the destination
/// or cancel the movement entirely.
/// </summary>
public class PlayerMoveEvent : PlayerEvent, Cancellable
{
    private Location _from;
    private Location _to;
    private bool _cancelled;

    internal PlayerMoveEvent(Player player, Location from, Location to) : base(player)
    {
        _from = from;
        _to = to;
    }

    /// <summary>
    /// Gets the location this player moved from.
    /// </summary>
    /// <returns>The from location.</returns>
    public Location getFrom() => _from;

    /// <summary>
    /// Gets the location this player moved to.
    /// </summary>
    /// <returns>The to location.</returns>
    public Location getTo() => _to;

    /// <summary>
    /// Sets the location to mark as where the player moved from.
    /// </summary>
    /// <param name="from">The new from location.</param>
    public void setFrom(Location from)
    {
        _from = from;
    }

    /// <summary>
    /// Sets the location that this player will move to.
    /// </summary>
    /// <param name="to">The new to location.</param>
    public void setTo(Location to)
    {
        _to = to;
    }

    /// <inheritdoc />
    public bool isCancelled() => _cancelled;

    /// <inheritdoc />
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
