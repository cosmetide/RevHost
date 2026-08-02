namespace Minecraft.Server.FourKit.Command;

/// <summary>
/// Represents a <see cref="Command"/> belonging to a plugin.
/// </summary>
public class PluginCommand : Command
{
    private CommandExecutor? _executor;

    // should this remain internal?
    /// <summary>
    /// Creates a new plugin command with the given name.
    /// Use <see cref="FourKit.getCommand"/> to obtain instances.
    /// </summary>
    /// <param name="name">Name of this command.</param>
    internal PluginCommand(string name) : base(name)
    {
    }

    /// <inheritdoc/>
    public override bool execute(CommandSender sender, string commandLabel, string[] args)
    {
        if (_executor != null)
            return _executor.onCommand(sender, this, commandLabel, args);
        return false;
    }

    /// <summary>
    /// Gets the <see cref="CommandExecutor"/> associated with this command.
    /// </summary>
    /// <returns>The command executor, or <c>null</c>.</returns>
    public CommandExecutor? getExecutor() => _executor;

    /// <summary>
    /// Sets the <see cref="CommandExecutor"/> to run when the command is dispatched.
    /// </summary>
    /// <param name="executor">New executor to set.</param>
    /// <returns><c>true</c> if the executor was set.</returns>
    public bool setExecutor(CommandExecutor executor)
    {
        _executor = executor;
        return true;
    }
}
