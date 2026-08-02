using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class EfficiencyEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.WOOD_SWORD,   Material.STONE_SWORD,   Material.IRON_SWORD,   Material.GOLD_SWORD,   Material.DIAMOND_SWORD,
        Material.WOOD_PICKAXE, Material.STONE_PICKAXE, Material.IRON_PICKAXE, Material.GOLD_PICKAXE, Material.DIAMOND_PICKAXE,
        Material.WOOD_AXE,     Material.STONE_AXE,     Material.IRON_AXE,     Material.GOLD_AXE,     Material.DIAMOND_AXE,
        Material.WOOD_SPADE,   Material.STONE_SPADE,   Material.IRON_SPADE,   Material.GOLD_SPADE,   Material.DIAMOND_SPADE,
        Material.WOOD_HOE,     Material.STONE_HOE,     Material.IRON_HOE,     Material.GOLD_HOE,     Material.DIAMOND_HOE,

        Material.SHEARS
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.TOOL;

    public override EnchantmentType getEnchantType() => EnchantmentType.DIG_SPEAD;

    public override int getMaxLevel() => 5;

    public override string getName() => "efficiency";

    public override int getStartLevel() => 1;
}
