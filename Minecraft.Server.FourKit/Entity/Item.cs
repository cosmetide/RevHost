namespace Minecraft.Server.FourKit.Entity;

using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents a dropped item on the ground.
/// </summary>
public class Item : Entity
{
    private ItemStack _itemStack;

    internal Item(int entityId, int dimId, double x, double y, double z, ItemStack itemStack)
    {
        SetEntityIdInternal(entityId);
        SetEntityTypeInternal(EntityType.DROPPED_ITEM);
        SetDimensionInternal(dimId);
        SetLocation(new Location(FourKit.getWorld(dimId), x, y, z));
        _itemStack = itemStack;
    }

    /// <summary>
    /// Gets the item stack associated with this item.
    /// </summary>
    /// <returns>An item stack.</returns>
    public ItemStack getItemStack() => _itemStack;

    /// <summary>
    /// Sets the item stack of this item.
    /// </summary>
    /// <param name="stack">The new item stack.</param>
    public void setItemStack(ItemStack stack) => _itemStack = stack;
}
