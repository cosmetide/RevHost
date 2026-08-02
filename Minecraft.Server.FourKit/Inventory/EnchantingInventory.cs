namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the inventory of an Enchanting Table.
/// Single slot at index 0.
/// </summary>
public class EnchantingInventory : Inventory
{
    internal EnchantingInventory(string title, int size, int entityId)
        : base(title, InventoryType.ENCHANTING, size, entityId)
    {
    }

    /// <summary>
    /// Get the item being enchanted.
    /// </summary>
    /// <returns>The item.</returns>
    public ItemStack? getEnchantItem() => getItem(0);

    /// <summary>
    /// Set the item being enchanted.
    /// </summary>
    /// <param name="item">The item to set.</param>
    public void setEnchantItem(ItemStack? item) => setItem(0, item);
}
