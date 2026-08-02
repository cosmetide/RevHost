namespace Minecraft.Server.FourKit.Block;

/// <summary>
/// Represents the action type for a player interaction.
/// </summary>
public enum Action
{
    /// <summary>Left-clicking the air.</summary>
    LEFT_CLICK_AIR = 0,

    /// <summary>Left-clicking a block.</summary>
    LEFT_CLICK_BLOCK = 1,

    /// <summary>Right-clicking the air.</summary>
    RIGHT_CLICK_AIR = 2,

    /// <summary>Right-clicking a block.</summary>
    RIGHT_CLICK_BLOCK = 3,

    /// <summary>
    /// Stepping onto or into a block (Ass-pressure).
    /// Examples: Jumping on soil, Standing on pressure plate,
    /// Triggering redstone ore, Triggering tripwire.
    /// </summary>
    PHYSICAL = 4
}
