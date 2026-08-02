namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;
// Yo this event pissed me the fuck off

/// <summary>
/// Fired when a player drops an item from their inventory.
/// If cancelled, the item will not be dropped and the player keeps it.
/// The dropped item can be modified by plugins.
/// </summary>
public class PlayerDropItemEvent : PlayerEvent, Cancellable
{
    private ItemStack _itemDrop;
    private bool _cancelled;
    internal PlayerDropItemEvent(Player player, ItemStack drop)
        : base(player)
    {
        _itemDrop = drop;
    }

    /// <summary>
    /// Gets the ItemDrop created by the player.
    /// </summary>
    /// <returns>The ItemStack being dropped.</returns>
    public ItemStack getItemDrop() => _itemDrop;

    /// <summary>
    /// Sets the item to be dropped. Plugins can modify which item
    /// is actually dropped.
    /// </summary>
    /// <param name="item">The new item to drop.</param>
    public void setItemDrop(ItemStack item)
    {
        _itemDrop = item;
    }

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
