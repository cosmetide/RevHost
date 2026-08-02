namespace Minecraft.Server.FourKit.Inventory;

public interface InventoryHolder
{
    /// <summary>
    /// Get the object's inventory.
    /// </summary>
    /// <returns>The inventory.</returns>
    Inventory getInventory();
}
