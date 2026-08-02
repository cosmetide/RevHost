namespace Minecraft.Server.FourKit.Command;

/// <summary>
/// Represents a Command, which executes various tasks upon user input.
/// </summary>
public abstract class Command
{
    private string _name;
    private string _description;
    private string _usage;
    private List<string> _aliases;

    /// <summary>
    /// Creates a new command with the given name and no aliases.
    /// </summary>
    /// <param name="name">Name of this command.</param>
    protected Command(string name)
    {
        _name = name;
        _description = string.Empty;
        _usage = "/" + name;
        _aliases = new List<string>();
    }

    /// <summary>
    /// Creates a new command with the given name, description, and aliases.
    /// </summary>
    /// <param name="name">Name of this command.</param>
    /// <param name="description">A brief description of this command.</param>
    /// <param name="aliases">A list of aliases for this command.</param>
    protected Command(string name, string description, List<string> aliases)
    {
        _name = name;
        _description = description ?? string.Empty;
        _usage = "/" + name;
        _aliases = aliases ?? new List<string>();
    }

    /// <summary>
    /// Executes the command, returning its success.
    /// </summary>
    /// <param name="sender">Source of the command.</param>
    /// <param name="commandLabel">Alias of the command which was used.</param>
    /// <param name="args">Passed command arguments.</param>
    /// <returns><c>true</c> if the command was successful, otherwise <c>false</c>.</returns>
    public abstract bool execute(CommandSender sender, string commandLabel, string[] args);

    /// <summary>
    /// Returns a list of active aliases of this command.
    /// </summary>
    /// <returns>List of aliases.</returns>
    public List<string> getAliases() => new(_aliases);

    /// <summary>
    /// Gets a brief description of this command.
    /// </summary>
    /// <returns>Description of this command.</returns>
    public string getDescription() => _description;

    /// <summary>
    /// Returns the current label for this command.
    /// </summary>
    /// <returns>Current label.</returns>
    public string getLabel() => _name;

    /// <summary>
    /// Returns the name of this command.
    /// </summary>
    /// <returns>Name of this command.</returns>
    public string getName() => _name;

    /// <summary>
    /// Gets an example usage of this command.
    /// </summary>
    /// <returns>Usage string.</returns>
    public string getUsage() => _usage;

    /// <summary>
    /// Sets the list of aliases to request on registration for this command.
    /// </summary>
    /// <param name="aliases">Aliases to register.</param>
    /// <returns>This command.</returns>
    public Command setAliases(List<string> aliases)
    {
        _aliases = aliases ?? new List<string>();
        return this;
    }

    /// <summary>
    /// Sets a brief description of this command.
    /// </summary>
    /// <param name="description">New command description.</param>
    /// <returns>This command.</returns>
    public Command setDescription(string description)
    {
        _description = description ?? string.Empty;
        return this;
    }

    /// <summary>
    /// Sets the example usage of this command.
    /// </summary>
    /// <param name="usage">New example usage.</param>
    /// <returns>This command.</returns>
    public Command setUsage(string usage)
    {
        _usage = usage ?? string.Empty;
        return this;
    }
}
