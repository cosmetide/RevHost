namespace Minecraft.Server.FourKit.Event.Inventory;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents a player related inventory event.
/// </summary>
public class InventoryEvent : Event
{
    /// <summary>The inventory view associated with this event.</summary>
    protected readonly InventoryView transaction;
    internal InventoryEvent(InventoryView transaction)
    {
        this.transaction = transaction;
    }

    /// <summary>
    /// Gets the primary Inventory involved in this transaction.
    /// </summary>
    /// <returns>The upper inventory.</returns>
    public global::Minecraft.Server.FourKit.Inventory.Inventory getInventory()
    {
        return transaction.getTopInventory();
    }

    /// <summary>
    /// Gets the list of players viewing the primary (upper) inventory
    /// involved in this event.
    /// </summary>
    /// <returns>A list of people viewing.</returns>
    public List<HumanEntity> getViewers()
    {
        return transaction.getTopInventory().getViewers();
    }

    /// <summary>
    /// Gets the view object itself.
    /// </summary>
    /// <returns>The InventoryView.</returns>
    public InventoryView getView()
    {
        return transaction;
    }
}
