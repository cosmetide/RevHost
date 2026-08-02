namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called early in the command handling process. This event is only for very
/// exceptional cases and you should not normally use it.
///
/// <para>If a PlayerCommandPreprocessEvent is cancelled, the command will not
/// be executed in the server, but will still pass to other plugins.</para>
/// </summary>
public class PlayerCommandPreprocessEvent : PlayerEvent, Cancellable
{
    private string _message;
    private bool _cancel;

    internal PlayerCommandPreprocessEvent(Player player, string message) : base(player)
    {
        _message = message;
        _cancel = false;
    }

    /// <inheritdoc/>
    public bool isCancelled() => _cancel;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }

    /// <summary>
    /// Gets the command that the player is attempting to send. All commands
    /// begin with a special character; implementations do not consider the
    /// first character when executing the content.
    /// </summary>
    /// <returns>Message the player is attempting to send.</returns>
    public string getMessage() => _message;

    /// <summary>
    /// Sets the command that the player will send. All commands begin with a
    /// special character; implementations do not consider the first character
    /// when executing the content.
    /// </summary>
    /// <param name="command">New message that the player will send.</param>
    /// <exception cref="ArgumentException">If command is null or empty.</exception>
    public void setMessage(string command)
    {
        if (string.IsNullOrEmpty(command))
            throw new ArgumentException("Command may not be null or empty", nameof(command));
        _message = command;
    }
}
