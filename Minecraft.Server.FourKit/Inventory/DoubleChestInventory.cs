namespace Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Represents the inventory of a Double Chest.
/// 54 slots total — left side is slots 0–26, right side is slots 27–53.
/// </summary>
public class DoubleChestInventory : Inventory
{
    private readonly Inventory _left;
    private readonly Inventory _right;

    internal DoubleChestInventory(string title, int size, int entityId)
        : base(title, InventoryType.CHEST, size, entityId)
    {
        _left = new InventorySlice("Left chest", this, 0, 27);
        _right = new InventorySlice("Right chest", this, 27, 27);
    }

    /// <summary>
    /// Get the left half of this double chest.
    /// </summary>
    /// <returns>The left side inventory.</returns>
    public Inventory getLeftSide() => _left;

    /// <summary>
    /// Get the right half of this double chest.
    /// </summary>
    /// <returns>The right side inventory.</returns>
    public Inventory getRightSide() => _right;

    // todo: get rid of this class
    private sealed class InventorySlice : Inventory
    {
        private readonly Inventory _parent;
        private readonly int _offset;

        internal InventorySlice(string name, Inventory parent, int offset, int size)
            : base(name, InventoryType.CHEST, size)
        {
            _parent = parent;
            _offset = offset;
        }

        public override ItemStack? getItem(int index)
        {
            if (index < 0 || index >= getSize()) return null;
            return _parent.getItem(_offset + index);
        }

        public override void setItem(int index, ItemStack? item)
        {
            if (index >= 0 && index < getSize())
                _parent.setItem(_offset + index, item);
        }
    }
}
