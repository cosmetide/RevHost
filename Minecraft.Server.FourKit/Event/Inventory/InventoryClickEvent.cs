namespace Minecraft.Server.FourKit.Event.Inventory;

using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// This event is called when a player clicks a slot in an inventory.
/// <para>
/// Because InventoryClickEvent occurs within a modification of the Inventory,
/// not all Inventory related methods are safe to use.
/// </para>
/// <para>
/// The following should never be invoked by an EventHandler for
/// InventoryClickEvent using the HumanEntity or InventoryView associated
/// with this event:
/// <list type="bullet">
///   <item><description><see cref="HumanEntity.closeInventory()"/></description></item>
///   <item><description><see cref="HumanEntity.openInventory(Inventory)"/></description></item>
///   <item><description><see cref="InventoryView.close()"/></description></item>
/// </list>
/// </para>
/// </summary>
public class InventoryClickEvent : InventoryInteractEvent
{
    private readonly SlotType _slotType;
    private readonly int _rawSlot;
    private readonly int _whichSlot;
    private readonly ClickType _click;
    private readonly InventoryAction _action;
    private readonly int _hotbarKey;
    private ItemStack? _currentItem;
    internal InventoryClickEvent(InventoryView view, SlotType type, int slot,
        ClickType click, InventoryAction action)
        : this(view, type, slot, click, action, -1)
    {
    }
    internal InventoryClickEvent(InventoryView view, SlotType type, int slot,
        ClickType click, InventoryAction action, int key)
        : base(view)
    {
        _slotType = type;
        _rawSlot = slot;
        _click = click;
        _action = action;
        _hotbarKey = key;
        _currentItem = view.getItem(slot);
        _whichSlot = view.convertSlot(slot);
    }

    /// <summary>
    /// Gets the inventory that was clicked, or null if outside of window.
    /// </summary>
    /// <returns>The clicked inventory.</returns>
    public Inventory? getClickedInventory()
    {
        if (_rawSlot == InventoryView.OUTSIDE)
            return null;
        int topSize = getView().getTopInventory().getSize();
        if (_rawSlot < topSize)
            return getView().getTopInventory();
        return getView().getBottomInventory();
    }

    /// <summary>
    /// Gets the type of slot that was clicked.
    /// </summary>
    /// <returns>The slot type.</returns>
    public SlotType getSlotType() => _slotType;

    /// <summary>
    /// Gets the current ItemStack on the cursor.
    /// </summary>
    /// <returns>The cursor ItemStack.</returns>
    public ItemStack? getCursor() => getView().getCursor();

    /// <summary>
    /// Gets the ItemStack currently in the clicked slot.
    /// </summary>
    /// <returns>The item in the clicked slot.</returns>
    public ItemStack? getCurrentItem() => _currentItem;

    /// <summary>
    /// Gets whether or not the ClickType for this event represents a right click.
    /// </summary>
    /// <returns>true if the ClickType uses the right mouse button.</returns>
    public bool isRightClick() => _click.isRightClick();

    /// <summary>
    /// Gets whether or not the ClickType for this event represents a left click.
    /// </summary>
    /// <returns>true if the ClickType uses the left mouse button.</returns>
    public bool isLeftClick() => _click.isLeftClick();

    /// <summary>
    /// Gets whether the ClickType for this event indicates that the key was
    /// pressed down when the click was made.
    /// </summary>
    /// <returns>true if the ClickType uses Shift or Ctrl.</returns>
    public bool isShiftClick() => _click.isShiftClick();

    /// <summary>
    /// Sets the item on the cursor.
    /// </summary>
    /// <param name="stack">The new cursor item.</param>
    [Obsolete("This changes the ItemStack in their hand before any calculations are applied to the Inventory.")]
    public void setCursor(ItemStack? stack) => getView().setCursor(stack);

    /// <summary>
    /// Sets the ItemStack currently in the clicked slot.
    /// </summary>
    /// <param name="stack">The item to be placed in the current slot.</param>
    public void setCurrentItem(ItemStack? stack)
    {
        _currentItem = stack;
        if (_rawSlot >= 0)
            getView().setItem(_rawSlot, stack);
    }

    /// <summary>
    /// The slot number that was clicked, ready for passing to
    /// <see cref="Inventory.getItem(int)"/>. Note that there may be two slots
    /// with the same slot number, since a view links two different inventories.
    /// </summary>
    /// <returns>The slot number.</returns>
    public int getSlot() => _whichSlot;

    /// <summary>
    /// The raw slot number clicked, ready for passing to
    /// <see cref="InventoryView.getItem(int)"/>. This slot number is unique
    /// for the view.
    /// </summary>
    /// <returns>The raw slot number.</returns>
    public int getRawSlot() => _rawSlot;

    /// <summary>
    /// If the ClickType is NUMBER_KEY, this method will return the index of
    /// the pressed key (0-8).
    /// </summary>
    /// <returns>The number on the key minus 1 (range 0-8); or -1 if not a NUMBER_KEY action.</returns>
    public int getHotbarButton() => _hotbarKey;

    /// <summary>
    /// Gets the InventoryAction that triggered this event.
    /// This action cannot be changed, and represents what the normal outcome
    /// of the event will be. To change the behavior of this InventoryClickEvent,
    /// changes must be manually applied.
    /// </summary>
    /// <returns>The InventoryAction that triggered this event.</returns>
    public InventoryAction getAction() => _action;

    /// <summary>
    /// Gets the ClickType for this event.
    /// This is insulated against changes to the inventory by other plugins.
    /// </summary>
    /// <returns>The type of inventory click.</returns>
    public ClickType getClick() => _click;
}
