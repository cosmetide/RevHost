using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class PunchEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.BOW
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.BOW;

    public override EnchantmentType getEnchantType() => EnchantmentType.ARROW_KNOCKBACK;

    public override int getMaxLevel() => 2;

    public override string getName() => "punch";

    public override int getStartLevel() => 1;
}
