namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

using FourKitBlock = Minecraft.Server.FourKit.Block.Block;

/// <summary>
/// Called when a player interacts with an object or air.
/// </summary>
public class PlayerInteractEvent : PlayerEvent, Cancellable
{
    private readonly Action _action;
    private readonly ItemStack? _item;
    private readonly FourKitBlock? _clickedBlock;
    private readonly BlockFace _clickedFace;
    private bool _cancelled;
    private bool _useItemInHand = true;

    internal PlayerInteractEvent(Player who, Action action, ItemStack? item, FourKitBlock? clickedBlock, BlockFace clickedFace)
        : base(who)
    {
        _action = action;
        _item = item;
        _clickedBlock = clickedBlock;
        _clickedFace = clickedFace;
    }

    /// <summary>
    /// Returns the action type.
    /// </summary>
    /// <returns>Action returns the type of interaction.</returns>
    public Action getAction() => _action;

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <summary>
    /// Sets the cancellation state of this event. A canceled event will not be
    /// executed in the server, but will still pass to other plugins.
    ///
    /// Canceling this event will prevent use of food (player won't lose the
    /// food item), prevent bows/snowballs/eggs from firing, etc. (player won't
    /// lose the ammo).
    /// </summary>
    /// <param name="cancel">true if you wish to cancel this event.</param>
    public void setCancelled(bool cancel) => _cancelled = cancel;

    /// <summary>
    /// Returns the item in hand represented by this event.
    /// </summary>
    /// <returns>ItemStack the item used.</returns>
    public ItemStack? getItem() => _item;

    /// <summary>
    /// Convenience method. Returns the material of the item represented by this event.
    /// </summary>
    /// <returns>Material the material of the item used.</returns>
    public Material getMaterial() => _item?.getType() ?? Material.AIR;

    /// <summary>
    /// Check if this event involved a block.
    /// </summary>
    /// <returns>true if it did.</returns>
    public bool hasBlock() => _clickedBlock != null;

    /// <summary>
    /// Check if this event involved an item.
    /// </summary>
    /// <returns>true if it did.</returns>
    public bool hasItem() => _item != null;

    /// <summary>
    /// Convenience method to inform the user whether this was a block placement event.
    /// </summary>
    /// <returns>true if the item in hand was a block.</returns>
    public bool isBlockInHand()
    {
        if (_item == null) return false;
        int id = (int)_item.getType();
        return id >= 1 && id <= 255;
    }

    /// <summary>
    /// Returns the clicked block.
    /// </summary>
    /// <returns>Block returns the block clicked with this item.</returns>
    public FourKitBlock? getClickedBlock() => _clickedBlock;

    /// <summary>
    /// Returns the face of the block that was clicked.
    /// </summary>
    /// <returns>BlockFace returns the face of the block that was clicked.</returns>
    public BlockFace getBlockFace() => _clickedFace;

    /// <summary>
    /// This controls the action to take with the item the player is holding.
    /// This includes both blocks and items (such as flint and steel or records).
    /// When this is set to default, it will be allowed if no action is taken on
    /// the interacted block.
    /// </summary>
    /// <returns>the action to take with the item in hand.</returns>
    public bool useItemInHand() => _useItemInHand;

    /// <summary>
    /// Sets whether to use the item in hand.
    /// </summary>
    /// <param name="useItemInHand">the action to take with the item in hand.</param>
    public void setUseItemInHand(bool useItemInHand) => _useItemInHand = useItemInHand;
}
