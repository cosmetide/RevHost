namespace Minecraft.Server.FourKit.Net;

/// <summary>
/// Represents an IP Socket Address (IP address + port number).
/// </summary>
public class InetSocketAddress
{
    private readonly InetAddress _address;
    private readonly string _hostname;
    private readonly int _port;

    /// <summary>
    /// Creates a socket address from an IP address and a port number.
    /// </summary>
    /// <param name="addr">The IP address.</param>
    /// <param name="port">The port number.</param>
    public InetSocketAddress(InetAddress addr, int port)
    {
        _address = addr;
        _hostname = addr.getHostAddress();
        _port = port;
    }

    /// <summary>
    /// Creates a socket address from a hostname and a port number.
    /// </summary>
    /// <param name="hostname">The hostname or IP address string.</param>
    /// <param name="port">The port number.</param>
    public InetSocketAddress(string hostname, int port)
    {
        _hostname = hostname ?? string.Empty;
        _address = new InetAddress(_hostname);
        _port = port;
    }

    /// <summary>
    /// Creates a socket address where the IP address is the wildcard address
    /// and the port number a specified value.
    /// </summary>
    /// <param name="port">The port number.</param>
    public InetSocketAddress(int port)
    {
        _hostname = "0.0.0.0";
        _address = new InetAddress(_hostname);
        _port = port;
    }

    /// <summary>
    /// Gets the InetAddress.
    /// </summary>
    /// <returns>The InetAddress.</returns>
    public InetAddress getAddress() => _address;

    /// <summary>
    /// Gets the hostname.
    /// </summary>
    /// <returns>The hostname, or the IP address string if created from an address.</returns>
    public string getHostName() => _hostname;

    /// <summary>
    /// Gets the port number.
    /// </summary>
    /// <returns>The port number.</returns>
    public int getPort() => _port;

    /// <inheritdoc/>
    public override int GetHashCode() => HashCode.Combine(_hostname, _port);

    /// <inheritdoc/>
    public override bool Equals(object? obj)
    {
        if (obj is InetSocketAddress other)
            return _hostname == other._hostname && _port == other._port;
        return false;
    }

    /// <inheritdoc/>
    public override string ToString() => _hostname + ":" + _port;
}
