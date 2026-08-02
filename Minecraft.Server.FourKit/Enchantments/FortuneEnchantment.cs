using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class FortuneEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.WOOD_PICKAXE, Material.STONE_PICKAXE, Material.IRON_PICKAXE, Material.GOLD_PICKAXE, Material.DIAMOND_PICKAXE,
        Material.WOOD_AXE,     Material.STONE_AXE,     Material.IRON_AXE,     Material.GOLD_AXE,     Material.DIAMOND_AXE,
        Material.WOOD_SPADE,   Material.STONE_SPADE,   Material.IRON_SPADE,   Material.GOLD_SPADE,   Material.DIAMOND_SPADE,
        Material.WOOD_HOE,     Material.STONE_HOE,     Material.IRON_HOE,     Material.GOLD_HOE,     Material.DIAMOND_HOE,
    };

    static readonly EnchantmentType[] conflictedEnchants = {
        EnchantmentType.SILK_TOUCH
    };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.TOOL;

    public override EnchantmentType getEnchantType() => EnchantmentType.LOOT_BONUS_BLOCKS;

    public override int getMaxLevel() => 3;

    public override string getName() => "fortune";

    public override int getStartLevel() => 1;
}
