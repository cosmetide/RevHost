namespace Minecraft.Server.FourKit.Event.Inventory;

/// <summary>
/// An estimation of what the result will be.
/// </summary>
public enum InventoryAction
{
    /// <summary>Nothing will happen from the click.</summary>
    NOTHING,
    /// <summary>All of the items on the clicked slot are moved to the cursor.</summary>
    PICKUP_ALL,
    /// <summary>Some of the items on the clicked slot are moved to the cursor.</summary>
    PICKUP_SOME,
    /// <summary>Half of the items on the clicked slot are moved to the cursor.</summary>
    PICKUP_HALF,
    /// <summary>One of the items on the clicked slot are moved to the cursor.</summary>
    PICKUP_ONE,
    /// <summary>All of the items on the cursor are moved to the clicked slot.</summary>
    PLACE_ALL,
    /// <summary>Some of the items from the cursor are moved to the clicked slot (usually up to the max stack size).</summary>
    PLACE_SOME,
    /// <summary>A single item from the cursor is moved to the clicked slot.</summary>
    PLACE_ONE,
    /// <summary>The clicked item and the cursor are exchanged.</summary>
    SWAP_WITH_CURSOR,
    /// <summary>The entire cursor item is dropped.</summary>
    DROP_ALL_CURSOR,
    /// <summary>One item is dropped from the cursor.</summary>
    DROP_ONE_CURSOR,
    /// <summary>The entire clicked slot is dropped.</summary>
    DROP_ALL_SLOT,
    /// <summary>One item is dropped from the clicked slot.</summary>
    DROP_ONE_SLOT,
    /// <summary>The item is moved to the opposite inventory if a space is found.</summary>
    MOVE_TO_OTHER_INVENTORY,
    /// <summary>The clicked item is moved to the hotbar, and the item currently there is re-added to the player's inventory.</summary>
    HOTBAR_MOVE_AND_READD,
    /// <summary>The clicked slot and the picked hotbar slot are swapped.</summary>
    HOTBAR_SWAP,
    /// <summary>A max-size stack of the clicked item is put on the cursor.</summary>
    CLONE_STACK,
    /// <summary>The inventory is searched for the same material, and they are put on the cursor up to Material.getMaxStackSize().</summary>
    COLLECT_TO_CURSOR,
    /// <summary>An unrecognized ClickType.</summary>
    UNKNOWN,
}
