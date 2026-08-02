using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class UnbreakingEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.WOOD_SWORD,   Material.STONE_SWORD,   Material.IRON_SWORD,   Material.GOLD_SWORD,   Material.DIAMOND_SWORD,
        Material.WOOD_PICKAXE, Material.STONE_PICKAXE, Material.IRON_PICKAXE, Material.GOLD_PICKAXE, Material.DIAMOND_PICKAXE,
        Material.WOOD_AXE,     Material.STONE_AXE,     Material.IRON_AXE,     Material.GOLD_AXE,     Material.DIAMOND_AXE,
        Material.WOOD_SPADE,   Material.STONE_SPADE,   Material.IRON_SPADE,   Material.GOLD_SPADE,   Material.DIAMOND_SPADE,
        Material.WOOD_HOE,     Material.STONE_HOE,     Material.IRON_HOE,     Material.GOLD_HOE,     Material.DIAMOND_HOE,

        Material.LEATHER_HELMET,   Material.LEATHER_CHESTPLATE,   Material.LEATHER_LEGGINGS,   Material.LEATHER_BOOTS,
        Material.CHAINMAIL_HELMET, Material.CHAINMAIL_CHESTPLATE, Material.CHAINMAIL_LEGGINGS, Material.CHAINMAIL_BOOTS,
        Material.GOLD_HELMET,      Material.GOLD_CHESTPLATE,      Material.GOLD_LEGGINGS,      Material.GOLD_BOOTS,
        Material.IRON_HELMET,      Material.IRON_CHESTPLATE,      Material.IRON_LEGGINGS,      Material.IRON_BOOTS,
        Material.DIAMOND_HELMET,   Material.DIAMOND_CHESTPLATE,   Material.DIAMOND_LEGGINGS,   Material.DIAMOND_BOOTS,

        Material.FISHING_ROD, Material.BOW,
        Material.SHEARS, Material.FLINT_AND_STEEL, Material.CARROT_STICK
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.ALL;

    public override EnchantmentType getEnchantType() => EnchantmentType.DURABILITY;

    public override int getMaxLevel() => 3;

    public override string getName() => "unbreaking";

    public override int getStartLevel() => 1;
}
