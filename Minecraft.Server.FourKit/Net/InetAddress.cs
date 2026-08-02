namespace Minecraft.Server.FourKit.Net;

/// <summary>
/// Represents an Internet Protocol (IP) address.
/// </summary>
public class InetAddress
{
    private readonly string _hostAddress;

    internal InetAddress(string hostAddress)
    {
        _hostAddress = hostAddress ?? string.Empty;
    }

    /// <summary>
    /// Returns the IP address string in textual presentation.
    /// </summary>
    /// <returns>The IP address as a string.</returns>
    public string getHostAddress() => _hostAddress;

    /// <summary>
    /// Gets the host name for this IP address.
    /// For this implementation, returns the IP address string.
    /// </summary>
    /// <returns>The host name.</returns>
    public string getHostName() => _hostAddress;

    /// <summary>
    /// Returns the raw IP address of this InetAddress object as bytes.
    /// </summary>
    /// <returns>The raw IP address bytes, or an empty array if parsing fails.</returns>
    public byte[] getAddress()
    {
        if (System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return ip.GetAddressBytes();
        return [];
    }

    /// <summary>
    /// Checks whether this is a loopback address (127.x.x.x or ::1).
    /// </summary>
    /// <returns><c>true</c> if this is a loopback address.</returns>
    public bool isLoopbackAddress()
    {
        if (System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return System.Net.IPAddress.IsLoopback(ip);
        return false;
    }

    /// <summary>
    /// Checks whether this is a site-local (private) address.
    /// </summary>
    /// <returns><c>true</c> if this is a site-local address.</returns>
    public bool isSiteLocalAddress()
    {
        if (!System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return false;
        byte[] bytes = ip.GetAddressBytes();
        if (bytes.Length != 4) return false;
        // 10.x.x.x, 172.16-31.x.x, 192.168.x.x
        if (bytes[0] == 10) return true;
        if (bytes[0] == 172 && bytes[1] >= 16 && bytes[1] <= 31) return true;
        if (bytes[0] == 192 && bytes[1] == 168) return true;
        return false;
    }

    /// <summary>
    /// Checks whether this is a link-local address (169.254.x.x).
    /// </summary>
    /// <returns><c>true</c> if this is a link-local address.</returns>
    public bool isLinkLocalAddress()
    {
        if (!System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return false;
        byte[] bytes = ip.GetAddressBytes();
        if (bytes.Length != 4) return false;
        return bytes[0] == 169 && bytes[1] == 254;
    }

    /// <summary>
    /// Checks whether this is a multicast address (224-239.x.x.x).
    /// </summary>
    /// <returns><c>true</c> if this is a multicast address.</returns>
    public bool isMulticastAddress()
    {
        if (!System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return false;
        byte[] bytes = ip.GetAddressBytes();
        if (bytes.Length != 4) return false;
        return bytes[0] >= 224 && bytes[0] <= 239;
    }

    /// <summary>
    /// Checks whether this is the wildcard (any) address (0.0.0.0).
    /// </summary>
    /// <returns><c>true</c> if this is the wildcard address.</returns>
    public bool isAnyLocalAddress()
    {
        if (System.Net.IPAddress.TryParse(_hostAddress, out var ip))
            return ip.Equals(System.Net.IPAddress.Any) || ip.Equals(System.Net.IPAddress.IPv6Any);
        return false;
    }

    /// <inheritdoc/>
    public override string ToString() => _hostAddress;
}
