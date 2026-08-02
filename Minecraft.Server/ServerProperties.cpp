#include "stdafx.h"

#include "ServerProperties.h"

#include "ServerLogger.h"
#include "Common//StringUtils.h"
#include "Common//FileUtils.h"
#include "..//Minecraft.World//ChunkSource.h"

#include <cctype>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>

namespace ServerRuntime
{
using StringUtils::ToLowerAscii;
using StringUtils::TrimAscii;
using StringUtils::StripUtf8Bom;
using StringUtils::Utf8ToWide;
using StringUtils::WideToUtf8;

struct ServerPropertyDefault
{
	const char *key;
	const char *value;
};

static const char *kServerPropertiesPath = "server.properties";

static const int kDefaultServerPort = 25565;
static const int kDefaultMaxPlayers = 16;
static const int kMaxDedicatedPlayers = 256;
static const int kDefaultAutosaveIntervalSeconds = 300; // 5 minutes
static const char *kLanAdvertisePropertyKey = "lan-advertise";

static const ServerPropertyDefault kServerPropertyDefaults[] =
{
	{ "allow-flight", "true" },
	{ "allow-nether", "true" },
	{ "difficulty", "1" },
	{ "disable-saving", "false" },
	{ "do-daylight-cycle", "true" },
	{ "do-mob-loot", "true" },
	{ "do-mob-spawning", "true" },
	{ "do-tile-drops", "true" },
	{ "fire-spreads", "true" },
	{ "generate-structures", "true" },
	{ "hardcore", "false" },
	{ "hardcore-ban-ip", "false" },
	{ "host-can-be-invisible", "true" },
	{ "host-can-change-hunger", "true" },
	{ "host-can-fly", "true" },
	{ "keep-inventory", "false" },
	{ "autosave-interval", "5m" },
	{ "autosave", "true" },
	{ "spawn-protection", "0" },
	{ "max-build-height", "256" },
	{ "max-players", "16" },
	{ "mob-griefing", "true" },
	{ "natural-regeneration", "true" },
	{ "pvp", "true" },
	{ "server-ip", "0.0.0.0" },
	{ "lan-server-name", "DedicatedServer" },
	{ "server-port", "25565" },
	{ "white-list", "false" },
	{ "lan-advertise", "false" },
	{ "spawn-animals", "true" },
	{ "spawn-monsters", "true" },
	{ "spawn-npcs", "true" },
	{ "tnt", "true" },
	{ "trust-players", "true" },
	{ "hide-player-list-prelogin", "true" },
	{ "rate-limit-connections-per-window", "5" },
	{ "rate-limit-window-seconds", "30" },
	{ "max-pending-connections", "10" },
	{ "require-challenge-token", "false" },
	{ "enable-stream-cipher", "true" },
	{ "require-secure-client", "true" },
	{ "proxy-protocol", "false" }
};

static std::string BoolToString(bool value)
{
	return value ? "true" : "false";
}

static std::string IntToString(int value)
{
	char buffer[32] = {};
	sprintf_s(buffer, sizeof(buffer), "%d", value);
	return std::string(buffer);
}

static int ClampInt(int value, int minValue, int maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	if (value > maxValue)
	{
		return maxValue;
	}
	return value;
}

static bool TryParseBool(const std::string &value, bool *outValue)
{
	if (outValue == NULL)
	{
		return false;
	}

	std::string lowered = ToLowerAscii(TrimAscii(value));
	if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on")
	{
		*outValue = true;
		return true;
	}
	if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off")
	{
		*outValue = false;
		return true;
	}
	return false;
}

static bool TryParseInt(const std::string &value, int *outValue)
{
	if (outValue == NULL)
	{
		return false;
	}

	std::string trimmed = TrimAscii(value);
	if (trimmed.empty())
	{
		return false;
	}

	char *end = NULL;
	long parsed = strtol(trimmed.c_str(), &end, 10);
	if (end == trimmed.c_str() || *end != 0)
	{
		return false;
	}

	*outValue = (int)parsed;
	return true;
}

static void ApplyDefaultServerProperties(std::unordered_map<std::string, std::string> *properties)
{
	if (properties == NULL)
	{
		return;
	}

	const size_t defaultCount = sizeof(kServerPropertyDefaults) / sizeof(kServerPropertyDefaults[0]);
	for (size_t i = 0; i < defaultCount; ++i)
	{
		(*properties)[kServerPropertyDefaults[i].key] = kServerPropertyDefaults[i].value;
	}
}

/**
 * **Parse server.properties Text**
 *
 * Extracts key/value pairs from `server.properties` format text
 * - Ignores lines starting with `#` or `!` as comments
 * - Accepts `=` or `:` as separators
 * - Skips invalid lines and continues
 * server.propertiesのパース処理
 */
static bool ReadServerPropertiesFile(const char *filePath, std::unordered_map<std::string, std::string> *properties, int *outParsedCount)
{
	if (properties == NULL)
	{
		return false;
	}

	std::string text;
	if (filePath == NULL || !FileUtils::ReadTextFile(filePath, &text))
	{
		return false;
	}

	text = StripUtf8Bom(text);

	int parsedCount = 0;
	for (size_t start = 0; start <= text.length();)
	{
		size_t end = text.find_first_of("\r\n", start);
		size_t nextStart = text.length() + 1;
		if (end != std::string::npos)
		{
			nextStart = end + 1;
			if (text[end] == '\r' && nextStart < text.length() && text[nextStart] == '\n')
			{
				++nextStart;
			}
		}

		std::string line;
		if (end == std::string::npos)
		{
			line = text.substr(start);
		}
		else
		{
			line = text.substr(start, end - start);
		}

		std::string trimmedLine = TrimAscii(line);
		if (trimmedLine.empty())
		{
			start = nextStart;
			continue;
		}

		if (trimmedLine[0] == '#' || trimmedLine[0] == '!')
		{
			start = nextStart;
			continue;
		}

		size_t eqPos = trimmedLine.find('=');
		size_t colonPos = trimmedLine.find(':');
		size_t sepPos = std::string::npos;
		if (eqPos == std::string::npos)
		{
			sepPos = colonPos;
		}
		else if (colonPos == std::string::npos)
		{
			sepPos = eqPos;
		}
		else
		{
			sepPos = (eqPos < colonPos) ? eqPos : colonPos;
		}

		if (sepPos == std::string::npos)
		{
			start = nextStart;
			continue;
		}

		std::string key = TrimAscii(trimmedLine.substr(0, sepPos));
		if (key.empty())
		{
			start = nextStart;
			continue;
		}

		std::string value = TrimAscii(trimmedLine.substr(sepPos + 1));
		(*properties)[key] = value;
		++parsedCount;
		start = nextStart;
	}

	if (outParsedCount != NULL)
	{
		*outParsedCount = parsedCount;
	}

	return true;
}

/**
 * **Write server.properties Text**
 *
 * Writes key/value data back as `server.properties`
 * Sorts keys before writing to keep output order stable
 * server.propertiesの書き戻し処理
 */
static bool WriteServerPropertiesFile(const char *filePath, const std::unordered_map<std::string, std::string> &properties)
{
	if (filePath == NULL)
	{
		return false;
	}

	std::string text;
	text += "# Minecraft server properties\n";
	text += "# Auto-generated and normalized when missing\n";

	std::map<std::string, std::string> sortedProperties(properties.begin(), properties.end());
	for (std::map<std::string, std::string>::const_iterator it = sortedProperties.begin(); it != sortedProperties.end(); ++it)
	{
		text += it->first;
		text += "=";
		text += it->second;
		text += "\n";
	}

	return FileUtils::WriteTextFileAtomic(filePath, text);
}

static bool ReadNormalizedBoolProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	bool defaultValue,
	bool *shouldWrite)
{
	std::string raw = TrimAscii((*properties)[key]);
	bool value = defaultValue;
	if (!TryParseBool(raw, &value))
	{
		value = defaultValue;
	}

	std::string normalized = BoolToString(value);
	if (raw != normalized)
	{
		(*properties)[key] = normalized;
		if (shouldWrite != NULL)
		{
			*shouldWrite = true;
		}
	}

	return value;
}

static int ReadNormalizedIntProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	int defaultValue,
	int minValue,
	int maxValue,
	bool *shouldWrite)
{
	std::string raw = TrimAscii((*properties)[key]);
	int value = defaultValue;
	if (!TryParseInt(raw, &value))
	{
		value = defaultValue;
	}
	value = ClampInt(value, minValue, maxValue);

	std::string normalized = IntToString(value);
	if (raw != normalized)
	{
		(*properties)[key] = normalized;
		if (shouldWrite != NULL)
		{
			*shouldWrite = true;
		}
	}

	return value;
}

static int ReadIntervalProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	int defaultSeconds,
	int minSeconds,
	int maxSeconds,
	bool *shouldWrite)
{
	std::string raw = TrimAscii((*properties)[key]);
	int seconds = defaultSeconds;

	if (!raw.empty())
	{
		char suffix = 0;
		std::string numStr = raw;

		char last = numStr[numStr.size() - 1];
		if (last == 's' || last == 'S' || last == 'm' || last == 'M' || last == 'h' || last == 'H')
		{
			suffix = (char)tolower(last);
			numStr = numStr.substr(0, numStr.size() - 1);
		}

		int value = 0;
		if (TryParseInt(numStr, &value))
		{
			if (suffix == 'm')
				seconds = value * 60;
			else if (suffix == 'h')
				seconds = value * 3600;
			else
				seconds = value;
		}
	}

	seconds = ClampInt(seconds, minSeconds, maxSeconds);

	int hours = seconds / 3600;
	int minutes = (seconds % 3600) / 60;
	int remainder = seconds % 60;
	std::string normalized;
	if (remainder == 0 && minutes == 0 && hours >= 1)
		normalized = IntToString(hours) + "h";
	else if (remainder == 0 && hours == 0 && minutes >= 1)
		normalized = IntToString(minutes) + "m";
	else
		normalized = IntToString(seconds) + "s";

	if (raw != normalized)
	{
		(*properties)[key] = normalized;
		if (shouldWrite != NULL)
			*shouldWrite = true;
	}

	return seconds;
}

static std::string ReadNormalizedStringProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	const std::string &defaultValue,
	size_t maxLength,
	bool *shouldWrite)
{
	std::string value = TrimAscii((*properties)[key]);
	if (value.empty())
	{
		value = defaultValue;
	}
	if (maxLength > 0 && value.length() > maxLength)
	{
		value.resize(maxLength);
	}

	if (value != (*properties)[key])
	{
		(*properties)[key] = value;
		if (shouldWrite != NULL)
		{
			*shouldWrite = true;
		}
	}

	return value;
}

static std::string ReadNormalizedLevelTypeProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	bool *outIsFlat,
	bool *shouldWrite)
{
	std::string raw = TrimAscii((*properties)[key]);
	std::string lowered = ToLowerAscii(raw);

	bool isFlat = false;
	std::string normalized = "default";
	if (lowered == "flat" || lowered == "superflat" || lowered == "1")
	{
		isFlat = true;
		normalized = "flat";
	}
	else if (lowered == "default" || lowered == "normal" || lowered == "0")
	{
		isFlat = false;
		normalized = "default";
	}

	if (raw != normalized)
	{
		(*properties)[key] = normalized;
		if (shouldWrite != NULL)
		{
			*shouldWrite = true;
		}
	}

	if (outIsFlat != NULL)
	{
		*outIsFlat = isFlat;
	}

	return normalized;
}

static std::string WorldSizeToPropertyValue(int worldSize)
{
	switch (worldSize)
	{
	case e_worldSize_Small:
		return "small";
	case e_worldSize_Medium:
		return "medium";
	case e_worldSize_Large:
		return "large";
	case e_worldSize_Classic:
	default:
		return "classic";
	}
}

static int WorldSizeToXzChunks(int worldSize)
{
	switch (worldSize)
	{
	case e_worldSize_Small:
		return LEVEL_WIDTH_SMALL;
	case e_worldSize_Medium:
		return LEVEL_WIDTH_MEDIUM;
	case e_worldSize_Large:
		return LEVEL_WIDTH_LARGE;
	case e_worldSize_Classic:
	default:
		return LEVEL_WIDTH_CLASSIC;
	}
}

static int WorldSizeToHellScale(int worldSize)
{
	switch (worldSize)
	{
	case e_worldSize_Small:
		return HELL_LEVEL_SCALE_SMALL;
	case e_worldSize_Medium:
		return HELL_LEVEL_SCALE_MEDIUM;
	case e_worldSize_Large:
		return HELL_LEVEL_SCALE_LARGE;
	case e_worldSize_Classic:
	default:
		return HELL_LEVEL_SCALE_CLASSIC;
	}
}

static bool TryParseWorldSize(const std::string &lowered, int *outWorldSize)
{
	if (outWorldSize == NULL)
	{
		return false;
	}

	if (lowered == "classic" || lowered == "54" || lowered == "1")
	{
		*outWorldSize = e_worldSize_Classic;
		return true;
	}
	if (lowered == "small" || lowered == "64" || lowered == "2")
	{
		*outWorldSize = e_worldSize_Small;
		return true;
	}
	if (lowered == "medium" || lowered == "192" || lowered == "3")
	{
		*outWorldSize = e_worldSize_Medium;
		return true;
	}
	if (lowered == "large" || lowered == "320" || lowered == "4")
	{
		*outWorldSize = e_worldSize_Large;
		return true;
	}

	return false;
}

static int ReadNormalizedWorldSizeProperty(
	std::unordered_map<std::string, std::string> *properties,
	const char *key,
	int defaultWorldSize,
	int *outXzChunks,
	int *outHellScale,
	bool *shouldWrite)
{
	std::string raw = TrimAscii((*properties)[key]);
	std::string lowered = ToLowerAscii(raw);

	int worldSize = defaultWorldSize;
	if (!raw.empty())
	{
		int parsedWorldSize = defaultWorldSize;
		if (TryParseWorldSize(lowered, &parsedWorldSize))
		{
			worldSize = parsedWorldSize;
		}
	}

	std::string normalized = WorldSizeToPropertyValue(worldSize);
	if (raw != normalized)
	{
		(*properties)[key] = normalized;
		if (shouldWrite != NULL)
		{
			*shouldWrite = true;
		}
	}

	if (outXzChunks != NULL)
	{
		*outXzChunks = WorldSizeToXzChunks(worldSize);
	}
	if (outHellScale != NULL)
	{
		*outHellScale = WorldSizeToHellScale(worldSize);
	}

	return worldSize;
}

/**
 * **Load Effective Server Properties Config**
 *
 * Loads effective world settings, repairs missing or invalid values, and returns normalized config
 * - Creates defaults when file is missing
 * - Fills required keys when absent
 * - Auto-saves when any fix is applied
 * 実効設定の読み込みと補正処理
 */
ServerPropertiesConfig LoadServerPropertiesConfig()
{
	ServerPropertiesConfig config;

	std::unordered_map<std::string, std::string> defaults;
	std::unordered_map<std::string, std::string> loaded;
	ApplyDefaultServerProperties(&defaults);

	int parsedCount = 0;
	bool readSuccess = ReadServerPropertiesFile(kServerPropertiesPath, &loaded, &parsedCount);
	std::unordered_map<std::string, std::string> merged = defaults;
	bool shouldWrite = false;

	if (!readSuccess)
	{
		LogWorldIO("server.properties not found or unreadable; creating defaults");
		shouldWrite = true;
	}
	else
	{
		if (parsedCount == 0)
		{
			LogWorldIO("server.properties has no properties; applying defaults");
			shouldWrite = true;
		}

		const size_t defaultCount = sizeof(kServerPropertyDefaults) / sizeof(kServerPropertyDefaults[0]);
		for (size_t i = 0; i < defaultCount; ++i)
		{
			if (loaded.find(kServerPropertyDefaults[i].key) == loaded.end())
			{
				shouldWrite = true;
				break;
			}
		}
	}

	for (std::unordered_map<std::string, std::string>::const_iterator it = loaded.begin(); it != loaded.end(); ++it)
	{
		// Merge loaded values over defaults and keep unknown keys whenever possible
		merged[it->first] = it->second;
	}

	// Removed properties are no longer written out; drop any stale entries from old files
	// and force a rewrite so they disappear from disk.
	static const char *const kRemovedKeys[] = { "level-id", "level-seed", "override-seed", "log-level", "server-name", "level-name" };
	for (const char *key : kRemovedKeys)
	{
		if (merged.count(key) != 0)
		{
			merged.erase(key);
			shouldWrite = true;
		}
	}

	config.serverPort = ReadNormalizedIntProperty(&merged, "server-port", kDefaultServerPort, 1, 65535, &shouldWrite);
	config.serverIp = ReadNormalizedStringProperty(&merged, "server-ip", "0.0.0.0", 255, &shouldWrite);
	config.lanAdvertise = ReadNormalizedBoolProperty(&merged, kLanAdvertisePropertyKey, false, &shouldWrite);
	config.whiteListEnabled = ReadNormalizedBoolProperty(&merged, "white-list", false, &shouldWrite);
	config.lanServerName = ReadNormalizedStringProperty(&merged, "lan-server-name", "DedicatedServer", 16, &shouldWrite);
	config.maxPlayers = ReadNormalizedIntProperty(&merged, "max-players", kDefaultMaxPlayers, 1, kMaxDedicatedPlayers, &shouldWrite);
	config.autosaveIntervalSeconds = ReadIntervalProperty(&merged, "autosave-interval", 300, 1, 86400, &shouldWrite);
	config.autosave = ReadNormalizedBoolProperty(&merged, "autosave", true, &shouldWrite);

	config.difficulty = ReadNormalizedIntProperty(&merged, "difficulty", 1, 0, 3, &shouldWrite);
	config.spawnProtectionRadius = ReadNormalizedIntProperty(&merged, "spawn-protection", 0, 0, 256, &shouldWrite);
	config.generateStructures = ReadNormalizedBoolProperty(&merged, "generate-structures", true, &shouldWrite);
	config.pvp = ReadNormalizedBoolProperty(&merged, "pvp", true, &shouldWrite);
	config.trustPlayers = ReadNormalizedBoolProperty(&merged, "trust-players", true, &shouldWrite);
	config.fireSpreads = ReadNormalizedBoolProperty(&merged, "fire-spreads", true, &shouldWrite);
	config.tnt = ReadNormalizedBoolProperty(&merged, "tnt", true, &shouldWrite);
	config.spawnAnimals = ReadNormalizedBoolProperty(&merged, "spawn-animals", true, &shouldWrite);
	config.spawnNpcs = ReadNormalizedBoolProperty(&merged, "spawn-npcs", true, &shouldWrite);
	config.spawnMonsters = ReadNormalizedBoolProperty(&merged, "spawn-monsters", true, &shouldWrite);
	config.allowFlight = ReadNormalizedBoolProperty(&merged, "allow-flight", true, &shouldWrite);
	config.allowNether = ReadNormalizedBoolProperty(&merged, "allow-nether", true, &shouldWrite);
	// RevHost: host privileges (cheats) are always ON by default and never disabled by config.
	// The keys still parse and write fine, but any "false" in an existing file is normalized to "true".
	config.hostCanFly = ReadNormalizedBoolProperty(&merged, "host-can-fly", true, &shouldWrite);
	config.hostCanChangeHunger = ReadNormalizedBoolProperty(&merged, "host-can-change-hunger", true, &shouldWrite);
	config.hostCanBeInvisible = ReadNormalizedBoolProperty(&merged, "host-can-be-invisible", true, &shouldWrite);
	if (config.hostCanFly != true || config.hostCanChangeHunger != true || config.hostCanBeInvisible != true)
	{
		config.hostCanFly = true;
		config.hostCanChangeHunger = true;
		config.hostCanBeInvisible = true;
		merged["host-can-fly"] = "true";
		merged["host-can-change-hunger"] = "true";
		merged["host-can-be-invisible"] = "true";
		shouldWrite = true;
	}
	config.disableSaving = ReadNormalizedBoolProperty(&merged, "disable-saving", false, &shouldWrite);
	config.mobGriefing = ReadNormalizedBoolProperty(&merged, "mob-griefing", true, &shouldWrite);
	config.keepInventory = ReadNormalizedBoolProperty(&merged, "keep-inventory", false, &shouldWrite);
	config.doMobSpawning = ReadNormalizedBoolProperty(&merged, "do-mob-spawning", true, &shouldWrite);
	config.doMobLoot = ReadNormalizedBoolProperty(&merged, "do-mob-loot", true, &shouldWrite);
	config.doTileDrops = ReadNormalizedBoolProperty(&merged, "do-tile-drops", true, &shouldWrite);
	config.naturalRegeneration = ReadNormalizedBoolProperty(&merged, "natural-regeneration", true, &shouldWrite);
	config.doDaylightCycle = ReadNormalizedBoolProperty(&merged, "do-daylight-cycle", true, &shouldWrite);
	config.hardcore = ReadNormalizedBoolProperty(&merged, "hardcore", false, &shouldWrite);
	config.hardcoreBanIp = ReadNormalizedBoolProperty(&merged, "hardcore-ban-ip", false, &shouldWrite);

	config.maxMonsters = ReadNormalizedIntProperty(&merged, "max-monsters", 50, 0, 1000, &shouldWrite);
	config.maxAnimals = ReadNormalizedIntProperty(&merged, "max-animals", 50, 0, 1000, &shouldWrite);
	config.maxAmbient = ReadNormalizedIntProperty(&merged, "max-ambient", 20, 0, 1000, &shouldWrite);
	config.maxWaterAnimals = ReadNormalizedIntProperty(&merged, "max-water-animals", 5, 0, 1000, &shouldWrite);
	config.maxWolves = ReadNormalizedIntProperty(&merged, "max-wolves", 8, 0, 1000, &shouldWrite);
	config.maxChickens = ReadNormalizedIntProperty(&merged, "max-chickens", 8, 0, 1000, &shouldWrite);
	config.maxMushroomCows = ReadNormalizedIntProperty(&merged, "max-mushroom-cows", 2, 0, 1000, &shouldWrite);

	config.maxBuildHeight = ReadNormalizedIntProperty(&merged, "max-build-height", 256, 64, 256, &shouldWrite);

	config.hidePlayerListPreLogin = ReadNormalizedBoolProperty(&merged, "hide-player-list-prelogin", true, &shouldWrite);
	config.rateLimitConnectionsPerWindow = ReadNormalizedIntProperty(&merged, "rate-limit-connections-per-window", 5, 1, 100, &shouldWrite);
	config.rateLimitWindowSeconds = ReadNormalizedIntProperty(&merged, "rate-limit-window-seconds", 30, 5, 300, &shouldWrite);
	config.maxPendingConnections = ReadNormalizedIntProperty(&merged, "max-pending-connections", 10, 1, 50, &shouldWrite);
	config.requireChallengeToken = ReadNormalizedBoolProperty(&merged, "require-challenge-token", false, &shouldWrite);
	config.enableStreamCipher = ReadNormalizedBoolProperty(&merged, "enable-stream-cipher", true, &shouldWrite);
	config.requireSecureClient = ReadNormalizedBoolProperty(&merged, "require-secure-client", true, &shouldWrite);
	config.proxyProtocol = ReadNormalizedBoolProperty(&merged, "proxy-protocol", false, &shouldWrite);

	if (shouldWrite)
	{
		if (WriteServerPropertiesFile(kServerPropertiesPath, merged))
		{
			LogWorldIO("wrote server.properties");
		}
		else
		{
			LogWorldIO("failed to write server.properties");
		}
	}

	return config;
}

/**
 * **Save World Identity While Preserving Other Keys**
 *
 * Saves settings while preserving as many other keys as possible
 * - Reads existing file and merges including unknown keys
 * - Updates `white-list` before writing back; `level-name` is dropped (world name lives in the .ms)
 * 設定の保存処理
 */
bool SaveServerPropertiesConfig(const ServerPropertiesConfig &config)
{
	std::unordered_map<std::string, std::string> merged;
	ApplyDefaultServerProperties(&merged);

	std::unordered_map<std::string, std::string> loaded;
	int parsedCount = 0;
	if (ReadServerPropertiesFile(kServerPropertiesPath, &loaded, &parsedCount))
	{
		for (std::unordered_map<std::string, std::string>::const_iterator it = loaded.begin(); it != loaded.end(); ++it)
		{
			// Keep existing content so keys untouched by caller are not dropped
			merged[it->first] = it->second;
		}
	}

	merged.erase("level-id");
	merged.erase("level-seed");
	merged.erase("override-seed");
	merged.erase("log-level");
	merged.erase("server-name");
	merged.erase("level-name");
	merged["white-list"] = BoolToString(config.whiteListEnabled);

	return WriteServerPropertiesFile(kServerPropertiesPath, merged);
}
}

