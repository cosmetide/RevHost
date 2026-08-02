namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a piston extends.
/// </summary>
public class BlockPistonExtendEvent : BlockPistonEvent
{
    private readonly int _length;

    internal BlockPistonExtendEvent(Block block, int length, BlockFace direction)
        : base(block, direction)
    {
        _length = length;
    }

    /// <summary>
    /// Get the amount of blocks which will be moved while extending.
    /// </summary>
    /// <returns>The amount of moving blocks.</returns>
    public int getLength() => _length;

    /// <summary>
    /// Get an immutable list of the blocks which will be moved by the extending.
    /// </summary>
    /// <returns>Immutable list of the moved blocks.</returns>
    public List<Block> getBlocks()
    {
        var blocks = new List<Block>();
        var world = getBlock().getWorld();
        int x = getBlock().getX();
        int y = getBlock().getY();
        int z = getBlock().getZ();
        var dir = getDirection();

        for (int i = 0; i < _length; i++)
        {
            x += dir.getModX();
            y += dir.getModY();
            z += dir.getModZ();
            blocks.Add(new Block(world, x, y, z));
        }
        return blocks.AsReadOnly().ToList();
    }
}
