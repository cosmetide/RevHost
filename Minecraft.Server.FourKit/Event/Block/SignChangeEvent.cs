namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called when a sign is changed by a player.
/// </summary>
public class SignChangeEvent : BlockEvent, Cancellable
{
    private readonly Player _player;
    private readonly string[] _lines;
    private bool _cancel;
    internal SignChangeEvent(Block theBlock, Player thePlayer, string[] theLines)
        : base(theBlock)
    {
        _player = thePlayer;
        _lines = theLines;
        _cancel = false;
    }

    /// <summary>
    /// Gets the player changing the sign involved in this event.
    /// </summary>
    /// <returns>The Player involved in this event.</returns>
    public Player getPlayer() => _player;

    /// <summary>
    /// Gets all of the lines of text from the sign involved in this event.
    /// </summary>
    /// <returns>The String array for the sign's lines new text.</returns>
    public string[] getLines() => _lines;

    /// <summary>
    /// Gets a single line of text from the sign involved in this event.
    /// </summary>
    /// <param name="index">Index of the line to get.</param>
    /// <returns>The String containing the line of text associated with the provided index.</returns>
    /// <exception cref="IndexOutOfRangeException">Thrown when the provided index is &gt; 3 or &lt; 0.</exception>
    public string getLine(int index)
    {
        if (index < 0 || index > 3)
            throw new IndexOutOfRangeException($"Line index must be between 0 and 3, got {index}");
        return _lines[index];
    }

    /// <summary>
    /// Sets a single line for the sign involved in this event.
    /// </summary>
    /// <param name="index">Index of the line to set.</param>
    /// <param name="line">Text to set.</param>
    /// <exception cref="IndexOutOfRangeException">Thrown when the provided index is &gt; 3 or &lt; 0.</exception>
    public void setLine(int index, string line)
    {
        if (index < 0 || index > 3)
            throw new IndexOutOfRangeException($"Line index must be between 0 and 3, got {index}");
        _lines[index] = line;
    }

    /// <inheritdoc />
    public bool isCancelled() => _cancel;

    /// <inheritdoc />
    public void setCancelled(bool cancel) => _cancel = cancel;
}
