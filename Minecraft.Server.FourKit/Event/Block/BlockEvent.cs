namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents a Block-related event.
/// </summary>
public abstract class BlockEvent : Event
{
    private readonly Block _block;

    internal protected BlockEvent(Block block)
    {
        _block = block;
    }

    /// <summary>
    /// Gets the block involved in this event.
    /// </summary>
    /// <returns>The Block which is involved in this event.</returns>
    public Block getBlock() => _block;
}
