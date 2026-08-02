namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the different types of inventories available.
/// </summary>
public enum InventoryType
{
    /// <summary>A chest inventory, with 0, 9, 18, 27, 36, 45, or 54 slots of type CONTAINER.</summary>
    CHEST,
    /// <summary>A dispenser inventory, with 9 slots of type CONTAINER.</summary>
    DISPENSER,
    /// <summary>A dropper inventory, with 9 slots of type CONTAINER.</summary>
    DROPPER,
    /// <summary>A furnace inventory, with a RESULT slot, a CRAFTING slot, and a FUEL slot.</summary>
    FURNACE,
    /// <summary>A workbench inventory, with 9 CRAFTING slots and a RESULT slot.</summary>
    WORKBENCH,
    /// <summary>A player's crafting inventory, with 4 CRAFTING slots and a RESULT slot.</summary>
    CRAFTING,
    /// <summary>An enchantment table inventory, with one CRAFTING slot and three enchanting buttons.</summary>
    ENCHANTING,
    /// <summary>A brewing stand inventory, with one FUEL slot and three CRAFTING slots.</summary>
    BREWING,
    /// <summary>A player's inventory, with 9 QUICKBAR slots, 27 CONTAINER slots, and 4 ARMOR slots.</summary>
    PLAYER,
    /// <summary>The creative mode inventory, with only 9 QUICKBAR slots and nothing else.</summary>
    CREATIVE,
    /// <summary>The merchant inventory, with 2 TRADE-IN slots, and 1 RESULT slot.</summary>
    MERCHANT,
    /// <summary>The ender chest inventory, with 27 slots.</summary>
    ENDER_CHEST,
    /// <summary>An anvil inventory, with 2 CRAFTING slots and 1 RESULT slot.</summary>
    ANVIL,
    /// <summary>A beacon inventory, with 1 CRAFTING slot.</summary>
    BEACON,
    /// <summary>A hopper inventory, with 5 slots of type CONTAINER.</summary>
    HOPPER
}

/// <summary>
/// Represents a slot type within an inventory.
/// </summary>
public enum SlotType
{
    /// <summary>A result slot in a furnace or crafting inventory.</summary>
    RESULT,
    /// <summary>A slot in the crafting matrix, or the input slot in a furnace inventory, the potion slot in the brewing stand, or the enchanting slot.</summary>
    CRAFTING,
    /// <summary>An armour slot in the player's inventory.</summary>
    ARMOR,
    /// <summary>A regular slot in the container or the player's inventory.</summary>
    CONTAINER,
    /// <summary>A slot in the bottom row or quickbar.</summary>
    QUICKBAR,
    /// <summary>A pseudo-slot representing the area outside the inventory window.</summary>
    OUTSIDE,
    /// <summary>The fuel slot in a furnace inventory, or the ingredient slot in a brewing stand inventory.</summary>
    FUEL,
}

/// <summary>
/// Provides default size and title information for each <see cref="InventoryType"/>.
/// </summary>
public static class InventoryTypeExtensions
{
    /// <summary>
    /// Gets the default size for this inventory type.
    /// </summary>
    /// <param name="type">The inventory type.</param>
    /// <returns>The default number of slots.</returns>
    public static int getDefaultSize(this InventoryType type) => type switch
    {
        InventoryType.CHEST => 27,
        InventoryType.DISPENSER => 9,
        InventoryType.DROPPER => 9,
        InventoryType.FURNACE => 3,
        InventoryType.WORKBENCH => 10,
        InventoryType.CRAFTING => 5,
        InventoryType.ENCHANTING => 1,
        InventoryType.BREWING => 4,
        InventoryType.PLAYER => 40,
        InventoryType.CREATIVE => 9,
        InventoryType.MERCHANT => 3,
        InventoryType.ENDER_CHEST => 27,
        InventoryType.ANVIL => 3,
        InventoryType.BEACON => 1,
        InventoryType.HOPPER => 5,
        _ => 0,
    };

    /// <summary>
    /// Gets the default title for this inventory type.
    /// </summary>
    /// <param name="type">The inventory type.</param>
    /// <returns>The default title string.</returns>
    public static string getDefaultTitle(this InventoryType type) => type switch
    {
        InventoryType.CHEST => "Chest",
        InventoryType.DISPENSER => "Dispenser",
        InventoryType.DROPPER => "Dropper",
        InventoryType.FURNACE => "Furnace",
        InventoryType.WORKBENCH => "Crafting",
        InventoryType.CRAFTING => "Crafting",
        InventoryType.ENCHANTING => "Enchant",
        InventoryType.BREWING => "Brewing",
        InventoryType.PLAYER => "Player",
        InventoryType.CREATIVE => "Creative",
        InventoryType.MERCHANT => "Trading",
        InventoryType.ENDER_CHEST => "Ender Chest",
        InventoryType.ANVIL => "Repairing",
        InventoryType.BEACON => "Beacon",
        InventoryType.HOPPER => "Item Hopper",
        _ => "Inventory",
    };
}
