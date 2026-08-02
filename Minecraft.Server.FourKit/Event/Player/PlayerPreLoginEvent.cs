namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Net;

/// <summary>
/// Stores details for players attempting to log in.
/// </summary>
public class PlayerPreLoginEvent : Event, Cancellable
{
    private string name;
    private InetSocketAddress ipAddress; //bukkit uses InetAddress but we expose port also
    private bool _cancelled;


    internal PlayerPreLoginEvent(string name, InetSocketAddress ipAddress) : base()
    {
        this.name = name;
        this.ipAddress = ipAddress;
    }


    /// <summary>
    /// Gets the player's name.
    /// </summary>
    /// <returns>The player's name.</returns>
    public string getName() => name;


    /// <summary>
    /// Gets the player IP address.
    /// </summary>
    /// <returns>The IP address.</returns>
    public InetSocketAddress getAddress() => ipAddress;

    /// <inheritdoc/>
    public bool isCancelled() => _cancelled;

    /// <inheritdoc/>
    public void setCancelled(bool cancel) => _cancelled = cancel;
}
