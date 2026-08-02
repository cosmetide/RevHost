namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// An event that is called when a block yields experience.
/// </summary>
public class BlockExpEvent : BlockEvent
{
    private int _exp;
    internal BlockExpEvent(Block block, int exp)
        : base(block)
    {
        _exp = exp;
    }

    /// <summary>
    /// Get the experience dropped by the block after the event has processed.
    /// </summary>
    /// <returns>The experience to drop.</returns>
    public int getExpToDrop() => _exp;

    /// <summary>
    /// Set the amount of experience dropped by the block after the event has processed.
    /// </summary>
    /// <param name="exp">1 or higher to drop experience, else nothing will drop.</param>
    public void setExpToDrop(int exp)
    {
        _exp = exp;
    }
}
