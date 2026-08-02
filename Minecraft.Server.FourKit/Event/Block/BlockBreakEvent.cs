namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;
using Minecraft.Server.FourKit.Entity;

/// <summary>
/// Called when a block is broken by a player.
///
/// If you wish to have the block drop experience, you must set the experience
/// value above 0. By default, experience will be set in the event if:
/// <list type="bullet">
///   <item><description>The player is not in creative or adventure mode</description></item>
///   <item><description>The player can loot the block (ie: does not destroy it completely, by using the correct tool)</description></item>
///   <item><description>The player does not have silk touch</description></item>
///   <item><description>The block drops experience in vanilla Minecraft</description></item>
/// </list>
///
/// Note: Plugins wanting to simulate a traditional block drop should set the
/// block to air and utilize their own methods for determining what the default
/// drop for the block being broken is and what to do about it, if anything.
///
/// If a Block Break event is cancelled, the block will not break and experience
/// will not drop.
/// </summary>
public class BlockBreakEvent : BlockExpEvent, Cancellable
{
    private readonly Player _player;
    private bool _cancel;
    internal BlockBreakEvent(Block block, Player player, int exp)
        : base(block, exp)
    {
        _player = player;
        _cancel = false;
    }

    /// <summary>
    /// Gets the Player that is breaking the block involved in this event.
    /// </summary>
    /// <returns>The Player that is breaking the block involved in this event.</returns>
    public Player getPlayer() => _player;

    /// <inheritdoc/>
    public bool isCancelled() => _cancel;

    /// <inheritdoc/>
    public void setCancelled(bool cancel)
    {
        _cancel = cancel;
    }
}
