namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;
using Minecraft.Server.FourKit.Inventory;

/// <summary>
/// Called when a block is placed by a player.
/// </summary>
public class BlockPlaceEvent : BlockEvent, Cancellable
{
    protected Block placedAgainst;
    protected ItemStack itemInHand;
    protected Player player;
    protected bool canBuild;
    protected bool cancel;

    internal BlockPlaceEvent(Block placedBlock, Block placedAgainst, ItemStack itemInHand, Player thePlayer, bool canBuild)
        : base(placedBlock)
    {
        this.placedAgainst = placedAgainst;
        this.itemInHand = itemInHand;
        this.player = thePlayer;
        this.canBuild = canBuild;
        this.cancel = false;
    }

    /// <summary>
    /// Gets the player who placed the block involved in this event.
    /// </summary>
    /// <returns>The Player who placed the block involved in this event.</returns>
    public Player getPlayer() => player;

    /// <summary>
    /// Clarity method for getting the placed block. Not really needed except
    /// for reasons of clarity.
    /// </summary>
    /// <returns>The Block that was placed.</returns>
    public Block getBlockPlaced() => getBlock();

    /// <summary>
    /// Gets the block that this block was placed against.
    /// </summary>
    /// <returns>Block the block that the new block was placed against.</returns>
    public Block getBlockAgainst() => placedAgainst;

    /// <summary>
    /// Gets the item in the player's hand when they placed the block.
    /// </summary>
    /// <returns>The ItemStack for the item in the player's hand when they placed the block.</returns>
    public ItemStack getItemInHand() => itemInHand;

    /// <inheritdoc />
    public bool isCancelled() => cancel;

    /// <inheritdoc />
    public void setCancelled(bool cancel) => this.cancel = cancel;
}
