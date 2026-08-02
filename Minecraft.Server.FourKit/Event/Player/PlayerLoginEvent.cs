namespace Minecraft.Server.FourKit.Event.Player;

using Minecraft.Server.FourKit.Enums;
using Minecraft.Server.FourKit.Net;

/// <summary>
/// Stores details for players attempting to log in.
/// </summary>
public class PlayerLoginEvent : Event, Cancellable
{
    private string name;
    private InetSocketAddress ipAddress; //bukkit uses InetAddress but we expose port also

    private ulong onlineXuid;
    private ulong offlineXuid;
    private bool changedXuidValues;

    private LoginType loginType;

    private bool _cancelled;

    internal PlayerLoginEvent(string name, InetSocketAddress ipAddress, LoginType type, ulong onlineXuid, ulong offlineXuid) : base()
    {
        this.name = name;
        this.ipAddress = ipAddress;
        this.onlineXuid = onlineXuid;
        this.offlineXuid = offlineXuid;
        this.changedXuidValues = false;

        this.loginType = type;
    }

    public LoginType getLoginType() => loginType;


    /// <summary>
    /// <b>Experimental.</b> Gets the online XUID (Xbox User ID), used for guests (splitscreen users).
    /// </summary>
    /// <returns>The online XUID value.</returns>
    public ulong getOnlineXuid() => onlineXuid;

    /// <summary>
    /// <b>Experimental.</b> Sets the online XUID (Xbox User ID). Marks XUID values as changed.
    /// </summary>
    /// <param name="newXuid">The new online XUID value.</param>
    public void setOnlineXuid(ulong newXuid)
    {
        this.onlineXuid = newXuid;
        this.changedXuidValues = true;
    }

    /// <summary>
    /// <b>Experimental.</b> Gets the offline XUID (Xbox User ID), which is the main XUID used by the client.
    /// </summary>
    /// <returns>The offline XUID value.</returns>
    public ulong getOfflineXuid() => offlineXuid;

    /// <summary>
    /// <b>Experimental.</b> Sets the offline XUID (Xbox User ID). Marks XUID values as changed.
    /// </summary>
    /// <param name="newXuid">The new offline XUID value.</param>
    public void setOfflineXuid(ulong newXuid)
    {
        this.offlineXuid = newXuid;
        this.changedXuidValues = true;
    }

    /// <summary>
    /// <b>Experimental.</b> Returns true if either XUID value has been changed via setters.
    /// </summary>
    /// <returns>True if XUID values have been changed; otherwise, false.</returns>
    public bool hasChangedXuidValues() => changedXuidValues;


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
