namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Fired when a player sends a chat message.
///
/// <para>When the event finishes execution the server formats the final
/// output using the same format specifiers from Java.
/// <c>%1$s</c> is the player's display name and <c>%2$s</c> is the
/// message, exactly like Bukkits <c>PlayerChatEvent</c>.</para>
/// </summary>
public class PlayerChatEvent : PlayerEvent, Cancellable
{
    private string _message;
    private string _format;
    private bool _cancelled;
    internal PlayerChatEvent(Player player, string message) : base(player)
    {
        _message = message;
        _format = "<%1$s> %2$s";
    }

    /// <summary>
    /// Gets the message that the player is attempting to send.
    /// This message will be used with <see cref="getFormat"/>.
    /// </summary>
    /// <returns>Message the player is attempting to send.</returns>
    public string getMessage() => _message;

    /// <summary>
    /// Sets the message that the player will send.
    /// This message will be used with <see cref="getFormat"/>.
    /// </summary>
    /// <param name="message">New message that the player will send.</param>
    public void setMessage(string message)
    {
        _message = message;
    }

    /// <summary>
    /// Gets the format used to display this chat message.
    ///
    /// <para>When this event finishes execution, the first format parameter
    /// (<c>%1$s</c>) is <c>Player.getDisplayName()</c> and the second
    /// parameter (<c>%2$s</c>) is <c>getMessage()</c>.</para>
    /// </summary>
    /// <returns>A Java-style positional format string compatible with Bukkit.</returns>
    public string getFormat() => _format;

    /// <summary>
    /// Sets the format used to display this chat message.
    ///
    /// <para>When this event finishes execution, the first format parameter
    /// (<c>%1$s</c>) is <c>Player.getDisplayName()</c> and the second
    /// parameter (<c>%2$s</c>) is <c>getMessage()</c>.</para>
    /// </summary>
    /// <param name="format">A Java-style positional format string (e.g. <c>"&lt;%1$s&gt; %2$s"</c>).</param>
    /// <exception cref="ArgumentNullException">If format is <c>null</c>.</exception>
    public void setFormat(string format)
    {
        ArgumentNullException.ThrowIfNull(format);
        _format = format;
    }

    /// <inheritdoc />
    public bool isCancelled() => _cancelled;

    /// <inheritdoc />
    public void setCancelled(bool cancel)
    {
        _cancelled = cancel;
    }
}
