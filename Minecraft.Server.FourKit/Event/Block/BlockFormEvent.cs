namespace Minecraft.Server.FourKit.Event.Block;

using Minecraft.Server.FourKit.Block;

/// <summary>
/// Called when a block is formed or spreads based on world conditions.
/// Use <see cref="BlockSpreadEvent"/> to catch blocks that actually spread
/// and don't just "randomly" form.
///
/// <para>Examples:</para>
/// <list type="bullet">
///   <item><description>Snow forming due to a snow storm.</description></item>
///   <item><description>Ice forming in a snowy Biome like Taiga or Tundra.</description></item>
/// </list>
///
/// <para>If a Block Form event is cancelled, the block will not be formed.</para>
/// </summary>
public class BlockFormEvent : BlockGrowEvent, Cancellable
{
    internal BlockFormEvent(Block block, BlockState newState) : base(block, newState)
    {
    }
}
