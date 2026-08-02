namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Fired when a player is kicked from the server.
/// If cancelled, the kick will not take place and the player remains connected.
/// Plugins may modify the kick reason and the leave message broadcast to
/// all online players.
/// </summary>
public class PlayerKickEvent : PlayerEvent, Cancellable
{
    private DisconnectReason _reason;
    private string _leaveMessage;
    private bool _cancelled;

    internal PlayerKickEvent(Player playerKicked, DisconnectReason kickReason, string leaveMessage)
        : base(playerKicked)
    {
        _reason = kickReason;
        _leaveMessage = leaveMessage;
    }

    /// <summary>
    /// Gets the reason why the player is getting kicked.
    /// </summary>
    /// <returns>The disconnect reason.</returns>
    public DisconnectReason getReason() => _reason;

    /// <summary>
    /// Sets the reason why the player is getting kicked.
    /// </summary>
    /// <param name="kickReason">The new disconnect reason.</param>
    public void setReason(DisconnectReason kickReason)
    {
        _reason = kickReason;
    }

    /// <summary>
    /// Gets the leave message sent to all online players.
    /// </summary>
    /// <returns>The leave message.</returns>
    public string getLeaveMessage() => _leaveMessage;

    /// <summary>
    /// Sets the leave message sent to all online players.
    /// </summary>
    /// <param name="leaveMessage">The new leave message.</param>
    public void setLeaveMessage(string leaveMessage)
    {
        _leaveMessage = leaveMessage;
    }

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
