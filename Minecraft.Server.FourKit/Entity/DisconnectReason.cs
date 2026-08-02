namespace Minecraft.Server.FourKit.Entity;
// eh

/// <summary>
/// Enum representing the reason a player was disconnected from the server.
/// mirrored from <c>DisconnectPacket::eDisconnectReason</c>.
/// </summary>
public enum DisconnectReason
{
    /// <summary>No specific reason.</summary>
    NONE = 0,
    /// <summary>The player quit voluntarily.</summary>
    QUITTING = 1,
    /// <summary>The connection was closed.</summary>
    CLOSED = 2,
    /// <summary>The login took too long.</summary>
    LOGIN_TOO_LONG = 3,
    /// <summary>The player had an illegal stance.</summary>
    ILLEGAL_STANCE = 4,
    /// <summary>The player had an illegal position.</summary>
    ILLEGAL_POSITION = 5,
    /// <summary>The player moved too quickly.</summary>
    MOVED_TOO_QUICKLY = 6,
    /// <summary>The player was flying when not allowed.</summary>
    NO_FLYING = 7,
    /// <summary>The player was kicked by an operator or plugin.</summary>
    KICKED = 8,
    /// <summary>The connection timed out.</summary>
    TIME_OUT = 9,
    /// <summary>Packet overflow.</summary>
    OVERFLOW = 10,
    /// <summary>End of stream reached unexpectedly.</summary>
    END_OF_STREAM = 11,
    /// <summary>The server is full.</summary>
    SERVER_FULL = 12,
    /// <summary>The server is outdated.</summary>
    OUTDATED_SERVER = 13,
    /// <summary>The client is outdated.</summary>
    OUTDATED_CLIENT = 14,
    /// <summary>An unexpected packet was received.</summary>
    UNEXPECTED_PACKET = 15,
    /// <summary>Connection creation failed.</summary>
    CONNECTION_CREATION_FAILED = 16,
    /// <summary>The host does not have multiplayer privileges.</summary>
    NO_MULTIPLAYER_PRIVILEGES_HOST = 17,
    /// <summary>The joining player does not have multiplayer privileges.</summary>
    NO_MULTIPLAYER_PRIVILEGES_JOIN = 18,
    /// <summary>All local players lack UGC permissions.</summary>
    NO_UGC_ALL_LOCAL = 19,
    /// <summary>A single local player lacks UGC permissions.</summary>
    NO_UGC_SINGLE_LOCAL = 20,
    /// <summary>All local players have content restrictions.</summary>
    CONTENT_RESTRICTED_ALL_LOCAL = 21,
    /// <summary>A single local player has content restrictions.</summary>
    CONTENT_RESTRICTED_SINGLE_LOCAL = 22,
    /// <summary>A remote player lacks UGC permissions.</summary>
    NO_UGC_REMOTE = 23,
    /// <summary>No friends in the game.</summary>
    NO_FRIENDS_IN_GAME = 24,
    /// <summary>The player was banned.</summary>
    BANNED = 25,
    /// <summary>The player is not friends with the host.</summary>
    NOT_FRIENDS_WITH_HOST = 26,
    /// <summary>NAT type mismatch.</summary>
    NAT_MISMATCH = 27,
}
