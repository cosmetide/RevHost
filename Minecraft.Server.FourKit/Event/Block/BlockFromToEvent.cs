namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents events with a source block and a destination block, currently
/// only applies to liquid (lava and water) and teleporting dragon eggs.
///
/// <para>If a Block From To event is cancelled, the block will not move
/// (the liquid will not flow).</para>
/// </summary>
public class BlockFromToEvent : BlockEvent, Cancellable
{
    private readonly Block _to;
    private readonly BlockFace _face;
    private bool _cancel;

    internal BlockFromToEvent(Block block, BlockFace face) : base(block)
    {
        _face = face;
        _to = block.getRelative(face);
        _cancel = false;
    }

    internal BlockFromToEvent(Block block, Block toBlock) : base(block)
    {
        _to = toBlock;
        _face = BlockFace.SELF;
        _cancel = false;
    }

    internal BlockFromToEvent(Block block, Block toBlock, BlockFace face) : base(block)
    {
        _to = toBlock;
        _face = face;
        _cancel = false;
    }

    /// <summary>
    /// Gets the BlockFace that the block is moving to.
    /// </summary>
    /// <returns>The BlockFace that the block is moving to.</returns>
    public BlockFace getFace() => _face;

    /// <summary>
    /// Convenience method for getting the faced Block.
    /// </summary>
    /// <returns>The faced Block.</returns>
    public Block getToBlock() => _to;

    /// <inheritdoc/>
    public bool isCancelled() => _cancel;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }
}
