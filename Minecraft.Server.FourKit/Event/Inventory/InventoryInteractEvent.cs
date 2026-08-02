namespace Minecraft.Server.FourKit.Event.Inventory;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// An abstract base class for events that describe an interaction between a
/// <see cref="HumanEntity"/> and the contents of an <see cref="Inventory"/>.
/// This is currently not emitted anywhere, use <see cref="InventoryClickEvent"/> instead.
/// </summary>
public abstract class InventoryInteractEvent : InventoryEvent, Cancellable
{
    private bool _cancelled;
    internal protected InventoryInteractEvent(InventoryView transaction) : base(transaction)
    {
    }

    /// <summary>
    /// Gets the player who performed the click.
    /// </summary>
    /// <returns>The clicking player.</returns>
    public HumanEntity getWhoClicked()
    {
        return transaction.getPlayer();
    }


    /// <inheritdoc />
    public bool isCancelled() => _cancelled;


    /// <inheritdoc />
    public void setCancelled(bool cancel) => _cancelled = cancel;
}
