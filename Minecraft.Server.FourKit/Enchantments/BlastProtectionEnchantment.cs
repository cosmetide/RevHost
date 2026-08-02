using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class BlastProtectionEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.LEATHER_HELMET,   Material.LEATHER_CHESTPLATE,   Material.LEATHER_LEGGINGS,   Material.LEATHER_BOOTS,
        Material.CHAINMAIL_HELMET, Material.CHAINMAIL_CHESTPLATE, Material.CHAINMAIL_LEGGINGS, Material.CHAINMAIL_BOOTS,
        Material.GOLD_HELMET,      Material.GOLD_CHESTPLATE,      Material.GOLD_LEGGINGS,      Material.GOLD_BOOTS,
        Material.IRON_HELMET,      Material.IRON_CHESTPLATE,      Material.IRON_LEGGINGS,      Material.IRON_BOOTS,
        Material.DIAMOND_HELMET,   Material.DIAMOND_CHESTPLATE,   Material.DIAMOND_LEGGINGS,   Material.DIAMOND_BOOTS,
    };

    static readonly EnchantmentType[] conflictedEnchants = {
        EnchantmentType.PROTECTION_ENVIRONMENTAL,
        EnchantmentType.PROTECTION_FIRE,
        EnchantmentType.PROTECTION_PROJECTILE,
    };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.ARMOR;

    public override EnchantmentType getEnchantType() => EnchantmentType.PROTECTION_EXPLOSIVE;

    public override int getMaxLevel() => 4;

    public override string getName() => "blastprotection";

    public override int getStartLevel() => 1;
}
