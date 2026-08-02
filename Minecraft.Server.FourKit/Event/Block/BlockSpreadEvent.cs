namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a block spreads based on world conditions.
/// Use <see cref="BlockFormEvent"/> to catch blocks that "randomly" form
/// instead of actually spread.
///
/// <para>Examples:</para>
/// <list type="bullet">
///   <item><description>Mushrooms spreading.</description></item>
///   <item><description>Fire spreading.</description></item>
/// </list>
///
/// <para>If a Block Spread event is cancelled, the block will not spread.</para>
/// </summary>
public class BlockSpreadEvent : BlockFormEvent, Cancellable
{
    private readonly Block _source;

    internal BlockSpreadEvent(Block block, Block source, BlockState newState) : base(block, newState)
    {
        _source = source;
    }

    /// <summary>
    /// Gets the source block involved in this event.
    /// </summary>
    /// <returns>The Block for the source block involved in this event.</returns>
    public Block getSource() => _source;
}
