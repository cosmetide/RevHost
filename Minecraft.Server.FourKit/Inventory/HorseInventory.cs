namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the inventory of a Horse.
/// Slot layout: 0 = saddle, 1 = armor, 2+ = chest slots.
/// </summary>
public class HorseInventory : Inventory
{
    internal HorseInventory(string title, int size, int entityId)
        : base(title, InventoryType.CHEST, size < 2 ? 2 : size, entityId)
    {
    }

    /// <summary>
    /// Gets the item in the horse's saddle slot.
    /// </summary>
    /// <returns>The saddle item.</returns>
    public ItemStack? getSaddle() => getItem(0);

    /// <summary>
    /// Sets the item in the horse's saddle slot.
    /// </summary>
    /// <param name="stack">The saddle item.</param>
    public void setSaddle(ItemStack? stack) => setItem(0, stack);

    /// <summary>
    /// Gets the item in the horse's armor slot.
    /// </summary>
    /// <returns>The armor item.</returns>
    public ItemStack? getArmor() => getItem(1);

    /// <summary>
    /// Sets the item in the horse's armor slot.
    /// </summary>
    /// <param name="stack">The armor item.</param>
    public void setArmor(ItemStack? stack) => setItem(1, stack);
}
