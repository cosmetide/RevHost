namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the inventory of a Furnace.
/// Slot layout: 0 = smelting input, 1 = fuel, 2 = result.
/// </summary>
public class FurnaceInventory : Inventory
{
    internal FurnaceInventory(string title, int size, int entityId)
        : base(title, InventoryType.FURNACE, size, entityId)
    {
    }

    /// <summary>
    /// Get the current item in the result slot.
    /// </summary>
    /// <returns>The item.</returns>
    public ItemStack? getResult() => getItem(2);

    /// <summary>
    /// Get the current fuel.
    /// </summary>
    /// <returns>The item.</returns>
    public ItemStack? getFuel() => getItem(1);

    /// <summary>
    /// Get the item currently smelting.
    /// </summary>
    /// <returns>The item.</returns>
    public ItemStack? getSmelting() => getItem(0);

    /// <summary>
    /// Set the current fuel.
    /// </summary>
    /// <param name="stack">The item.</param>
    public void setFuel(ItemStack? stack) => setItem(1, stack);

    /// <summary>
    /// Set the current item in the result slot.
    /// </summary>
    /// <param name="stack">The item.</param>
    public void setResult(ItemStack? stack) => setItem(2, stack);

    /// <summary>
    /// Set the item currently smelting.
    /// </summary>
    /// <param name="stack">The item.</param>
    public void setSmelting(ItemStack? stack) => setItem(0, stack);
}
