using Minecraft.Server.FourKit.Inventory;

namespace Minecraft.Server.FourKit.Enchantments;

public class LootingEnchantment : Enchantment
{
    static readonly Material[] supportedItems = {
        Material.WOOD_PICKAXE, Material.STONE_PICKAXE, Material.IRON_PICKAXE, Material.GOLD_PICKAXE, Material.DIAMOND_PICKAXE,
        Material.WOOD_AXE,     Material.STONE_AXE,     Material.IRON_AXE,     Material.GOLD_AXE,     Material.DIAMOND_AXE,
    };

    static readonly EnchantmentType[] conflictedEnchants = { };

    public override bool canEnchantItem(ItemStack item) => supportedItems.Contains(item.getType());

    public override bool conflictsWith(Enchantment other) => conflictedEnchants.Contains(other.getEnchantType());

    public override EnchantmentTarget getItemTarget() => EnchantmentTarget.WEAPON;

    public override EnchantmentType getEnchantType() => EnchantmentType.LOOT_BONUS_MOBS;

    public override int getMaxLevel() => 3;

    public override string getName() => "looting";

    public override int getStartLevel() => 1;
}
