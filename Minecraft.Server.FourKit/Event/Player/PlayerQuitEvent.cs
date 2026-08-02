namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Fired when a player disconnects from the server.
/// Plugins may read or modify the quit message that is broadcast to all
/// online players.
/// </summary>
public class PlayerQuitEvent : PlayerEvent
{
    private string _quitMessage;
    internal PlayerQuitEvent(Player player) : base(player)
    {
        _quitMessage = $"{player.getName()} left the game";
    }

    /// <summary>
    /// Gets the quit message to send to all online players.
    /// </summary>
    /// <returns>The quit message.</returns>
    public string getQuitMessage() => _quitMessage;

    /// <summary>
    /// Sets the quit message to send to all online players.
    /// </summary>
    /// <param name="quitMessage">The new quit message, or <c>null</c> to suppress it.</param>
    public void setQuitMessage(string? quitMessage)
    {
        _quitMessage = quitMessage ?? string.Empty;
    }
}
