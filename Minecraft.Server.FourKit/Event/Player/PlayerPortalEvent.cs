namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called when a player is about to teleport because it is in contact with a portal.
/// </summary>
public class PlayerPortalEvent : PlayerTeleportEvent
{
    internal PlayerPortalEvent(Player player, Location from, Location to)
        : base(player, from, to, TeleportCause.UNKNOWN) { }

    /// <summary>
    /// Constructs a new PlayerPortalEvent with the given cause.
    /// </summary>
    /// <param name="player">The player entering the portal.</param>
    /// <param name="from">The location the player is coming from.</param>
    /// <param name="to">The location the player is teleporting to.</param>
    /// <param name="cause">The cause of this teleportation (should be a portal-related cause).</param>
    public PlayerPortalEvent(Player player, Location from, Location to, TeleportCause cause)
        : base(player, from, to, cause) { }
}
