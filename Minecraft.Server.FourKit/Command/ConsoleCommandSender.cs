namespace Minecraft.Server.FourKit.Command;

/// <summary>
/// Represents the server console as a command sender.
/// </summary>
public class ConsoleCommandSender : CommandSender
{
    internal static readonly ConsoleCommandSender Instance = new();

    private ConsoleCommandSender() { }

    /// <inheritdoc/>
    public void sendMessage(string message)
    {
        ServerLog.Info("console", message);
    }

    /// <inheritdoc/>
    public void sendMessage(string[] messages)
    {
        foreach (var msg in messages)
            sendMessage(msg);
    }

    /// <inheritdoc/>
    public string getName() => "CONSOLE";
}
