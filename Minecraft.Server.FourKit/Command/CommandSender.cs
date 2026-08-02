namespace Minecraft.Server.FourKit.Command;

/// <summary>
/// Represents something that can send commands and receive messages.
/// </summary>
public interface CommandSender
{
    /// <summary>
    /// Sends this sender a message.
    /// </summary>
    /// <param name="message">Message to be displayed.</param>
    void sendMessage(string message);

    /// <summary>
    /// Sends this sender multiple messages.
    /// </summary>
    /// <param name="messages">An array of messages to be displayed.</param>
    void sendMessage(string[] messages);

    /// <summary>
    /// Gets the name of this command sender.
    /// </summary>
    /// <returns>Name of the sender.</returns>
    string getName();
}
