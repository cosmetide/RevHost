namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Thrown when a player picks an item up from the ground.
/// If cancelled the item will not be picked up.
/// </summary>
public class PlayerPickupItemEvent : PlayerEvent, Cancellable
{
    private readonly Item _item;
    private readonly int _remaining;
    private bool _cancelled;
    internal PlayerPickupItemEvent(Player player, Item item, int remaining)
        : base(player)
    {
        _item = item;
        _remaining = remaining;
    }

    /// <summary>
    /// Gets the Item picked up by the player.
    /// </summary>
    /// <returns>The <see cref="Item"/> entity.</returns>
    public Item getItem() => _item;

    /// <summary>
    /// Gets the amount remaining on the ground, if any.
    /// </summary>
    /// <returns>Amount remaining on the ground.</returns>
    public int getRemaining() => _remaining;

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
