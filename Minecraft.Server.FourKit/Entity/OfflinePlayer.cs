namespace Minecraft.Server.FourKit.Entity;

/// <summary>
/// Represents a player identity that may or may not currently be online.
/// </summary>
public interface OfflinePlayer
{
    /// <summary>Returns the name of this player.</summary>
    /// <returns>The player's name.</returns>
    string getName();

    /// <summary>Gets a Player object that this represents, if there is one.</summary>
    /// <returns>A <see cref="Player"/> instance if the player is online; otherwise <c>null</c>.</returns>
    Player? getPlayer();

    /// <summary>
    /// Returns the UUID that uniquely identifies this player across sessions.
    /// This is the player-specific UUID, not the entity UUID.
    /// </summary>
    /// <returns>The player's unique identifier.</returns>
    Guid getUniqueId();

    /// <summary>Checks if this player is currently online.</summary>
    /// <returns><c>true</c> if the player is online; otherwise <c>false</c>.</returns>
    bool isOnline();
}
