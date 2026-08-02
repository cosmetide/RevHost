namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a block is destroyed as a result of being burnt by fire.
///
/// <para>If a Block Burn event is cancelled, the block will not be destroyed
/// as a result of being burnt by fire.</para>
/// </summary>
public class BlockBurnEvent : BlockEvent, Cancellable
{
    private bool _cancel;

    internal BlockBurnEvent(Block block) : base(block)
    {
        _cancel = false;
    }


    /// <inheritdoc />
    public bool isCancelled() => _cancel;


    /// <inheritdoc />
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }
}
