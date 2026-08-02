namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called when a player joins a server
/// </summary>
public class PlayerJoinEvent : PlayerEvent
{
    private string _joinMessage;
    internal PlayerJoinEvent(Player player) : base(player)
    {
        _joinMessage = $"{player.getName()} joined the game";
    }

    /// <summary>
    /// Gets the join message to send to all online players
    /// </summary>
    /// <returns>string join message</returns>
    public string getJoinMessage() => _joinMessage;

    /// <summary>
    /// Sets the join message to send to all online players
    /// </summary>
    /// <param name="joinMessage">join message.</param>
    public void setJoinMessage(string? joinMessage)
    {
        _joinMessage = joinMessage ?? string.Empty;
    }
}
