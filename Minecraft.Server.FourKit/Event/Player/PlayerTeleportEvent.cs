namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Holds information for player teleport events.
/// </summary>
public class PlayerTeleportEvent : PlayerMoveEvent
{
    /// <summary>
    /// Represents the cause of a player teleportation.
    /// </summary>
    public enum TeleportCause
    {
        /// <summary>Indicates the teleportation was caused by a player throwing an Ender Pearl.</summary>
        ENDER_PEARL,
        /// <summary>Indicates the teleportation was caused by a player executing a command.</summary>
        COMMAND,
        /// <summary>Indicates the teleportation was caused by a plugin.</summary>
        PLUGIN,
        /// <summary>Indicates the teleportation was caused by a player entering a Nether portal.</summary>
        NETHER_PORTAL,
        /// <summary>Indicates the teleportation was caused by a player entering an End portal.</summary>
        END_PORTAL,
        /// <summary>Indicates the teleportation was caused by an event not covered by this enum.</summary>
        UNKNOWN,
    }

    private readonly TeleportCause _cause;

    internal PlayerTeleportEvent(Player player, Location from, Location to)
        : this(player, from, to, TeleportCause.UNKNOWN) { }
    internal PlayerTeleportEvent(Player player, Location from, Location to, TeleportCause cause)
        : base(player, from, to)
    {
        _cause = cause;
    }

    /// <summary>
    /// Gets the cause of this teleportation event.
    /// </summary>
    /// <returns>The cause of the event.</returns>
    public TeleportCause getCause() => _cause;
}
