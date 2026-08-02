using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class RespirationEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.LEATHER_HELMET, Material.CHAINMAIL_HELMET, Material.GOLD_HELMET, Material.IRON_HELMET, Material.DIAMOND_HELMET,
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.ARMOR_HEAD;

    public override EnchantmentType getEnchantType() => EnchantmentType.OXYGEN;

    public override int getMaxLevel() => 3;

    public override string getName() => "respiration";

    public override int getStartLevel() => 1;
}
