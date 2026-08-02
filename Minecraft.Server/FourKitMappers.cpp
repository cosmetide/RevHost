#include "FourKitBridge.h"
#include "stdafx.h"

#include "../Minecraft.World/DamageSource.h"
#include "../Minecraft.World/EntityDamageSource.h"

namespace FourKitBridge
{
int MapEntityType(int nativeType)
{
    eINSTANCEOF type = (eINSTANCEOF)nativeType;
    const int ARROW = 0, BAT = 1, BLAZE = 2, BOAT = 3, CAVE_SPIDER = 4;
    const int CHICKEN = 5, COW = 7, CREEPER = 8, DROPPED_ITEM = 9;
    const int EGG = 10, ENDER_CRYSTAL = 11, ENDER_DRAGON = 12;
    const int ENDER_PEARL = 13, ENDER_SIGNAL = 14, ENDERMAN = 15;
    const int EXPERIENCE_ORB = 16, FALLING_BLOCK = 17, FIREBALL = 18;
    const int FIREWORK = 19, FISHING_HOOK = 20, GHAST = 21, GIANT = 22;
    const int HORSE = 23, IRON_GOLEM = 24, ITEM_FRAME = 25;
    const int LEASH_HITCH = 26, LIGHTNING = 27, MAGMA_CUBE = 28;
    const int MINECART = 29, MINECART_CHEST = 30, MINECART_FURNACE = 32;
    const int MINECART_HOPPER = 33, MINECART_MOB_SPAWNER = 34;
    const int MINECART_TNT = 35, MUSHROOM_COW = 36, OCELOT = 37;
    const int PAINTING = 38, PIG = 39, PIG_ZOMBIE = 40, PLAYER = 41;
    const int PRIMED_TNT = 42, SHEEP = 43, SILVERFISH = 44;
    const int SKELETON = 45, SLIME = 46, SMALL_FIREBALL = 47;
    const int SNOWBALL = 48, SNOWMAN = 49, SPIDER = 50;
    const int SPLASH_POTION = 51, SQUID = 52, THROWN_EXP_BOTTLE = 53;
    const int UNKNOWN = 54, VILLAGER = 55, WITCH = 57;
    const int WITHER = 58, WITHER_SKULL = 59, WOLF = 60, ZOMBIE = 61;

    switch (type)
    {
    case eTYPE_ARROW:
        return ARROW;
    case eTYPE_BAT:
        return BAT;
    case eTYPE_BLAZE:
        return BLAZE;
    case eTYPE_BOAT:
        return BOAT;
    case eTYPE_CAVESPIDER:
        return CAVE_SPIDER;
    case eTYPE_CHICKEN:
        return CHICKEN;
    case eTYPE_COW:
        return COW;
    case eTYPE_CREEPER:
        return CREEPER;
    case eTYPE_ITEMENTITY:
        return DROPPED_ITEM;
    case eTYPE_THROWNEGG:
        return EGG;
    case eTYPE_NETHER_SPHERE:
        return ENDER_CRYSTAL;
    case eTYPE_ENDERDRAGON:
        return ENDER_DRAGON;
    case eTYPE_THROWNENDERPEARL:
        return ENDER_PEARL;
    case eTYPE_EYEOFENDERSIGNAL:
        return ENDER_SIGNAL;
    case eTYPE_ENDERMAN:
        return ENDERMAN;
    case eTYPE_EXPERIENCEORB:
        return EXPERIENCE_ORB;
    case eTYPE_FALLINGTILE:
        return FALLING_BLOCK;
    case eTYPE_LARGE_FIREBALL:
        return FIREBALL;
    case eTYPE_FIREWORKS_ROCKET:
        return FIREWORK;
    case eTYPE_FISHINGHOOK:
        return FISHING_HOOK;
    case eTYPE_GHAST:
        return GHAST;
    case eTYPE_GIANT:
        return GIANT;
    case eTYPE_HORSE:
        return HORSE;
    case eTYPE_VILLAGERGOLEM:
        return IRON_GOLEM;
    case eTYPE_ITEM_FRAME:
        return ITEM_FRAME;
    case eTYPE_LEASHFENCEKNOT:
        return LEASH_HITCH;
    case eTYPE_LIGHTNINGBOLT:
        return LIGHTNING;
    case eTYPE_LAVASLIME:
        return MAGMA_CUBE;
    case eTYPE_MINECART_RIDEABLE:
        return MINECART;
    case eTYPE_MINECART_CHEST:
        return MINECART_CHEST;
    case eTYPE_MINECART_FURNACE:
        return MINECART_FURNACE;
    case eTYPE_MINECART_HOPPER:
        return MINECART_HOPPER;
    case eTYPE_MINECART_SPAWNER:
        return MINECART_MOB_SPAWNER;
    case eTYPE_MINECART_TNT:
        return MINECART_TNT;
    case eTYPE_MUSHROOMCOW:
        return MUSHROOM_COW;
    case eTYPE_OCELOT:
        return OCELOT;
    case eTYPE_PAINTING:
        return PAINTING;
    case eTYPE_PIG:
        return PIG;
    case eTYPE_PIGZOMBIE:
        return PIG_ZOMBIE;
    case eTYPE_PLAYER:
        return PLAYER;
    case eTYPE_SERVERPLAYER:
        return PLAYER;
    case eTYPE_REMOTEPLAYER:
        return PLAYER;
    case eTYPE_LOCALPLAYER:
        return PLAYER;
    case eTYPE_PRIMEDTNT:
        return PRIMED_TNT;
    case eTYPE_SHEEP:
        return SHEEP;
    case eTYPE_SILVERFISH:
        return SILVERFISH;
    case eTYPE_SKELETON:
        return SKELETON;
    case eTYPE_SLIME:
        return SLIME;
    case eTYPE_SMALL_FIREBALL:
        return SMALL_FIREBALL;
    case eTYPE_SNOWBALL:
        return SNOWBALL;
    case eTYPE_SNOWMAN:
        return SNOWMAN;
    case eTYPE_SPIDER:
        return SPIDER;
    case eTYPE_THROWNPOTION:
        return SPLASH_POTION;
    case eTYPE_SQUID:
        return SQUID;
    case eTYPE_THROWNEXPBOTTLE:
        return THROWN_EXP_BOTTLE;
    case eTYPE_VILLAGER:
        return VILLAGER;
    case eTYPE_WITCH:
        return WITCH;
    case eTYPE_WITHERBOSS:
        return WITHER;
    case eTYPE_WITHER_SKULL:
        return WITHER_SKULL;
    case eTYPE_WOLF:
        return WOLF;
    case eTYPE_ZOMBIE:
        return ZOMBIE;
    default:
        return UNKNOWN;
    }
}

int MapDamageCause(void *sourcePtr)
{
    DamageSource *source = (DamageSource *)sourcePtr;
    const int CONTACT = 1, CUSTOM = 2, DROWNING = 3;
    const int ENTITY_ATTACK = 4, ENTITY_EXPLOSION = 5;
    const int FALL = 6, FALLING_BLOCK = 7, FIRE = 8, FIRE_TICK = 9;
    const int LAVA = 10, MAGIC = 12;
    const int PROJECTILE = 15, STARVATION = 16, SUFFOCATION = 17;
    const int CAUSE_VOID = 20, CAUSE_WITHER = 21;

    if (source == nullptr)
    {
        return CUSTOM;
    }
    if (source == DamageSource::inFire)
    {
        return FIRE;
    }
    if (source == DamageSource::onFire)
    {
        return FIRE_TICK;
    }
    if (source == DamageSource::lava)
    {
        return LAVA;
    }
    if (source == DamageSource::inWall)
    {
        return SUFFOCATION;
    }
    if (source == DamageSource::drown)
    {
        return DROWNING;
    }
    if (source == DamageSource::starve)
    {
        return STARVATION;
    }
    if (source == DamageSource::cactus)
    {
        return CONTACT;
    }
    if (source == DamageSource::fall)
    {
        return FALL;
    }
    if (source == DamageSource::outOfWorld)
    {
        return CAUSE_VOID;
    }
    if (source == DamageSource::genericSource)
    {
        return CUSTOM;
    }
    if (source == DamageSource::magic)
    {
        return MAGIC;
    }
    if (source == DamageSource::wither)
    {
        return CAUSE_WITHER;
    }
    if (source == DamageSource::anvil)
    {
        return FALLING_BLOCK;
    }
    if (source == DamageSource::fallingBlock)
    {
        return FALLING_BLOCK;
    }

    if (source->isExplosion())
    {
        return ENTITY_EXPLOSION;
    }
    if (source->isProjectile())
    {
        return PROJECTILE;
    }
    if (source->isMagic())
    {
        return MAGIC;
    }

    if (dynamic_cast<EntityDamageSource *>(source) != nullptr)
    {
        return ENTITY_ATTACK;
    }

    return CUSTOM;
}
}
