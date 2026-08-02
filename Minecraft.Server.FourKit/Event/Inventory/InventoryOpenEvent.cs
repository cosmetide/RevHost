namespace Minecraft.Server.FourKit.Event.Inventory;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Called when a player opens an inventory. Cancelling this event will prevent
/// the inventory screen from showing.
/// </summary>
public class InventoryOpenEvent : InventoryEvent, Cancellable
{
    private bool _cancelled;
    internal InventoryOpenEvent(InventoryView transaction) : base(transaction)
    {
    }

    /// <summary>
    /// Returns the player involved in this event.
    /// </summary>
    /// <returns>Player who is involved in this event.</returns>
    public HumanEntity getPlayer() => transaction.getPlayer();

    /// <summary>
    /// Gets the cancellation state of this event. A cancelled event will not
    /// be executed in the server, but will still pass to other plugins.
    /// If an inventory open event is cancelled, the inventory screen will not show.
    /// </summary>
    /// <returns>true if this event is cancelled.</returns>
    public bool isCancelled() => _cancelled;

    /// <summary>
    /// Sets the cancellation state of this event. A cancelled event will not
    /// be executed in the server, but will still pass to other plugins.
    /// If an inventory open event is cancelled, the inventory screen will not show.
    /// </summary>
    /// <param name="cancel">true if you wish to cancel this event.</param>
    public void setCancelled(bool cancel) => _cancelled = cancel;
}
