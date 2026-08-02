namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the inventory of a Beacon.
/// Single slot at index 0.
/// </summary>
public class BeaconInventory : Inventory
{
    internal BeaconInventory(string title, int size, int entityId)
        : base(title, InventoryType.BEACON, size, entityId)
    {
    }

    /// <summary>
    /// Get the item in the beacon's single slot.
    /// </summary>
    /// <returns>The item.</returns>
    public ItemStack? getBeaconItem() => getItem(0);

    /// <summary>
    /// Set the item in the beacon's single slot.
    /// </summary>
    /// <param name="item">The item to set.</param>
    public void setBeaconItem(ItemStack? item) => setItem(0, item);
}
