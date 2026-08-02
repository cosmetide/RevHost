namespace Minecraft.Server.FourKit;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents the various type of game modes that <see cref="HumanEntity"/>s may have
/// </summary>
public enum GameMode
{
    /// <summary>
    /// Survival mode is the "normal" gameplay type, with no special features.
    /// </summary>
    SURVIVAL = 0,

    /// <summary>
    /// Creative mode may fly, build instantly, become invulnerable and create free items.
    /// </summary>
    CREATIVE = 1,

    /// <summary>
    /// Adventure mode cannot break blocks without the correct tools.
    /// </summary>
    ADVENTURE = 2,
}
