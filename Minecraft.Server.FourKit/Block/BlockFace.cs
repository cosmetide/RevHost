namespace Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents the face of a block.
/// </summary>
public enum BlockFace
{
    DOWN = 0,
    UP = 1,
    NORTH = 2,
    SOUTH = 3,
    WEST = 4,
    EAST = 5,
    SELF = 6,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH_EAST,
    SOUTH_WEST,
    WEST_NORTH_WEST,
    NORTH_NORTH_WEST,
    NORTH_NORTH_EAST,
    EAST_NORTH_EAST,
    EAST_SOUTH_EAST,
    SOUTH_SOUTH_EAST,
    SOUTH_SOUTH_WEST,
    WEST_SOUTH_WEST
}

public static class BlockFaceExtensions
{
    /// <summary>
    /// Get the amount of X-coordinates to modify to get the represented block.
    /// </summary>
    /// <param name="face">The block face.</param>
    /// <returns>Amount of X-coordinates to modify.</returns>
    public static int getModX(this BlockFace face) => face switch
    {
        BlockFace.EAST => 1,
        BlockFace.WEST => -1,
        BlockFace.NORTH_EAST => 1,
        BlockFace.NORTH_WEST => -1,
        BlockFace.SOUTH_EAST => 1,
        BlockFace.SOUTH_WEST => -1,
        BlockFace.EAST_NORTH_EAST => 1,
        BlockFace.EAST_SOUTH_EAST => 1,
        BlockFace.NORTH_NORTH_EAST => 1,
        BlockFace.SOUTH_SOUTH_EAST => 1,
        BlockFace.WEST_NORTH_WEST => -1,
        BlockFace.WEST_SOUTH_WEST => -1,
        BlockFace.NORTH_NORTH_WEST => -1,
        BlockFace.SOUTH_SOUTH_WEST => -1,
        _ => 0
    };

    /// <summary>
    /// Get the amount of Y-coordinates to modify to get the represented block.
    /// </summary>
    /// <param name="face">The block face.</param>
    /// <returns>Amount of Y-coordinates to modify.</returns>
    public static int getModY(this BlockFace face) => face switch
    {
        BlockFace.UP => 1,
        BlockFace.DOWN => -1,
        _ => 0
    };

    /// <summary>
    /// Get the amount of Z-coordinates to modify to get the represented block.
    /// </summary>
    /// <param name="face">The block face.</param>
    /// <returns>Amount of Z-coordinates to modify.</returns>
    public static int getModZ(this BlockFace face) => face switch
    {
        BlockFace.NORTH => -1,
        BlockFace.SOUTH => 1,
        BlockFace.NORTH_EAST => -1,
        BlockFace.NORTH_WEST => -1,
        BlockFace.SOUTH_EAST => 1,
        BlockFace.SOUTH_WEST => 1,
        BlockFace.NORTH_NORTH_EAST => -1,
        BlockFace.NORTH_NORTH_WEST => -1,
        BlockFace.EAST_NORTH_EAST => -1,
        BlockFace.WEST_NORTH_WEST => -1,
        BlockFace.EAST_SOUTH_EAST => 1,
        BlockFace.WEST_SOUTH_WEST => 1,
        BlockFace.SOUTH_SOUTH_EAST => 1,
        BlockFace.SOUTH_SOUTH_WEST => 1,
        _ => 0
    };

    /// <summary>
    /// Gets the opposite face of this block face.
    /// </summary>
    /// <param name="face">The block face.</param>
    /// <returns>The opposite block face.</returns>
    public static BlockFace getOppositeFace(this BlockFace face) => face switch
    {
        BlockFace.NORTH => BlockFace.SOUTH,
        BlockFace.SOUTH => BlockFace.NORTH,
        BlockFace.EAST => BlockFace.WEST,
        BlockFace.WEST => BlockFace.EAST,
        BlockFace.UP => BlockFace.DOWN,
        BlockFace.DOWN => BlockFace.UP,
        BlockFace.NORTH_EAST => BlockFace.SOUTH_WEST,
        BlockFace.NORTH_WEST => BlockFace.SOUTH_EAST,
        BlockFace.SOUTH_EAST => BlockFace.NORTH_WEST,
        BlockFace.SOUTH_WEST => BlockFace.NORTH_EAST,
        BlockFace.WEST_NORTH_WEST => BlockFace.EAST_SOUTH_EAST,
        BlockFace.NORTH_NORTH_WEST => BlockFace.SOUTH_SOUTH_EAST,
        BlockFace.NORTH_NORTH_EAST => BlockFace.SOUTH_SOUTH_WEST,
        BlockFace.EAST_NORTH_EAST => BlockFace.WEST_SOUTH_WEST,
        BlockFace.EAST_SOUTH_EAST => BlockFace.WEST_NORTH_WEST,
        BlockFace.SOUTH_SOUTH_EAST => BlockFace.NORTH_NORTH_WEST,
        BlockFace.SOUTH_SOUTH_WEST => BlockFace.NORTH_NORTH_EAST,
        BlockFace.WEST_SOUTH_WEST => BlockFace.EAST_NORTH_EAST,
        _ => BlockFace.SELF
    };
}
