namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a piston block is triggered.
/// </summary>
public abstract class BlockPistonEvent : BlockEvent, Cancellable
{
    private bool _cancel;
    private readonly BlockFace _direction;

    internal protected BlockPistonEvent(Block block, BlockFace direction) : base(block)
    {
        _direction = direction;
        _cancel = false;
    }


    /// <inheritdoc />
    public bool isCancelled() => _cancel;


    /// <inheritdoc />
    public void setCancelled(bool cancelled)
    {
        _cancel = cancelled;
    }

    /// <summary>
    /// Returns true if the Piston in the event is sticky.
    /// </summary>
    /// <returns>Stickiness of the piston.</returns>
    public bool isSticky()
    {
        var type = getBlock().getType();
        return type == Material.PISTON_STICKY_BASE;
    }

    /// <summary>
    /// Return the direction in which the piston will operate.
    /// </summary>
    /// <returns>Direction of the piston.</returns>
    public BlockFace getDirection() => _direction;
}
