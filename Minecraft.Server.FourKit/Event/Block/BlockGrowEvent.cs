namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a block grows naturally in the world.
///
/// <para>Examples:</para>
/// <list type="bullet">
///   <item><description>Wheat</description></item>
///   <item><description>Sugar Cane</description></item>
///   <item><description>Cactus</description></item>
///   <item><description>Watermelon</description></item>
///   <item><description>Pumpkin</description></item>
/// </list>
///
/// <para>If a Block Grow event is cancelled, the block will not grow.</para>
/// </summary>
public class BlockGrowEvent : BlockEvent, Cancellable
{
    private bool _cancel;
    private readonly BlockState _newState;

    internal BlockGrowEvent(Block block, BlockState newState) : base(block)
    {
        _cancel = false;
        _newState = newState;
    }

    /// <summary>
    /// Gets the state of the block where it will form or spread to.
    /// </summary>
    /// <returns>The block state for this events block.</returns>
    public BlockState getNewState() => _newState;

    /// <inheritdoc/>
    public bool isCancelled() => _cancel;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }
}
