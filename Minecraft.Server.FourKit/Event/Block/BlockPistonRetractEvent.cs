namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a piston retracts.
/// </summary>
public class BlockPistonRetractEvent : BlockPistonEvent
{
    internal BlockPistonRetractEvent(Block block, BlockFace direction)
        : base(block, direction)
    {
    }

    /// <summary>
    /// Gets the location where the possible moving block might be if the
    /// retracting piston is sticky.
    /// </summary>
    /// <returns>The possible location of the possibly moving block.</returns>
    public Location getRetractLocation()
    {
        var block = getBlock();
        var dir = getDirection();
        return new Location(
            block.getWorld(),
            block.getX() + dir.getModX() * 2,
            block.getY() + dir.getModY() * 2,
            block.getZ() + dir.getModZ() * 2);
    }
}
