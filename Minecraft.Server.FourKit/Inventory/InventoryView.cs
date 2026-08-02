namespace Minecraft.Server.FourKit.Inventory;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents a view linking two inventories and a single player
/// (whose inventory may or may not be one of the two).
/// </summary>
public class InventoryView
{
    /// <summary>
    /// Represents the raw slot ID for clicks outside the inventory window.
    /// </summary>
    public static readonly int OUTSIDE = -999;

    private readonly Inventory _topInventory;
    private readonly Inventory _bottomInventory;
    private readonly HumanEntity _player;
    private readonly InventoryType _type;

    /// <summary>
    /// Creates a new InventoryView linking two inventories and a player.
    /// </summary>
    /// <param name="topInventory">The upper inventory.</param>
    /// <param name="bottomInventory">The lower inventory.</param>
    /// <param name="player">The player viewing.</param>
    /// <param name="type">The inventory type.</param>
    public InventoryView(Inventory topInventory, Inventory bottomInventory, HumanEntity player, InventoryType type)
    {
        _topInventory = topInventory;
        _bottomInventory = bottomInventory;
        _player = player;
        _type = type;
    }

    /// <summary>
    /// Get the upper inventory involved in this transaction.
    /// </summary>
    /// <returns>The inventory.</returns>
    public virtual Inventory getTopInventory() => _topInventory;

    /// <summary>
    /// Get the lower inventory involved in this transaction.
    /// </summary>
    /// <returns>The inventory.</returns>
    public virtual Inventory getBottomInventory() => _bottomInventory;

    /// <summary>
    /// Get the player viewing.
    /// </summary>
    /// <returns>The player.</returns>
    public virtual HumanEntity getPlayer() => _player;

    /// <summary>
    /// Determine the type of inventory involved in the transaction. This indicates
    /// the window style being shown. It will never return PLAYER, since that is
    /// common to all windows.
    /// </summary>
    /// <returns>The inventory type.</returns>
    public virtual InventoryType getType() => _type;

    /// <summary>
    /// Sets one item in this inventory view by its raw slot ID.
    /// </summary>
    /// <remarks>
    /// If slot ID -999 is chosen, it may be expected that the item is dropped
    /// on the ground. This is not required behaviour, however.
    /// </remarks>
    /// <param name="slot">The raw slot ID.</param>
    /// <param name="item">The new item to put in the slot, or null to clear it.</param>
    public void setItem(int slot, ItemStack? item)
    {
        if (slot == OUTSIDE) return;
        int topSize = _topInventory.getSize();
        if (slot < topSize)
            _topInventory.setItem(slot, item);
        else
            _bottomInventory.setItem(slot - topSize, item);
    }

    /// <summary>
    /// Gets one item in this inventory view by its raw slot ID.
    /// </summary>
    /// <param name="slot">The raw slot ID.</param>
    /// <returns>The item currently in the slot.</returns>
    public ItemStack? getItem(int slot)
    {
        if (slot == OUTSIDE) return null;
        int topSize = _topInventory.getSize();
        if (slot < topSize)
            return _topInventory.getItem(slot);
        return _bottomInventory.getItem(slot - topSize);
    }

    /// <summary>
    /// Sets the item on the cursor of one of the viewing players.
    /// </summary>
    /// <param name="item">The item to put on the cursor, or null to remove it.</param>
    public void setCursor(ItemStack? item)
    {
        _player.setItemOnCursor(item);
    }

    /// <summary>
    /// Get the item on the cursor of one of the viewing players.
    /// </summary>
    /// <returns>The item on the player's cursor, or null if they aren't holding one.</returns>
    public ItemStack? getCursor()
    {
        return _player.getItemOnCursor();
    }

    /// <summary>
    /// Converts a raw slot ID into its local slot ID into whichever of the two
    /// inventories the slot points to.
    /// </summary>
    /// <param name="rawSlot">The raw slot ID.</param>
    /// <returns>The converted slot ID.</returns>
    public int convertSlot(int rawSlot)
    {
        int topSize = _topInventory.getSize();
        if (rawSlot < topSize)
            return rawSlot;
        return rawSlot - topSize;
    }

    /// <summary>
    /// Closes the inventory view.
    /// </summary>
    public void close()
    {
    }

    /// <summary>
    /// Check the total number of slots in this view, combining the upper
    /// and lower inventories.
    /// </summary>
    /// <returns>The total size.</returns>
    public int countSlots()
    {
        return _topInventory.getSize() + _bottomInventory.getSize();
    }

    /// <summary>
    /// Sets an extra property of this inventory if supported by that inventory,
    /// for example the state of a progress bar.
    /// </summary>
    /// <param name="prop">The window property to update.</param>
    /// <param name="value">The new value for the window property.</param>
    /// <returns>true if the property was updated successfully.</returns>
    public bool setProperty(Property prop, int value)
    {
        return false;
    }

    /// <summary>
    /// Get the title of this inventory window.
    /// </summary>
    /// <returns>The title.</returns>
    public string getTitle()
    {
        return _topInventory.getTitle();
    }

    /// <summary>
    /// Represents various extra properties of certain inventory windows.
    /// </summary>
    public enum Property
    {
        /// <summary>The progress of the down-pointing arrow in a brewing inventory.</summary>
        BREW_TIME,
        /// <summary>The progress of the flame in a furnace inventory.</summary>
        BURN_TIME,
        /// <summary>The progress of the right-pointing arrow in a furnace inventory.</summary>
        COOK_TIME,
        /// <summary>In an enchanting inventory, the top button's experience level value.</summary>
        ENCHANT_BUTTON1,
        /// <summary>In an enchanting inventory, the middle button's experience level value.</summary>
        ENCHANT_BUTTON2,
        /// <summary>In an enchanting inventory, the bottom button's experience level value.</summary>
        ENCHANT_BUTTON3,
        /// <summary>How many total ticks the current fuel should last.</summary>
        TICKS_FOR_CURRENT_FUEL
    }
}

/// <summary>
/// Extension methods for <see cref="InventoryView.Property"/>.
/// </summary>
public static class InventoryViewPropertyExtensions
{
    /// <summary>
    /// Gets the <see cref="InventoryType"/> that this property belongs to.
    /// </summary>
    /// <param name="prop">The property.</param>
    /// <returns>The inventory type.</returns>
    public static InventoryType getType(this InventoryView.Property prop) => prop switch
    {
        InventoryView.Property.BREW_TIME => InventoryType.BREWING,
        InventoryView.Property.BURN_TIME => InventoryType.FURNACE,
        InventoryView.Property.COOK_TIME => InventoryType.FURNACE,
        InventoryView.Property.ENCHANT_BUTTON1 => InventoryType.ENCHANTING,
        InventoryView.Property.ENCHANT_BUTTON2 => InventoryType.ENCHANTING,
        InventoryView.Property.ENCHANT_BUTTON3 => InventoryType.ENCHANTING,
        InventoryView.Property.TICKS_FOR_CURRENT_FUEL => InventoryType.FURNACE,
        _ => InventoryType.CHEST,
    };

    /// <summary>
    /// Gets the window-property id for this <see cref="InventoryView.Property"/>.
    /// </summary>
    /// <param name="prop">The property.</param>
    /// <returns>The id.</returns>
    public static int getId(this InventoryView.Property prop) => prop switch
    {
        InventoryView.Property.BREW_TIME => 0,
        InventoryView.Property.BURN_TIME => 0,
        InventoryView.Property.COOK_TIME => 2,
        InventoryView.Property.ENCHANT_BUTTON1 => 0,
        InventoryView.Property.ENCHANT_BUTTON2 => 1,
        InventoryView.Property.ENCHANT_BUTTON3 => 2,
        InventoryView.Property.TICKS_FOR_CURRENT_FUEL => 1,
        _ => -1,
    };
}
