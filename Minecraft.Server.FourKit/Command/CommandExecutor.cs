namespace Minecraft.Server.FourKit.Command;

/// <summary>
/// Represents a class which contains a single method for executing commands.
/// </summary>
public interface CommandExecutor
{
    /// <summary>
    /// Executes the given command, returning its success.
    /// </summary>
    /// <param name="sender">Source of the command.</param>
    /// <param name="command">Command which was executed.</param>
    /// <param name="label">Alias of the command which was used.</param>
    /// <param name="args">Passed command arguments.</param>
    /// <returns><c>true</c> if a valid command, otherwise <c>false</c>.</returns>
    bool onCommand(CommandSender sender, Command command, string label, string[] args);
}
