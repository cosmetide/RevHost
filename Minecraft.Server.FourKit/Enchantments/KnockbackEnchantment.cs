using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class KnockbackEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.WOOD_SWORD, Material.STONE_SWORD, Material.IRON_SWORD, Material.GOLD_SWORD, Material.DIAMOND_SWORD,
        Material.WOOD_AXE,   Material.STONE_AXE,   Material.IRON_AXE,   Material.GOLD_AXE,   Material.DIAMOND_AXE,
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.WEAPON;

    public override EnchantmentType getEnchantType() => EnchantmentType.KNOCKBACK;

    public override int getMaxLevel() => 2;

    public override string getName() => "knockback";

    public override int getStartLevel() => 1;
}
