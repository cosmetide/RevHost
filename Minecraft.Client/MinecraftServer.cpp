#include "stdafx.h"
//#include "Minecraft.h"

#include <ctime>

#include "ConsoleInput.h"
#include "DerivedServerLevel.h"
#include "DispenserBootstrap.h"
#include "EntityTracker.h"
#include "MinecraftServer.h"
#include "Options.h"
#include "PlayerList.h"
#include "ServerChunkCache.h"
#include "ServerConnection.h"
#include "ServerLevel.h"
#include "ServerLevelListener.h"
#include "Settings.h"
#include "../Minecraft.World/Command.h"
#include "../Minecraft.World/AABB.h"
#include "../Minecraft.World/Vec3.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/net.minecraft.world.level.dimension.h"
#include "../Minecraft.World/net.minecraft.world.level.storage.h"
#include "../Minecraft.World/net.minecraft.world.h"
#include "../Minecraft.World/net.minecraft.world.level.h"
#include "../Minecraft.World/net.minecraft.world.level.tile.h"
#include "../Minecraft.World/Pos.h"
#include "../Minecraft.World/System.h"
#include "../Minecraft.World/StringHelpers.h"
#include "../Minecraft.World/net.minecraft.world.entity.item.h"
#include "../Minecraft.World/net.minecraft.world.item.h"
#include "../Minecraft.World/net.minecraft.world.item.enchantment.h"
#include "../Minecraft.World/net.minecraft.world.damagesource.h"
#ifdef _WINDOWS64
#include "Windows64/Network/WinsockNetLayer.h"
#endif
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
#include "../Minecraft.Server/ServerLogger.h"
#include "../Minecraft.Server/ServerLogManager.h"
#include "../Minecraft.Server/FourKitBridge.h"
#include "../Minecraft.Server/Access/Access.h"
#include "../Minecraft.Server/Common/StringUtils.h"
#include "../Minecraft.Server/ServerProperties.h"
#include "../Minecraft.Server/Security/SecurityConfig.h"
#endif
#include <sstream>
#ifdef SPLIT_SAVES
#include "../Minecraft.World/ConsoleSaveFileSplit.h"
#endif
#include "../Minecraft.World/ConsoleSaveFileOriginal.h"
#include "../Minecraft.World/Socket.h"
#include "../Minecraft.World/net.minecraft.world.entity.h"
#include "../Minecraft.World/MobCategory.h"
#include "ProgressRenderer.h"
#include "ServerPlayer.h"
#include "GameRenderer.h"
#include "../Minecraft.World/ThreadName.h"
#include "../Minecraft.World/IntCache.h"
#include "../Minecraft.World/CompressedTileStorage.h"
#include "../Minecraft.World/SparseLightStorage.h"
#include "../Minecraft.World/SparseDataStorage.h"
#include "../Minecraft.World/compression.h"
#ifdef _XBOX
#include "Common/XUI/XUI_DebugSetCamera.h"
#endif
#include "PS3/PS3Extras/ShutdownManager.h"
#include "ServerCommandDispatcher.h"
#include "../Minecraft.World/BiomeSource.h"
#include "PlayerChunkMap.h"
#include "Common/Telemetry/TelemetryManager.h"
#include "PlayerConnection.h"
#ifdef _XBOX_ONE
#include "Durango/Network/NetworkPlayerDurango.h"
#endif

#define DEBUG_SERVER_DONT_SPAWN_MOBS 0

//4J Added
MinecraftServer *MinecraftServer::server = nullptr;
bool MinecraftServer::setTimeAtEndOfTick = false;
int64_t MinecraftServer::setTime = 0;
bool MinecraftServer::setTimeOfDayAtEndOfTick = false;
int64_t MinecraftServer::setTimeOfDay = 0;
bool	MinecraftServer::m_bPrimaryPlayerSignedOut=false;
bool	MinecraftServer::s_bServerHalted=false;
bool	MinecraftServer::s_bSaveOnExitAnswered=false;
bool	MinecraftServer::s_bExitAfterStop=false;
#ifdef _ACK_CHUNK_SEND_THROTTLING
bool MinecraftServer::s_hasSentEnoughPackets = false;
int64_t MinecraftServer::s_tickStartTime = 0;
vector<INetworkPlayer *> MinecraftServer::s_sentTo;
#else
int MinecraftServer::s_slowQueuePlayerIndex = 0;
int MinecraftServer::s_slowQueueLastTime = 0;
bool MinecraftServer::s_slowQueuePacketSent = false;
#ifdef MINECRAFT_SERVER_BUILD
int MinecraftServer::s_dedicatedChunkSendsThisTick = 0;
#endif
#endif

unordered_map<wstring, int> MinecraftServer::ironTimers;

static bool ShouldUseDedicatedServerProperties()
{
#ifdef _WINDOWS64
	return g_Win64DedicatedServer;
#else
	return false;
#endif
}

static int GetDedicatedServerInt(Settings *settings, const wchar_t *key, int defaultValue)
{
	return (ShouldUseDedicatedServerProperties() && settings != nullptr) ? settings->getInt(key, defaultValue) : defaultValue;
}

static bool GetDedicatedServerBool(Settings *settings, const wchar_t *key, bool defaultValue)
{
	return (ShouldUseDedicatedServerProperties() && settings != nullptr) ? settings->getBoolean(key, defaultValue) : defaultValue;
}

static wstring GetDedicatedServerString(Settings *settings, const wchar_t *key, const wstring &defaultValue)
{
	return (ShouldUseDedicatedServerProperties() && settings != nullptr) ? settings->getString(key, defaultValue) : defaultValue;
}

static CRITICAL_SECTION s_consoleOutputCS;
extern HANDLE GetConsoleOutputHandle();

extern volatile LONG g_consolePromptVisible;
extern wchar_t g_consoleInputBuf[];
extern int g_consoleInputLen;

static void WritePromptRestoreInput(HANDLE hOut)
{
	DWORD written;
	SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	WriteConsoleW(hOut, L"server> ", 8, &written, nullptr);
	SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	if (g_consoleInputLen > 0)
		WriteConsoleW(hOut, g_consoleInputBuf, (DWORD)g_consoleInputLen, &written, nullptr);
}

static void PrintConsoleLine(const wchar_t *prefix, const wstring &message)
{
	HANDLE hOut = GetConsoleOutputHandle();
	if (!hOut || hOut == INVALID_HANDLE_VALUE) return;

	EnterCriticalSection(&s_consoleOutputCS);

	DWORD written;
	BOOL promptWasVisible = (g_consolePromptVisible != 0);

	if (promptWasVisible)
		WriteConsoleW(hOut, L"\r", 1, &written, nullptr);

	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		wchar_t timestamp[64];
		swprintf_s(timestamp, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d]%ls[server]",
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
			prefix);
		SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		wchar_t line[1024];
		int len = swprintf_s(line, L"%ls %ls\r\n", timestamp, message.c_str());
		if (len > 0) WriteConsoleW(hOut, line, (DWORD)len, &written, nullptr);
	}

	if (promptWasVisible)
		WritePromptRestoreInput(hOut);

	LeaveCriticalSection(&s_consoleOutputCS);
}

void ConsoleLog(const wchar_t *message, bool showServer)
{
	HANDLE hOut = GetConsoleOutputHandle();
	if (!hOut || hOut == INVALID_HANDLE_VALUE) return;

	EnterCriticalSection(&s_consoleOutputCS);

	DWORD written;
	BOOL promptWasVisible = (g_consolePromptVisible != 0);

	if (promptWasVisible)
		WriteConsoleW(hOut, L"\r", 1, &written, nullptr);

	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		wchar_t timestamp[64];
		if (showServer)
			swprintf_s(timestamp, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d][INFO][server]", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		else
			swprintf_s(timestamp, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d][INFO]", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		wchar_t line[1024];
		int len = swprintf_s(line, L"%ls %ls\r\n", timestamp, message);
		if (len > 0) WriteConsoleW(hOut, line, (DWORD)len, &written, nullptr);
	}

	if (promptWasVisible)
		WritePromptRestoreInput(hOut);

	LeaveCriticalSection(&s_consoleOutputCS);
}

void ConsoleProgressBar(int percent)
{
	HANDLE hOut = GetConsoleOutputHandle();
	if (!hOut || hOut == INVALID_HANDLE_VALUE) return;

	EnterCriticalSection(&s_consoleOutputCS);

	int barWidth = 20;
	int filled = (percent * barWidth) / 100;
	wchar_t bar[64];
	int pos = 0;
	bar[pos++] = L'[';
	for (int i = 0; i < barWidth; i++)
		bar[pos++] = (i < filled) ? L'#' : L' ';
	bar[pos++] = L']';
	bar[pos++] = L' ';
	if (percent < 10) { bar[pos++] = L'0' + percent; }
	else if (percent < 100) { bar[pos++] = L'0' + percent / 10; bar[pos++] = L'0' + percent % 10; }
	else { bar[pos++] = L'1'; bar[pos++] = L'0'; bar[pos++] = L'0'; }
	bar[pos++] = L'%';
	bar[pos++] = L'\r';
	bar[pos] = L'\0';

	DWORD written;
	WriteConsoleW(hOut, bar, pos, &written, nullptr);

	LeaveCriticalSection(&s_consoleOutputCS);
}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
static bool s_saveInProgress = false;
static DWORD s_saveStartTick = 0;
static DWORD s_saveLastProgressTick = 0;
#endif

void ConsoleSaveProgressLog(const wchar_t *stage, int percent)
{
	wchar_t bar[64];
	int barWidth = 20;
	int filled = (percent * barWidth) / 100;
	int pos = 0;
	bar[pos++] = L'[';
	for (int i = 0; i < barWidth; i++)
		bar[pos++] = (i < filled) ? L'#' : L' ';
	bar[pos++] = L']';
	bar[pos++] = L' ';
	if (percent < 10) { bar[pos++] = L'0' + percent; }
	else if (percent < 100) { bar[pos++] = L'0' + percent / 10; bar[pos++] = L'0' + percent % 10; }
	else { bar[pos++] = L'1'; bar[pos++] = L'0'; bar[pos++] = L'0'; }
	bar[pos++] = L'%';
	bar[pos] = L'\0';

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Multi-line progress display: each stage prints its own line. tick()
	// reports "Still saving..." while the background write is in flight.
	if (!s_saveInProgress)
	{
		s_saveInProgress = true;
		s_saveStartTick = GetTickCount();
		s_saveLastProgressTick = GetTickCount();
	}
#endif

	wchar_t msg[1024];
	swprintf_s(msg, L"Save: %ls %ls", stage, bar);
	ConsoleLog(msg);
}

void LockConsoleOutput()
{
	EnterCriticalSection(&s_consoleOutputCS);
}

void UnlockConsoleOutput()
{
	LeaveCriticalSection(&s_consoleOutputCS);
}

static bool TryParseIntValue(const wstring &text, int &value)
{
	std::wistringstream stream(text);
	stream >> value;
	return !stream.fail() && stream.eof();
}

static vector<wstring> SplitConsoleCommand(const wstring &command)
{
	vector<wstring> tokens;
	std::wistringstream stream(command);
	wstring token;
	while (stream >> token)
	{
		tokens.push_back(token);
	}
	return tokens;
}

static wstring JoinConsoleCommandTokens(const vector<wstring> &tokens, size_t startIndex)
{
	wstring joined;
	for (size_t i = startIndex; i < tokens.size(); ++i)
	{
		if (!joined.empty()) joined += L" ";
		joined += tokens[i];
	}
	return joined;
}

static shared_ptr<ServerPlayer> FindPlayerByName(PlayerList *playerList, const wstring &name)
{
	if (playerList == nullptr) return nullptr;

	for (size_t i = 0; i < playerList->players.size(); ++i)
	{
		shared_ptr<ServerPlayer> player = playerList->players[i];
		if (player != nullptr && equalsIgnoreCase(player->getName(), name))
		{
			return player;
		}
	}

	return nullptr;
}

static void SetAllLevelTimes(MinecraftServer *server, int value)
{
	for (unsigned int i = 0; i < server->levels.length; ++i)
	{
		if (server->levels[i] != nullptr)
		{
			server->levels[i]->setDayTime(value);
		}
	}
}

static bool ExecuteConsoleCommand(MinecraftServer *server, const wstring &rawCommand)
{
	if (server == nullptr)
		return false;

	wstring command = trimString(rawCommand);
	if (command.empty())
		return true;

	if (command[0] == L'/')
	{
		command = trimString(command.substr(1));
	}

	vector<wstring> tokens = SplitConsoleCommand(command);
	if (tokens.empty())
		return true;

	const wstring action = toLower(tokens[0]);
	PlayerList *playerList = server->getPlayers();

	if (action == L"help" || action == L"?")
	{
		ConsoleLog(L"", false);
		ConsoleLog(L"=== RevHost Server ===", false);
		ConsoleLog(L"help          - Show this message", false);
		ConsoleLog(L"stop/exit     - Stop the server", false);
		ConsoleLog(L"list          - List online players", false);
		ConsoleLog(L"players       - List online players", false);
		ConsoleLog(L"tps           - Show server TPS", false);
		ConsoleLog(L"say <msg>     - Broadcast a message", false);
		ConsoleLog(L"save/save-all - Save the world (same as the Save button)", false);
		ConsoleLog(L"clear         - Clear the console", false);
		ConsoleLog(L"time <value>  - Set time of day", false);
		ConsoleLog(L"weather <val> - Set weather (clear/rain/thunder)", false);
		ConsoleLog(L"tp <player> <x> <y> <z> - Teleport", false);
		ConsoleLog(L"give <player> <item> [count] - Give items", false);
		ConsoleLog(L"kill [player] - Kill player", false);
		ConsoleLog(L"gamemode <mode> [player] - Set gamemode", false);
		ConsoleLog(L"xp <amount> [player] - Give XP", false);
		ConsoleLog(L"kick <player> [reason] - Kick a player", false);
		ConsoleLog(L"ban <player> [reason] - Ban a player", false);
		ConsoleLog(L"ban-ip <player> - Ban a player by IP", false);
		ConsoleLog(L"banlist       - List banned players", false);
		ConsoleLog(L"pardon <xuid> - Unban a player", false);
		ConsoleLog(L"pardon-ip <ip> - Unban an IP", false);
		ConsoleLog(L"whitelist on/off - Toggle whitelist", false);
		ConsoleLog(L"whitelist add <xuid> - Add to whitelist", false);
		ConsoleLog(L"whitelist remove <xuid> - Remove from whitelist", false);
		ConsoleLog(L"whitelist list - List whitelisted players", false);
		ConsoleLog(L"op <xuid>     - Grant operator status", false);
		ConsoleLog(L"deop <xuid>   - Revoke operator status", false);
		ConsoleLog(L"vanish [player] - Toggle vanish mode", false);
		ConsoleLog(L"properties    - Show server properties", false);
		ConsoleLog(L"plugins       - List loaded/failed plugins", false);
		ConsoleLog(L"", false);
		return true;
	}

	if (action == L"stop")
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		if (ConsoleSaveFileOriginal::hasPendingBackgroundSave())
		{
			server->info(L"Cannot stop: a save is in progress. Wait for it to finish.");
			return true;
		}
#endif
		server->info(L"Stopping server...");
		server->setSaveOnExit(true);
		MinecraftServer::setExitAfterStop(true);
		MinecraftServer::HaltServer();
		return true;
	}

	if (action == L"list")
	{
		wstring playerNames = (playerList != nullptr) ? playerList->getPlayerNames() : L"";
		if (playerNames.empty()) playerNames = L"(none)";
		server->info(L"Players (" + std::to_wstring((playerList != nullptr) ? playerList->getPlayerCount() : 0) + L"): " + playerNames);
		return true;
	}

	if (action == L"players")
	{
		wstring playerNames = (playerList != nullptr) ? playerList->getPlayerNames() : L"";
		if (playerNames.empty()) playerNames = L"(none)";
		server->info(L"Online (" + std::to_wstring((playerList != nullptr) ? playerList->getPlayerCount() : 0) + L"): " + playerNames);
		return true;
	}

	if (action == L"plugins")
	{
		char buf[16384];
		int outLen = 0;
		int total = FourKitBridge::GetPluginList(buf, sizeof(buf), &outLen);
		if (total == 0 || outLen <= 0)
		{
			server->info(L"Plugins: none loaded.");
			return true;
		}

		std::string data(buf, outLen);
		std::vector<std::string> lines;
		{
			size_t start = 0;
			while (start <= data.size())
			{
				size_t end = data.find('\n', start);
				if (end == std::string::npos)
					end = data.size();
				std::string line = data.substr(start, end - start);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				if (!line.empty())
					lines.push_back(line);
				if (end == data.size())
					break;
				start = end + 1;
			}
		}

		int loadedCount = 0;
		int failedCount = 0;
		for (const auto &line : lines)
		{
			if (line.rfind("loaded\t", 0) == 0) loadedCount++;
			else if (line.rfind("failed\t", 0) == 0) failedCount++;
		}

		server->info(L"Plugins loaded: " + std::to_wstring(loadedCount) + L", failed: " + std::to_wstring(failedCount));
		for (const auto &line : lines)
		{
			std::vector<std::string> parts;
			{
				std::string remaining = line;
				size_t pos;
				while ((pos = remaining.find('\t')) != std::string::npos)
				{
					parts.push_back(remaining.substr(0, pos));
					remaining = remaining.substr(pos + 1);
				}
				parts.push_back(remaining);
			}

			if (parts.empty())
				continue;

			if (parts[0] == "loaded")
			{
				std::string name = parts.size() > 1 ? parts[1] : "";
				std::string version = parts.size() > 2 ? parts[2] : "";
				std::string author = parts.size() > 3 ? parts[3] : "";
				server->info(L"  [loaded] " + ServerRuntime::StringUtils::Utf8ToWide(name) + L" v" + ServerRuntime::StringUtils::Utf8ToWide(version) + L" by " + ServerRuntime::StringUtils::Utf8ToWide(author));
			}
			else if (parts[0] == "failed")
			{
				std::string name = parts.size() > 1 ? parts[1] : "";
				std::string error = parts.size() > 4 ? parts[4] : "";
				server->info(L"  [failed] " + ServerRuntime::StringUtils::Utf8ToWide(name) + L" - " + ServerRuntime::StringUtils::Utf8ToWide(error));
			}
		}
		return true;
	}

	if (action == L"tps")
	{
		wchar_t buf[128];
		swprintf(buf, 128, L"TPS: 5s: %.1f  1m: %.1f  5m: %.1f  10m: %.1f",
			server->m_tps5s, server->m_tps1m, server->m_tps5m, server->m_tps10m);
		server->info(buf);
		return true;
	}

	if (action == L"say")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: say <message>");
			return false;
		}

		wstring message = L"[Server] " + JoinConsoleCommandTokens(tokens, 1);
		if (playerList != nullptr)
		{
			playerList->broadcastAll(std::make_shared<ChatPacket>(message));
		}
		server->info(message);
		return true;
	}

	if (action == L"save" || action == L"save-all")
	{
		// Route through the same eXuiServerAction_AutoSaveGame handler the
		// in-game Save button and the autosave timer use (LCE-Revelations
		// autosave method): non-blocking Flush() with background compression,
		// drained on the main thread. Never touch save state from this thread.
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		app.SetXuiServerAction(ProfileManager.GetPrimaryPad(), eXuiServerAction_AutoSaveGame);
		FourKitBridge::FireWorldSave();
		server->info(L"Save requested.");
#else
		if (playerList != nullptr)
		{
			playerList->saveAll(nullptr, false);
		}
		server->info(L"World saved.");
#endif
		return true;
	}

	if (action == L"time")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: time set <day|night|ticks> | time add <ticks>");
			return false;
		}

		if (toLower(tokens[1]) == L"add")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: time add <ticks>");
				return false;
			}

			int delta = 0;
			if (!TryParseIntValue(tokens[2], delta))
			{
				server->warn(L"Invalid tick value: " + tokens[2]);
				return false;
			}

			for (unsigned int i = 0; i < server->levels.length; ++i)
			{
				if (server->levels[i] != nullptr)
				{
					server->levels[i]->setDayTime(server->levels[i]->getDayTime() + delta);
				}
			}

			server->info(L"Added " + std::to_wstring(delta) + L" ticks.");
			return true;
		}

		wstring timeValue = toLower(tokens[1]);
		if (timeValue == L"set")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: time set <day|night|ticks>");
				return false;
			}
			timeValue = toLower(tokens[2]);
		}

		int targetTime = 0;
		if (timeValue == L"day")
		{
			targetTime = 0;
		}
		else if (timeValue == L"night")
		{
			targetTime = 12500;
		}
		else if (!TryParseIntValue(timeValue, targetTime))
		{
			server->warn(L"Invalid time value: " + timeValue);
			return false;
		}

		SetAllLevelTimes(server, targetTime);
		server->info(L"Time set to " + std::to_wstring(targetTime) + L".");
		return true;
	}

	if (action == L"weather")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: weather <clear|rain|thunder> [seconds]");
			return false;
		}

		int durationSeconds = 600;
		if (tokens.size() >= 3 && !TryParseIntValue(tokens[2], durationSeconds))
		{
			server->warn(L"Invalid duration: " + tokens[2]);
			return false;
		}

		if (server->levels[0] == nullptr)
		{
			server->warn(L"The overworld is not loaded.");
			return false;
		}

		LevelData *levelData = server->levels[0]->getLevelData();
		int duration = durationSeconds * SharedConstants::TICKS_PER_SECOND;
		levelData->setRainTime(duration);
		levelData->setThunderTime(duration);

		wstring weather = toLower(tokens[1]);
		if (weather == L"clear")
		{
			levelData->setRaining(false);
			levelData->setThundering(false);
		}
		else if (weather == L"rain")
		{
			levelData->setRaining(true);
			levelData->setThundering(false);
		}
		else if (weather == L"thunder")
		{
			levelData->setRaining(true);
			levelData->setThundering(true);
		}
		else
		{
			server->warn(L"Usage: weather <clear|rain|thunder> [seconds]");
			return false;
		}

		server->info(L"Weather set to " + weather + L".");
		return true;
	}

	if (action == L"tp" || action == L"teleport")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: tp <player> <target>");
			return false;
		}

		shared_ptr<ServerPlayer> subject = FindPlayerByName(playerList, tokens[1]);
		shared_ptr<ServerPlayer> destination = FindPlayerByName(playerList, tokens[2]);
		if (subject == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		if (destination == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[2]);
			return false;
		}
		if (subject->level->dimension->id != destination->level->dimension->id || !subject->isAlive())
		{
			server->warn(L"Teleport failed because the players are not in the same dimension or the source player is dead.");
			return false;
		}

		subject->ride(nullptr);
		subject->connection->teleport(destination->x, destination->y, destination->z, destination->yRot, destination->xRot);
		server->info(L"Teleported " + subject->getName() + L" to " + destination->getName() + L".");
		return true;
	}

	if (action == L"give")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: give <player> <itemId> [amount] [aux]");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		int itemId = 0;
		int amount = 1;
		int aux = 0;
		if (!TryParseIntValue(tokens[2], itemId))
		{
			server->warn(L"Invalid item id: " + tokens[2]);
			return false;
		}
		if (tokens.size() >= 4 && !TryParseIntValue(tokens[3], amount))
		{
			server->warn(L"Invalid amount: " + tokens[3]);
			return false;
		}
		if (tokens.size() >= 5 && !TryParseIntValue(tokens[4], aux))
		{
			server->warn(L"Invalid aux value: " + tokens[4]);
			return false;
		}
		if (itemId <= 0 || Item::items[itemId] == nullptr)
		{
			server->warn(L"Unknown item id: " + std::to_wstring(itemId));
			return false;
		}
		if (amount <= 0)
		{
			server->warn(L"Amount must be positive.");
			return false;
		}

		shared_ptr<ItemInstance> itemInstance(new ItemInstance(itemId, amount, aux));
		shared_ptr<ItemEntity> drop = player->drop(itemInstance);
		if (drop != nullptr)
		{
			drop->throwTime = 0;
		}
		server->info(L"Gave item " + std::to_wstring(itemId) + L" x" + std::to_wstring(amount) + L" to " + player->getName() + L".");
		return true;
	}

	if (action == L"enchant")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: enchant <player> <enchantId> [level]");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		int enchantmentId = 0;
		int enchantmentLevel = 1;
		if (!TryParseIntValue(tokens[2], enchantmentId))
		{
			server->warn(L"Invalid enchantment id: " + tokens[2]);
			return false;
		}
		if (tokens.size() >= 4 && !TryParseIntValue(tokens[3], enchantmentLevel))
		{
			server->warn(L"Invalid enchantment level: " + tokens[3]);
			return false;
		}

		shared_ptr<ItemInstance> selectedItem = player->getSelectedItem();
		if (selectedItem == nullptr)
		{
			server->warn(L"The player is not holding an item.");
			return false;
		}

		Enchantment *enchantment = Enchantment::enchantments[enchantmentId];
		if (enchantment == nullptr)
		{
			server->warn(L"Unknown enchantment id: " + std::to_wstring(enchantmentId));
			return false;
		}
		if (!enchantment->canEnchant(selectedItem))
		{
			server->warn(L"That enchantment cannot be applied to the selected item.");
			return false;
		}

		if (enchantmentLevel < enchantment->getMinLevel()) enchantmentLevel = enchantment->getMinLevel();
		if (enchantmentLevel > enchantment->getMaxLevel()) enchantmentLevel = enchantment->getMaxLevel();

		if (selectedItem->hasTag())
		{
			ListTag<CompoundTag> *enchantmentTags = selectedItem->getEnchantmentTags();
			if (enchantmentTags != nullptr)
			{
				for (int i = 0; i < enchantmentTags->size(); i++)
				{
					int type = enchantmentTags->get(i)->getShort((wchar_t *)ItemInstance::TAG_ENCH_ID);
					if (Enchantment::enchantments[type] != nullptr && !Enchantment::enchantments[type]->isCompatibleWith(enchantment))
					{
						server->warn(L"That enchantment conflicts with an existing enchantment on the selected item.");
						return false;
					}
				}
			}
		}

		selectedItem->enchant(enchantment, enchantmentLevel);
		server->info(L"Enchanted " + player->getName() + L"'s held item with " + std::to_wstring(enchantmentId) + L" " + std::to_wstring(enchantmentLevel) + L".");
		return true;
	}

	if (action == L"kill")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: kill <player>");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		player->hurt(DamageSource::outOfWorld, 3.4e38f);
		server->info(L"Killed " + player->getName() + L".");
		return true;
	}

	if (action == L"clear")
	{
		HANDLE hOut = GetConsoleOutputHandle();
		if (hOut && hOut != INVALID_HANDLE_VALUE)
		{
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo(hOut, &csbi);
			DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
			DWORD written;
			COORD origin = { 0, 0 };
			FillConsoleOutputCharacterW(hOut, L' ', cells, origin, &written);
			FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, origin, &written);
			SetConsoleCursorPosition(hOut, origin);
		}
		return true;
	}

	if (action == L"exit" || action == L"quit")
	{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
		if (ConsoleSaveFileOriginal::hasPendingBackgroundSave())
		{
			server->info(L"Cannot exit: a save is in progress. Wait for it to finish.");
			return true;
		}
#endif
		server->info(L"Stopping server...");
		MinecraftServer::HaltServer();
		return true;
	}

	if (action == L"gamemode")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: gamemode <0|1|2|survival|creative|adventure> <player>");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[2]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[2]);
			return false;
		}

		GameType *mode = nullptr;
		wstring modeStr = toLower(tokens[1]);
		if (modeStr == L"0" || modeStr == L"survival") mode = GameType::SURVIVAL;
		else if (modeStr == L"1" || modeStr == L"creative") mode = GameType::CREATIVE;
		else if (modeStr == L"2" || modeStr == L"adventure") mode = GameType::ADVENTURE;

		if (mode == nullptr)
		{
			server->warn(L"Invalid gamemode: " + tokens[1]);
			return false;
		}

		player->setGameMode(mode);
		server->info(L"Set " + player->getName() + L"'s gamemode to " + tokens[1] + L".");
		return true;
	}

	if (action == L"defaultgamemode")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: defaultgamemode <0|1|2|survival|creative|adventure>");
			return false;
		}

		GameType *mode = nullptr;
		wstring modeStr = toLower(tokens[1]);
		if (modeStr == L"0" || modeStr == L"survival") mode = GameType::SURVIVAL;
		else if (modeStr == L"1" || modeStr == L"creative") mode = GameType::CREATIVE;
		else if (modeStr == L"2" || modeStr == L"adventure") mode = GameType::ADVENTURE;

		if (mode == nullptr)
		{
			server->warn(L"Invalid gamemode: " + tokens[1]);
			return false;
		}

		for (unsigned int i = 0; i < server->levels.length; ++i)
		{
			if (server->levels[i] != nullptr && server->levels[i]->getLevelData() != nullptr)
			{
				server->levels[i]->getLevelData()->setGameType(mode);
			}
		}
		server->info(L"Default gamemode set to " + tokens[1] + L".");
		return true;
	}

	if (action == L"xp")
	{
		if (tokens.size() < 3)
		{
			server->warn(L"Usage: xp <player> <amount>");
			return false;
		}

		shared_ptr<ServerPlayer> player = FindPlayerByName(playerList, tokens[1]);
		if (player == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}

		int amount = 0;
		if (!TryParseIntValue(tokens[2], amount))
		{
			server->warn(L"Invalid amount: " + tokens[2]);
			return false;
		}

		player->giveExperienceLevels(amount);
		server->info(L"Gave " + std::to_wstring(amount) + L" XP to " + player->getName() + L".");
		return true;
	}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// === kick <player> [reason] ===
	if (action == L"kick")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: kick <player> [reason]");
			return false;
		}
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		if (target->connection && target->connection->isLocal())
		{
			server->warn(L"Cannot kick the host.");
			return false;
		}
		wstring reason = (tokens.size() >= 3) ? JoinConsoleCommandTokens(tokens, 2) : L"Kicked by an operator.";
		playerList->queueDisconnect(target, DisconnectPacket::eDisconnect_Kicked, reason, true, false);
		server->info(L"Kicked " + target->getName() + L".");
		return true;
	}

	// === ban <player> [reason] ===
	if (action == L"ban")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: ban <player> [reason]");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1] + L" (only online players can be banned).");
			return false;
		}
		if (target->connection && target->connection->isLocal())
		{
			server->warn(L"Cannot ban the host.");
			return false;
		}
		PlayerUID xuid = target->getXuid();
		if (xuid == INVALID_XUID)
		{
			server->warn(L"Cannot ban: no valid XUID available.");
			return false;
		}
		if (ServerRuntime::Access::IsPlayerBanned(xuid))
		{
			server->warn(L"That player is already banned.");
			return false;
		}
		auto metadata = ServerRuntime::Access::BanManager::BuildDefaultMetadata("Console");
		metadata.reason = (tokens.size() >= 3) ? ServerRuntime::StringUtils::WideToUtf8(JoinConsoleCommandTokens(tokens, 2)) : "Banned by an operator.";
		std::string playerName = ServerRuntime::StringUtils::WideToUtf8(target->getName());
		if (!ServerRuntime::Access::AddPlayerBan(xuid, playerName, metadata))
		{
			server->warn(L"Failed to write player ban.");
			return false;
		}
		if (target->connection)
		{
			target->connection->disconnect(DisconnectPacket::eDisconnect_Banned);
		}
		server->info(L"Banned player " + target->getName() + L".");
		return true;
	}

	// === ban-ip <address|player> [reason] ===
	if (action == L"ban-ip" || action == L"banip")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: ban-ip <address|player> [reason]");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		std::string remoteIp;
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target != nullptr)
		{
			if (target->connection && target->connection->isLocal())
			{
				server->warn(L"Cannot ban the host IP.");
				return false;
			}
			if (target->connection && target->connection->connection)
			{
				unsigned char smallId = target->connection->connection->getSocket()->getSmallId();
				if (!ServerRuntime::ServerLogManager::TryGetConnectionRemoteIp(smallId, &remoteIp))
				{
					server->warn(L"Cannot resolve that player's IP.");
					return false;
				}
			}
		}
		else
		{
			remoteIp = ServerRuntime::StringUtils::WideToUtf8(tokens[1]);
		}
		if (remoteIp.empty())
		{
			server->warn(L"Could not resolve IP address.");
			return false;
		}
		if (ServerRuntime::Access::IsIpBanned(remoteIp))
		{
			server->warn(L"That IP is already banned.");
			return false;
		}
		auto metadata = ServerRuntime::Access::BanManager::BuildDefaultMetadata("Console");
		metadata.reason = (tokens.size() >= 3) ? ServerRuntime::StringUtils::WideToUtf8(JoinConsoleCommandTokens(tokens, 2)) : "Banned by an operator.";
		if (!ServerRuntime::Access::AddIpBan(remoteIp, metadata))
		{
			server->warn(L"Failed to write IP ban.");
			return false;
		}
		server->info(L"Banned IP " + std::wstring(remoteIp.begin(), remoteIp.end()) + L".");
		return true;
	}

	// === banlist ===
	if (action == L"banlist" || action == L"ban-list")
	{
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		std::vector<ServerRuntime::Access::BannedPlayerEntry> playerEntries;
		std::vector<ServerRuntime::Access::BannedIpEntry> ipEntries;
		ServerRuntime::Access::SnapshotBannedPlayers(&playerEntries);
		ServerRuntime::Access::SnapshotBannedIps(&ipEntries);
		server->info(L"Banned players (" + std::to_wstring(playerEntries.size()) + L"):");
		for (auto &e : playerEntries)
		{
			std::wstring wname(e.name.begin(), e.name.end());
			server->info(L"  " + wname);
		}
		server->info(L"Banned IPs (" + std::to_wstring(ipEntries.size()) + L"):");
		for (auto &e : ipEntries)
		{
			std::wstring wip(e.ip.begin(), e.ip.end());
			server->info(L"  " + wip);
		}
		return true;
	}

	// === pardon <player> ===
	if (action == L"pardon")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: pardon <player>");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		PlayerUID xuid = INVALID_XUID;
		if (!ServerRuntime::Access::TryParseXuid(ServerRuntime::StringUtils::WideToUtf8(tokens[1]), &xuid))
		{
			server->warn(L"Usage: pardon <xuid>  (enter the banned player's XUID)");
			return false;
		}
		if (!ServerRuntime::Access::IsPlayerBanned(xuid))
		{
			server->warn(L"That player is not banned.");
			return false;
		}
		ServerRuntime::Access::RemovePlayerBan(xuid);
		server->info(L"Pardoned XUID " + ServerRuntime::StringUtils::Utf8ToWide(ServerRuntime::Access::FormatXuid(xuid)) + L".");
		return true;
	}

	// === pardon-ip <ip> ===
	if (action == L"pardon-ip" || action == L"pardonip")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: pardon-ip <ip>");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		std::string ip = ServerRuntime::StringUtils::WideToUtf8(tokens[1]);
		if (!ServerRuntime::Access::IsIpBanned(ip))
		{
			server->warn(L"That IP is not banned.");
			return false;
		}
		ServerRuntime::Access::RemoveIpBan(ip);
		server->info(L"Pardoned IP " + tokens[1] + L".");
		return true;
	}

	// === whitelist <on|off|add|remove|list> ===
	if (action == L"whitelist" || action == L"wl")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: whitelist <on|off|add|remove|list>");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		wstring sub = toLower(tokens[1]);
		if (sub == L"on")
		{
			ServerRuntime::Access::SetWhitelistEnabled(true);
			server->info(L"Whitelist enabled.");
			ServerRuntime::ServerPropertiesConfig cfg = ServerRuntime::LoadServerPropertiesConfig();
			cfg.whiteListEnabled = true;
			ServerRuntime::SaveServerPropertiesConfig(cfg);
			return true;
		}
		if (sub == L"off")
		{
			ServerRuntime::Access::SetWhitelistEnabled(false);
			server->info(L"Whitelist disabled.");
			ServerRuntime::ServerPropertiesConfig cfg = ServerRuntime::LoadServerPropertiesConfig();
			cfg.whiteListEnabled = false;
			ServerRuntime::SaveServerPropertiesConfig(cfg);
			return true;
		}
		if (sub == L"list")
		{
			std::vector<ServerRuntime::Access::WhitelistedPlayerEntry> entries;
			ServerRuntime::Access::SnapshotWhitelistedPlayers(&entries);
			server->info(L"Whitelist (" + std::to_wstring(entries.size()) + L"): " +
				std::wstring(ServerRuntime::Access::IsWhitelistEnabled() ? L"enabled" : L"disabled"));
			for (auto &e : entries)
			{
				std::wstring wname(e.name.begin(), e.name.end());
				server->info(L"  " + wname + L" (" + std::wstring(e.xuid.begin(), e.xuid.end()) + L")");
			}
			return true;
		}
		if (sub == L"add")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: whitelist add <xuid|name>");
				return false;
			}
			PlayerUID xuid = INVALID_XUID;
			std::string name = ServerRuntime::StringUtils::WideToUtf8(tokens[2]);
			if (!ServerRuntime::Access::TryParseXuid(name, &xuid))
			{
				server->warn(L"Usage: whitelist add <xuid>");
				return false;
			}
			auto meta = ServerRuntime::Access::WhitelistManager::BuildDefaultMetadata("Console");
			ServerRuntime::Access::AddWhitelistedPlayer(xuid, name, meta);
			server->info(L"Added to whitelist: " + tokens[2]);
			return true;
		}
		if (sub == L"remove")
		{
			if (tokens.size() < 3)
			{
				server->warn(L"Usage: whitelist remove <xuid>");
				return false;
			}
			PlayerUID xuid = INVALID_XUID;
			std::string xuidStr = ServerRuntime::StringUtils::WideToUtf8(tokens[2]);
			if (!ServerRuntime::Access::TryParseXuid(xuidStr, &xuid))
			{
				server->warn(L"Invalid XUID.");
				return false;
			}
			ServerRuntime::Access::RemoveWhitelistedPlayer(xuid);
			server->info(L"Removed from whitelist: " + tokens[2]);
			return true;
		}
		server->warn(L"Usage: whitelist <on|off|add|remove|list>");
		return false;
	}

	// === op <player> ===
	if (action == L"op")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: op <player>");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		PlayerUID xuid = target->getXuid();
		if (xuid == INVALID_XUID)
		{
			server->warn(L"No valid XUID available.");
			return false;
		}
		auto meta = ServerRuntime::Access::OpManager::BuildDefaultMetadata("Console");
		ServerRuntime::Access::AddOp(xuid, ServerRuntime::StringUtils::WideToUtf8(target->getName()), meta);
		server->info(L"Opped " + target->getName() + L".");
		return true;
	}

	// === deop <player> ===
	if (action == L"deop")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: deop <player>");
			return false;
		}
		if (!ServerRuntime::Access::IsInitialized())
		{
			server->warn(L"Access system is not initialized.");
			return false;
		}
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		PlayerUID xuid = target->getXuid();
		if (xuid == INVALID_XUID)
		{
			server->warn(L"No valid XUID available.");
			return false;
		}
		ServerRuntime::Access::RemoveOp(xuid);
		server->info(L"De-opped " + target->getName() + L".");
		return true;
	}

	// === vanish [player] ===
	if (action == L"vanish")
	{
		if (tokens.size() < 2)
		{
			server->warn(L"Usage: vanish <player>");
			return false;
		}
		shared_ptr<ServerPlayer> target = FindPlayerByName(playerList, tokens[1]);
		if (target == nullptr)
		{
			server->warn(L"Unknown player: " + tokens[1]);
			return false;
		}
		if (target->connection && target->connection->isLocal())
		{
			server->warn(L"Cannot vanish the host.");
			return false;
		}
		bool wasVanished = playerList->isVanished(target);
		playerList->setVanished(target, !wasVanished);
		if (!wasVanished)
		{
			server->info(L"Vanished " + target->getName() + L".");
		}
		else
		{
			server->info(L"Unvanished " + target->getName() + L".");
		}
		return true;
	}

	// === properties [get|set] ===
	if (action == L"properties" || action == L"prop")
	{
		ServerRuntime::ServerPropertiesConfig cfg = ServerRuntime::LoadServerPropertiesConfig();

		if (tokens.size() >= 2 && (tokens[1] == L"set" || tokens[1] == L"get"))
		{
			if (tokens.size() < 3)
			{
			ConsoleLog(L"Usage: properties set <key> <value>", false);
			ConsoleLog(L"Use 'properties' with no args to list all.", false);
				return true;
			}

			if (tokens[1] == L"get")
			{
				wstring key = JoinConsoleCommandTokens(tokens, 2);
				// Display a single property by re-reading
				ServerRuntime::ServerPropertiesConfig c2 = ServerRuntime::LoadServerPropertiesConfig();
				(void)c2;
				ConsoleLog(L"Use 'properties' to list all values.", false);
				return true;
			}

			wstring key = JoinConsoleCommandTokens(tokens, 2);
			// For now, read-only display; set support requires per-key apply
			ConsoleLog(L"Property modification not yet supported. Edit server.properties directly.", false);
			return true;
		}

		ConsoleLog(L"", false);
		server->info(L"=== Server Properties ===");
		server->info(L"lan-server-name: " + ServerRuntime::StringUtils::Utf8ToWide(cfg.lanServerName));
		server->info(L"server-port: " + std::to_wstring(cfg.serverPort));
		server->info(L"server-ip: " + ServerRuntime::StringUtils::Utf8ToWide(cfg.serverIp));
		server->info(L"max-players: " + std::to_wstring(cfg.maxPlayers));
		server->info(L"white-list: " + std::wstring(cfg.whiteListEnabled ? L"true" : L"false"));
		server->info(L"difficulty: " + std::to_wstring(cfg.difficulty));
		server->info(L"pvp: " + std::wstring(cfg.pvp ? L"true" : L"false"));
		server->info(L"hardcore: " + std::wstring(cfg.hardcore ? L"true" : L"false"));
		server->info(L"allow-flight: " + std::wstring(cfg.allowFlight ? L"true" : L"false"));
		server->info(L"allow-nether: " + std::wstring(cfg.allowNether ? L"true" : L"false"));
		server->info(L"spawn-animals: " + std::wstring(cfg.spawnAnimals ? L"true" : L"false"));
		server->info(L"spawn-monsters: " + std::wstring(cfg.spawnMonsters ? L"true" : L"false"));
		server->info(L"spawn-npcs: " + std::wstring(cfg.spawnNpcs ? L"true" : L"false"));
		server->info(L"spawn-protection: " + std::to_wstring(cfg.spawnProtectionRadius));
		server->info(L"generate-structures: " + std::wstring(cfg.generateStructures ? L"true" : L"false"));
		server->info(L"do-daylight-cycle: " + std::wstring(cfg.doDaylightCycle ? L"true" : L"false"));
		server->info(L"do-mob-spawning: " + std::wstring(cfg.doMobSpawning ? L"true" : L"false"));
		server->info(L"do-mob-loot: " + std::wstring(cfg.doMobLoot ? L"true" : L"false"));
		server->info(L"do-tile-drops: " + std::wstring(cfg.doTileDrops ? L"true" : L"false"));
		server->info(L"mob-griefing: " + std::wstring(cfg.mobGriefing ? L"true" : L"false"));
		server->info(L"natural-regeneration: " + std::wstring(cfg.naturalRegeneration ? L"true" : L"false"));
		server->info(L"fire-spreads: " + std::wstring(cfg.fireSpreads ? L"true" : L"false"));
		server->info(L"keep-inventory: " + std::wstring(cfg.keepInventory ? L"true" : L"false"));
		server->info(L"host-can-fly: " + std::wstring(cfg.hostCanFly ? L"true" : L"false"));
		server->info(L"host-can-change-hunger: " + std::wstring(cfg.hostCanChangeHunger ? L"true" : L"false"));
		server->info(L"host-can-be-invisible: " + std::wstring(cfg.hostCanBeInvisible ? L"true" : L"false"));
		server->info(L"trust-players: " + std::wstring(cfg.trustPlayers ? L"true" : L"false"));
		server->info(L"tnt: " + std::wstring(cfg.tnt ? L"true" : L"false"));
		server->info(L"disable-saving: " + std::wstring(cfg.disableSaving ? L"true" : L"false"));
		server->info(L"autosave: " + std::wstring(cfg.autosave ? L"true" : L"false"));
		server->info(L"autosave-interval: " + std::to_wstring(cfg.autosaveIntervalSeconds) + L"s");
		server->info(L"max-build-height: " + std::to_wstring(cfg.maxBuildHeight));
		server->info(L"hardcore-ban-ip: " + std::wstring(cfg.hardcoreBanIp ? L"true" : L"false"));
		server->info(L"lan-advertise: " + std::wstring(cfg.lanAdvertise ? L"true" : L"false"));
		ConsoleLog(L"");
		return true;
	}
#endif

	server->warn(L"Unknown command: " + command);
	return false;
}

MinecraftServer::MinecraftServer()
{
	InitializeCriticalSection(&s_consoleOutputCS);

	// 4J - added initialisers
	connection = nullptr;
	settings = nullptr;
	players = nullptr;
	commands = nullptr;
	running = true;
	m_bLoaded = false;
	stopped = false;
	tickCount = 0;
	wstring progressStatus;
	progress = 0;
	motd = L"";

	m_isServerPaused = false;
	m_serverPausedEvent = new C4JThread::Event;

	m_saveOnExit = false;
	m_exitAfterSave = false;
	m_savePerformed = false;
	m_deleteWorldOnExit = false;
	m_suspending = false;

	m_tpsWindowStartMs = System::currentTimeMillis();
	m_tpsWindowTicks = 0;
	m_tpsWindowTicksMs = 0;
	m_tps5s = 20.0;
	m_tps1m = 20.0;
	m_tps5m = 20.0;
	m_tps10m = 20.0;
	m_tps5sTicks = 0;
	m_tps5sStartMs = System::currentTimeMillis();
	m_tps1mTicks = 0;
	m_tps1mStartMs = System::currentTimeMillis();
	m_tps5mTicks = 0;
	m_tps5mStartMs = System::currentTimeMillis();
	m_tps10mTicks = 0;
	m_tps10mStartMs = System::currentTimeMillis();

	m_ugcPlayersVersion = 0;
	m_texturePackId = 0;
	maxBuildHeight = Level::maxBuildHeight;
	playerIdleTimeout = 0;
	m_postUpdateThread = nullptr;
	forceGameType = false;
	m_spawnProtectionRadius = 0;

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	m_autosaveNextTickMs = 0;
	m_autosaveIntervalMs = 300000; // 5 minutes
	m_autosaveEnabled = true;
	m_whiteListEnabled = false;
#endif

	commandDispatcher = new ServerCommandDispatcher();
	InitializeCriticalSection(&m_consoleInputCS);

	DispenserBootstrap::bootStrap();
}

MinecraftServer::~MinecraftServer()
{
	DeleteCriticalSection(&m_consoleInputCS);
}

bool MinecraftServer::initServer(int64_t seed, NetworkGameInitData *initData, DWORD initSettings, bool findSeed)
{
	// 4J - removed
#if 0
	commands = new ConsoleCommands(this);

	Thread t = new Thread() {
		public void run() {
			BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
			String line = null;
			try {
				while (!stopped && running && (line = br.readLine()) != null) {
					handleConsoleInput(line, MinecraftServer.this);
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	};
	t.setDaemon(true);
	t.start();


	LogConfigurator.initLogger();
	logger.info("Starting minecraft server version " + VERSION);

	if (Runtime.getRuntime().maxMemory() / 1024 / 1024 < 512) {
		logger.warning("**** NOT ENOUGH RAM!");
		logger.warning("To start the server with more ram, launch it as \"java -Xmx1024M -Xms1024M -jar minecraft_server.jar\"");
	}

	logger.info("Loading properties");
#endif

	// Load server.properties through our full config system first,
	// which generates all defaults if the file is missing or incomplete.
	// Then the old Settings class reads the now-complete file.
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		ServerRuntime::ServerPropertiesConfig spConfig = ServerRuntime::LoadServerPropertiesConfig();
		m_autosaveIntervalMs = (unsigned int)(spConfig.autosaveIntervalSeconds * 1000);
		m_autosaveEnabled = spConfig.autosave;
		m_whiteListEnabled = spConfig.whiteListEnabled;
		m_autosaveNextTickMs = GetTickCount() + m_autosaveIntervalMs;
		app.DebugPrintf("server.properties loaded (%d max players, autosave=%s, autosave-interval=%ds)",
			spConfig.maxPlayers,
			spConfig.autosave ? "enabled" : "disabled",
			spConfig.autosaveIntervalSeconds);

		// Natural spawn caps (max-monsters, max-animals, ...)
		MobCategory::monster->setMaxInstancesPerLevel(spConfig.maxMonsters);
		MobCategory::creature->setMaxInstancesPerLevel(spConfig.maxAnimals);
		MobCategory::ambient->setMaxInstancesPerLevel(spConfig.maxAmbient);
		MobCategory::waterCreature->setMaxInstancesPerLevel(spConfig.maxWaterAnimals);
		MobCategory::creature_wolf->setMaxInstancesPerLevel(spConfig.maxWolves);
		MobCategory::creature_chicken->setMaxInstancesPerLevel(spConfig.maxChickens);
		MobCategory::creature_mushroomcow->setMaxInstancesPerLevel(spConfig.maxMushroomCows);

		// Keep the split-screen / identity-token path in sync with the configured network settings
		g_Win64MultiplayerPort = spConfig.serverPort;
		strncpy_s(g_Win64MultiplayerIP, spConfig.serverIp.c_str(), _TRUNCATE);

		ServerRuntime::Security::SecuritySettings secSettings;
		secSettings.hidePlayerListPreLogin = spConfig.hidePlayerListPreLogin;
		secSettings.rateLimitConnectionsPerWindow = spConfig.rateLimitConnectionsPerWindow;
		secSettings.rateLimitWindowSeconds = spConfig.rateLimitWindowSeconds;
		secSettings.maxPendingConnections = spConfig.maxPendingConnections;
		secSettings.requireChallengeToken = spConfig.requireChallengeToken;
		secSettings.enableStreamCipher = spConfig.enableStreamCipher;
		secSettings.requireSecureClient = spConfig.requireSecureClient;
		secSettings.proxyProtocol = spConfig.proxyProtocol;
		ServerRuntime::Security::InitializeSettings(secSettings);
		app.DebugPrintf("Security settings initialized (streamCipher=%s, proxyProtocol=%s, rateLimit=%d)",
			ServerRuntime::Security::GetSettings().enableStreamCipher ? "true" : "false",
			ServerRuntime::Security::GetSettings().proxyProtocol ? "true" : "false",
			ServerRuntime::Security::GetSettings().rateLimitConnectionsPerWindow);
	}
#endif

	settings = new Settings(new File(L"server.properties"));
	// Dedicated-only: spawn-protection radius in blocks; 0 disables protection.
	m_spawnProtectionRadius = GetDedicatedServerInt(settings, L"spawn-protection", 0);
	if (m_spawnProtectionRadius < 0) m_spawnProtectionRadius = 0;
	if (m_spawnProtectionRadius > 256) m_spawnProtectionRadius = 256;

	app.SetGameHostOption(eGameHostOption_Difficulty, GetDedicatedServerInt(settings, L"difficulty", app.GetGameHostOption(eGameHostOption_Difficulty)));
	app.SetGameHostOption(eGameHostOption_Structures, GetDedicatedServerBool(settings, L"generate-structures", app.GetGameHostOption(eGameHostOption_Structures) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_PvP, GetDedicatedServerBool(settings, L"pvp", app.GetGameHostOption(eGameHostOption_PvP) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_TrustPlayers, GetDedicatedServerBool(settings, L"trust-players", app.GetGameHostOption(eGameHostOption_TrustPlayers) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_FireSpreads, GetDedicatedServerBool(settings, L"fire-spreads", app.GetGameHostOption(eGameHostOption_FireSpreads) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_TNT, GetDedicatedServerBool(settings, L"tnt", app.GetGameHostOption(eGameHostOption_TNT) > 0) ? 1 : 0);

	// Newly wired server.properties game host options
	// RevHost: host privileges (cheats) are always ON by default and never disabled by config.
	app.SetGameHostOption(eGameHostOption_HostCanFly, 1);
	app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, 1);
	app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, 1);
	app.SetGameHostOption(eGameHostOption_Hardcore, GetDedicatedServerBool(settings, L"hardcore", app.GetGameHostOption(eGameHostOption_Hardcore) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DisableSaving, GetDedicatedServerBool(settings, L"disable-saving", app.GetGameHostOption(eGameHostOption_DisableSaving) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_MobGriefing, GetDedicatedServerBool(settings, L"mob-griefing", app.GetGameHostOption(eGameHostOption_MobGriefing) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_KeepInventory, GetDedicatedServerBool(settings, L"keep-inventory", app.GetGameHostOption(eGameHostOption_KeepInventory) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoMobSpawning, GetDedicatedServerBool(settings, L"do-mob-spawning", app.GetGameHostOption(eGameHostOption_DoMobSpawning) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoMobLoot, GetDedicatedServerBool(settings, L"do-mob-loot", app.GetGameHostOption(eGameHostOption_DoMobLoot) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoTileDrops, GetDedicatedServerBool(settings, L"do-tile-drops", app.GetGameHostOption(eGameHostOption_DoTileDrops) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_NaturalRegeneration, GetDedicatedServerBool(settings, L"natural-regeneration", app.GetGameHostOption(eGameHostOption_NaturalRegeneration) > 0) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoDaylightCycle, GetDedicatedServerBool(settings, L"do-daylight-cycle", app.GetGameHostOption(eGameHostOption_DoDaylightCycle) > 0) ? 1 : 0);

	app.DebugPrintf("\n*** SERVER SETTINGS ***\n");
	app.DebugPrintf("ServerSettings: pvp is %s\n",(app.GetGameHostOption(eGameHostOption_PvP)>0)?"on":"off");
	app.DebugPrintf("ServerSettings: fire spreads is %s\n",(app.GetGameHostOption(eGameHostOption_FireSpreads)>0)?"on":"off");
	app.DebugPrintf("ServerSettings: tnt explodes is %s\n",(app.GetGameHostOption(eGameHostOption_TNT)>0)?"on":"off");
	app.DebugPrintf("ServerSettings: spawn protection radius is %d\n", m_spawnProtectionRadius);
	app.DebugPrintf("\n");

	// TODO 4J Stu - Init a load of settings based on data passed as params
	//settings->setBooleanAndSave( L"host-friends-only", (app.GetGameHostOption(eGameHostOption_FriendsOfFriends)>0) );

	// 4J - Unused
	//localIp = settings->getString(L"server-ip", L"");
	//onlineMode = settings->getBoolean(L"online-mode", true);
	//motd = settings->getString(L"motd", L"A Minecraft Server");
	//motd.replace('�', '$');

	setAnimals(GetDedicatedServerBool(settings, L"spawn-animals", true));
	setNpcsEnabled(GetDedicatedServerBool(settings, L"spawn-npcs", true));
	setPvpAllowed(app.GetGameHostOption( eGameHostOption_PvP )>0?true:false);

	// 4J Stu - We should never have hacked clients flying when they shouldn't be like the PC version, so enable flying always
	// Fix for #46612 - TU5: Code: Multiplayer: A client can be banned for flying when accidentaly being blown by dynamite
	setFlightAllowed(GetDedicatedServerBool(settings, L"allow-flight", true));

	// 4J Stu - Enabling flight to stop it kicking us when we use it
#if (defined _DEBUG_MENUS_ENABLED && defined _DEBUG)
	setFlightAllowed(true);
#endif

#if 1
	connection = new ServerConnection(this);
	Socket::Initialise(connection);	// 4J - added
#else
	// 4J - removed
	InetAddress localAddress = null;
	if (localIp.length() > 0) localAddress = InetAddress.getByName(localIp);
	port = settings.getInt("server-port", DEFAULT_MINECRAFT_PORT);

	logger.info("Starting Minecraft server on " + (localIp.length() == 0 ? "*" : localIp) + ":" + port);
	try {
		connection = new ServerConnection(this, localAddress, port);
	} catch (IOException e) {
		logger.warning("**** FAILED TO BIND TO PORT!");
		logger.log(Level.WARNING, "The exception was: " + e.toString());
		logger.warning("Perhaps a server is already running on that port?");
		return false;
	}

	if (!onlineMode) {
		logger.warning("**** SERVER IS RUNNING IN OFFLINE/INSECURE MODE!");
		logger.warning("The server will make no attempt to authenticate usernames. Beware.");
		logger.warning("While this makes the game possible to play without internet access, it also opens up the ability for hackers to connect with any username they choose.");
		logger.warning("To change this, set \"online-mode\" to \"true\" in the server.settings file.");
	}
#endif
	setPlayers(new PlayerList(this));
#ifdef _WINDOWS64
	{
		int maxP = getPlayerList()->getMaxPlayers();
		WinsockNetLayer::UpdateAdvertiseMaxPlayers((BYTE)(maxP > 255 ? 255 : maxP));
	}
#endif

	// 4J-JEV: Need to wait for levelGenerationOptions to load.
	while ( app.getLevelGenerationOptions() != nullptr && !app.getLevelGenerationOptions()->hasLoadedData() )
		Sleep(1);

	if ( app.getLevelGenerationOptions() != nullptr && !app.getLevelGenerationOptions()->ready() )
	{
		// TODO: Stop loading, add error message.
	}

	int64_t levelNanoTime = System::nanoTime();

        wstring levelName = (initData && !initData->levelName.empty()) ? initData->levelName : GetDedicatedServerString(settings, L"level-name", L"world");
		wstring levelTypeString;

	bool gameRuleUseFlatWorld = false;
	if(app.getLevelGenerationOptions() != nullptr)
	{
		gameRuleUseFlatWorld = app.getLevelGenerationOptions()->getuseFlatWorld();
	}
	if(gameRuleUseFlatWorld || app.GetGameHostOption(eGameHostOption_LevelType)>0)
	{
		levelTypeString = GetDedicatedServerString(settings, L"level-type",  L"flat");
	}
	else
	{
		levelTypeString = GetDedicatedServerString(settings, L"level-type",L"default");
	}

	LevelType *pLevelType = LevelType::getLevelType(levelTypeString);
	if (pLevelType == nullptr)
	{
		pLevelType = LevelType::lvl_normal;
	}

	ProgressRenderer *mcprogress = Minecraft::GetInstance()->progressRenderer;
	mcprogress->progressStart(IDS_PROGRESS_INITIALISING_SERVER);

	if( findSeed )
	{
		int worldSizeChunks = (initData && initData->xzSize > 0) ? (int)initData->xzSize : 54;
#ifdef __PSVITA__
		seed = BiomeSource::findSeed(pLevelType, &running, worldSizeChunks);
#else
		seed = BiomeSource::findSeed(pLevelType, worldSizeChunks);
#endif
	}

	setMaxBuildHeight(GetDedicatedServerInt(settings, L"max-build-height", Level::maxBuildHeight));
	setMaxBuildHeight(((getMaxBuildHeight() + 8) / 16) * 16);
	setMaxBuildHeight(Mth::clamp(getMaxBuildHeight(), 64, Level::maxBuildHeight));
	//settings->setProperty(L"max-build-height", maxBuildHeight);

	//        logger.info("Preparing level \"" + levelName + "\"");
	m_bLoaded = loadLevel(new McRegionLevelStorageSource(File(L".")), levelName, seed, pLevelType, initData);
	//        logger.info("Done (" + (System.nanoTime() - levelNanoTime) + "ns)! For help, type \"help\" or \"?\"");

	// 4J delete passed in save data now - this is only required for the tutorial which is loaded by passing data directly in rather than using the storage manager
	if( initData->saveData )
	{
		delete initData->saveData->data;
		initData->saveData->data = 0;
		initData->saveData->fileSize = 0;
	}

	FourKitBridge::Initialize();

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Initialize ban/whitelist/op access system (whitelist enabled per server.properties)
	ServerRuntime::Access::Initialize(".", m_whiteListEnabled);
#endif

	g_NetworkManager.ServerReady();	// 4J added

	// Signal that the server console is ready for input
	extern void SignalConsoleReady();
	SignalConsoleReady();

	// Log autosave status
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	{
		wchar_t buf[128];
		if (m_autosaveEnabled)
		{
			int intervalSecs = m_autosaveIntervalMs / 1000;
			swprintf(buf, 128, L"Autosave: enabled, every %d seconds", intervalSecs);
		}
		else
		{
			swprintf(buf, 128, L"Autosave: disabled");
		}
		ConsoleLog(buf);
	}
#endif

	ConsoleLog(L"Done! For help, type \"help\"");

	return m_bLoaded;

}

// 4J - added - extra thread to post processing on separate thread during level creation
int MinecraftServer::runPostUpdate(void* lpParam)
{
	ShutdownManager::HasStarted(ShutdownManager::ePostProcessThread);

	MinecraftServer *server = static_cast<MinecraftServer *>(lpParam);
	Entity::useSmallIds();		// This thread can end up spawning entities as resources
	IntCache::CreateNewThreadStorage();
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	Compression::UseDefaultThreadStorage();
	Level::enableLightingCache();
	Tile::CreateNewThreadStorage();

	// Update lights for both levels until we are signalled to terminate
	do
	{
		EnterCriticalSection(&server->m_postProcessCS);
		if( server->m_postProcessRequests.size() )
		{
			MinecraftServer::postProcessRequest request = server->m_postProcessRequests.back();
			server->m_postProcessRequests.pop_back();
			LeaveCriticalSection(&server->m_postProcessCS);
			static int count = 0;
			PIXBeginNamedEvent(0,"Post processing %d ", (count++)%8);
			request.chunkSource->postProcess(request.chunkSource, request.x, request.z );
			PIXEndNamedEvent();
		}
		else
		{
			LeaveCriticalSection(&server->m_postProcessCS);
		}
		Sleep(1);
	} while (!server->m_postUpdateTerminate && ShutdownManager::ShouldRun(ShutdownManager::ePostProcessThread));
	//#ifndef __PS3__
	// One final pass through updates to make sure we're done
	EnterCriticalSection(&server->m_postProcessCS);
	int maxRequests = server->m_postProcessRequests.size();
	while(server->m_postProcessRequests.size() && ShutdownManager::ShouldRun(ShutdownManager::ePostProcessThread) )
	{
		MinecraftServer::postProcessRequest request = server->m_postProcessRequests.back();
		server->m_postProcessRequests.pop_back();
		LeaveCriticalSection(&server->m_postProcessCS);
		request.chunkSource->postProcess(request.chunkSource, request.x, request.z );
#ifdef __PS3__
#ifndef _CONTENT_PACKAGE
		if((server->m_postProcessRequests.size() % 10) == 0)
			printf("processing request %00d\n", server->m_postProcessRequests.size());
#endif
		Sleep(1);
#endif
		EnterCriticalSection(&server->m_postProcessCS);
	}
	LeaveCriticalSection(&server->m_postProcessCS);
	//#endif //__PS3__
	Tile::ReleaseThreadStorage();
	IntCache::ReleaseThreadStorage();
	AABB::ReleaseThreadStorage();
	Vec3::ReleaseThreadStorage();
	Level::destroyLightingCache();

	ShutdownManager::HasFinished(ShutdownManager::ePostProcessThread);

	return 0;
}

void	MinecraftServer::addPostProcessRequest(ChunkSource *chunkSource, int x, int z)
{
	EnterCriticalSection(&m_postProcessCS);
	m_postProcessRequests.push_back(MinecraftServer::postProcessRequest(x,z,chunkSource));
	LeaveCriticalSection(&m_postProcessCS);
}

void MinecraftServer::postProcessTerminate(ProgressRenderer *mcprogress)
{
	DWORD status = 0;

	EnterCriticalSection(&server->m_postProcessCS);
	size_t postProcessItemCount = server->m_postProcessRequests.size();
	LeaveCriticalSection(&server->m_postProcessCS);

	do
	{
		status = m_postUpdateThread->WaitForCompletion(50);
		if( status == WAIT_TIMEOUT )
		{
			EnterCriticalSection(&server->m_postProcessCS);
			size_t postProcessItemRemaining = server->m_postProcessRequests.size();
			LeaveCriticalSection(&server->m_postProcessCS);

			if( postProcessItemCount )
			{
				mcprogress->progressStagePercentage((postProcessItemCount - postProcessItemRemaining) * 100 / postProcessItemCount);
			}
			CompressedTileStorage::tick();
			SparseLightStorage::tick();
			SparseDataStorage::tick();
		}
	} while ( status == WAIT_TIMEOUT );
	delete m_postUpdateThread;
	m_postUpdateThread = nullptr;
	DeleteCriticalSection(&m_postProcessCS);
}

bool MinecraftServer::loadLevel(LevelStorageSource *storageSource, const wstring& name, int64_t levelSeed, LevelType *pLevelType, NetworkGameInitData *initData)
{
	//	4J - TODO - do with new save stuff
	//    if (storageSource->requiresConversion(name))
	//	{
	//		assert(false);
	//    }
	ProgressRenderer *mcprogress = Minecraft::GetInstance()->progressRenderer;

	// 4J Added - store save folder name for potential hardcore world deletion
	{
		char szSaveFolder[MAX_SAVEFILENAME_LENGTH] = {};
		StorageManager.GetSaveUniqueFilename(szSaveFolder);
		wchar_t wSaveFolder[MAX_SAVEFILENAME_LENGTH] = {};
		mbstowcs(wSaveFolder, szSaveFolder, MAX_SAVEFILENAME_LENGTH - 1);
		m_saveFolderName = wSaveFolder;
	}

	// 4J TODO - free levels here if there are already some?
	levels = ServerLevelArray(3);

	// RevHost: host privileges (cheats) are always ON by default whenever a world is created or loaded.
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	app.SetGameHostOption(eGameHostOption_HostCanFly, 1);
	app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, 1);
	app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, 1);
#endif

	int gameTypeId = GetDedicatedServerInt(settings, L"gamemode", app.GetGameHostOption(eGameHostOption_GameType));//LevelSettings::GAMETYPE_SURVIVAL);
	GameType *gameType = LevelSettings::validateGameType(gameTypeId);
	app.DebugPrintf("Default game type: %d\n" , gameTypeId);

	LevelSettings *levelSettings = new LevelSettings(levelSeed, gameType, app.GetGameHostOption(eGameHostOption_Structures)>0?true:false, isHardcore(), true, pLevelType, initData->xzSize, initData->hellScale);
	if( app.GetGameHostOption(eGameHostOption_BonusChest ) ) levelSettings->enableStartingBonusItems();

	// 4J - temp - load existing level
	shared_ptr<McRegionLevelStorage> storage = nullptr;
	bool levelChunksNeedConverted = false;
	if( initData->saveData != nullptr )
	{
		// We are loading a file from disk with the data passed in

#ifdef SPLIT_SAVES
		ConsoleSaveFileOriginal oldFormatSave( initData->saveData->saveName, initData->saveData->data, initData->saveData->fileSize, false, initData->savePlatform );
		ConsoleSaveFile* pSave = new ConsoleSaveFileSplit( &oldFormatSave );

		//ConsoleSaveFile* pSave = new ConsoleSaveFileSplit( initData->saveData->saveName, initData->saveData->data, initData->saveData->fileSize, false, initData->savePlatform );
#else
		ConsoleSaveFile* pSave = new ConsoleSaveFileOriginal( initData->saveData->saveName, initData->saveData->data, initData->saveData->fileSize, false, initData->savePlatform );
#endif
		if(pSave->isSaveEndianDifferent())
			levelChunksNeedConverted = true;
		pSave->ConvertToLocalPlatform(); // check if we need to convert this file from PS3->PS4

		storage = std::make_shared<McRegionLevelStorage>(pSave, File(L"."), name, true);
	}
	else
	{
		// We are loading a save from the storage manager
#ifdef SPLIT_SAVES
		bool bLevelGenBaseSave = false;
		LevelGenerationOptions *levelGen = app.getLevelGenerationOptions();
		if( levelGen != nullptr && levelGen->requiresBaseSave())
		{
			DWORD fileSize = 0;
			LPVOID pvSaveData = levelGen->getBaseSaveData(fileSize);
			if(pvSaveData && fileSize != 0) bLevelGenBaseSave = true;
		}
		ConsoleSaveFileSplit *newFormatSave = nullptr;
		if(bLevelGenBaseSave)
		{
			ConsoleSaveFileOriginal oldFormatSave( L"" );
			newFormatSave = new ConsoleSaveFileSplit( &oldFormatSave );
		}
		else
		{
			newFormatSave = new ConsoleSaveFileSplit( L"" );
		}

		storage = shared_ptr<McRegionLevelStorage>(new McRegionLevelStorage(newFormatSave, File(L"."), name, true));
#else
		ConsoleSaveFileOriginal* pSave = new ConsoleSaveFileOriginal(L"");

		pSave->ConvertToLocalPlatform();
		storage = std::make_shared<McRegionLevelStorage>(pSave, File(L"."), name, true);
	
#endif
	}

	//	McRegionLevelStorage *storage = new McRegionLevelStorage(new ConsoleSaveFile( L"" ), L"", L"", 0); // original
	//    McRegionLevelStorage *storage = new McRegionLevelStorage(File(L"."), name, true); // TODO
	for (unsigned int i = 0; i < levels.length; i++)
	{
		if( s_bServerHalted || !g_NetworkManager.IsInSession() )
		{
			return false;
		}

		//            String levelName = name;
		//            if (i == 1) levelName += "_nether";
		int dimension = 0;
		if (i == 1) dimension = -1;
		if (i == 2) dimension = 1;
		if (i == 0)
		{
			levels[i] = new ServerLevel(this, storage, name, dimension, levelSettings);
			if(app.getLevelGenerationOptions() != nullptr)
			{
				LevelGenerationOptions *mapOptions = app.getLevelGenerationOptions();
				Pos *spawnPos = mapOptions->getSpawnPos();
				if( spawnPos != nullptr )
				{
					levels[i]->setSpawnPos( spawnPos );
				}

				levels[i]->getLevelData()->setHasBeenInCreative(mapOptions->isFromDLC());
			}
		}
		else levels[i] = new DerivedServerLevel(this, storage, name, dimension, levelSettings, levels[0]);
		//        levels[i]->addListener(new ServerLevelListener(this, levels[i]));		// 4J - have moved this to the ServerLevel ctor so that it is set up in time for the first chunk to load, which might actually happen there

		// 4J Stu - We set the levels difficulty based on the minecraft options
		//levels[i]->difficulty = settings->getBoolean(L"spawn-monsters", true) ? Difficulty::EASY : Difficulty::PEACEFUL;
		Minecraft *pMinecraft = Minecraft::GetInstance();
		//		m_lastSentDifficulty = pMinecraft->options->difficulty;
		levels[i]->difficulty = app.GetGameHostOption(eGameHostOption_Difficulty); //pMinecraft->options->difficulty;
		app.DebugPrintf("MinecraftServer::loadLevel - Difficulty = %d\n",levels[i]->difficulty);

#if DEBUG_SERVER_DONT_SPAWN_MOBS
		levels[i]->setSpawnSettings(false, false);
#else
		levels[i]->setSpawnSettings(GetDedicatedServerBool(settings, L"spawn-monsters", true), animals);
#endif
		levels[i]->getLevelData()->setGameType(gameType);

#ifdef MINECRAFT_SERVER_BUILD
		// Dedicated server: server.properties hardcore flag is authoritative
		levels[i]->getLevelData()->setHardcore(isHardcore());
#endif
		// Offline/client-hosted: keep the world's saved hardcore flag from NBT

		if(app.getLevelGenerationOptions() != nullptr)
		{
			LevelGenerationOptions *mapOptions = app.getLevelGenerationOptions();
			levels[i]->getLevelData()->setHasBeenInCreative(mapOptions->getLevelHasBeenInCreative() );
		}

		players->setLevel(levels);
	}

	if( levels[0]->isNew )
	{
		mcprogress->progressStage(IDS_PROGRESS_GENERATING_SPAWN_AREA);
	}
	else
	{
		mcprogress->progressStage(IDS_PROGRESS_LOADING_SPAWN_AREA);
	}
	app.SetGameHostOption( eGameHostOption_HasBeenInCreative, gameType == GameType::CREATIVE || levels[0]->getHasBeenInCreative() );
	app.SetGameHostOption( eGameHostOption_Structures, levels[0]->isGenerateMapFeatures() );

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	// 4J - Make a new thread to do post processing
	InitializeCriticalSection(&m_postProcessCS);

	// 4J-PB - fix for 108310 - TCR #001 BAS Game Stability: TU12: Code: Compliance: Crash after creating world on "journey" seed.
	// Stack gets very deep with some sand tower falling, so increased the stacj to 256K from 128k on other platforms (was already set to that on PS3 and Orbis)

	m_postUpdateThread = new C4JThread(runPostUpdate, this, "Post processing", 256*1024);

	m_postUpdateTerminate = false;
	m_postUpdateThread->SetProcessor(CPU_CORE_POST_PROCESSING);
	m_postUpdateThread->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
	m_postUpdateThread->Run();

	int64_t startTime = System::currentTimeMillis();

	// 4J Stu - Added this to temporarily make starting games on vita faster
#ifdef __PSVITA__
	int r = 48;
#else
	int r = 196;
#endif

	//  4J JEV: load gameRules.
	ConsoleSavePath filepath(GAME_RULE_SAVENAME);
	ConsoleSaveFile *csf = getLevel(0)->getLevelStorage()->getSaveFile();
	if( csf->doesFileExist(filepath) )
	{
		DWORD numberOfBytesRead;
		byteArray ba_gameRules;

		FileEntry *fe = csf->createFile(filepath);

		ba_gameRules.length = fe->getFileSize();
		ba_gameRules.data = new BYTE[ ba_gameRules.length ];

		csf->setFilePointer(fe,0,nullptr,FILE_BEGIN);
		csf->readFile(fe, ba_gameRules.data, ba_gameRules.length, &numberOfBytesRead);
		assert(numberOfBytesRead == ba_gameRules.length);

		app.m_gameRules.loadGameRules(ba_gameRules.data, ba_gameRules.length);
		csf->closeHandle(fe);
	}

	int64_t lastTime = System::currentTimeMillis();
#ifdef _LARGE_WORLDS
	if(app.GetGameNewWorldSize() > levels[0]->getLevelData()->getXZSizeOld())
	{
		if(!app.GetGameNewWorldSizeUseMoat()) // check the moat settings to see if we should be overwriting the edge tiles
		{
			overwriteBordersForNewWorldSize(levels[0]);
		}
		// we're always overwriting hell edges
		int oldHellSize = levels[0]->getLevelData()->getXZHellSizeOld();
		overwriteHellBordersForNewWorldSize(levels[1], oldHellSize);
	}
#endif

	// 4J Stu - This loop is changed in 1.0.1 to only process the first level (ie the overworld), but I think we still want to do them all
	int i = 0;
	for (int i = 0; i < levels.length ; i++)
	{
		//        logger.info("Preparing start region for level " + i);
		if (i == 0 || GetDedicatedServerBool(settings, L"allow-nether", true))
		{
			ServerLevel *level = levels[i];
			if(levelChunksNeedConverted)
			{
				// 				storage->getSaveFile()->convertLevelChunks(level)
			}

#if 0
			int64_t lastStorageTickTime = System::currentTimeMillis();

			// Test code to enable full creation of levels at start up
			int halfsidelen = ( i == 0 ) ? 27 : 9;
			for( int x = -halfsidelen; x < halfsidelen; x++ )
			{
				for( int z = -halfsidelen; z < halfsidelen; z++ )
				{
					int total = halfsidelen * halfsidelen * 4;
					int pos = z + halfsidelen + ( ( x + halfsidelen ) * 2 * halfsidelen );
					mcprogress->progressStagePercentage((pos) * 100 / total);
					level->cache->create(x,z, true);	// 4J - added parameter to disable postprocessing here

					if( System::currentTimeMillis() - lastStorageTickTime > 50 )
					{
						CompressedTileStorage::tick();
						SparseLightStorage::tick();
						SparseDataStorage::tick();
						lastStorageTickTime = System::currentTimeMillis();
					}
				}
			}
#else
			int64_t lastStorageTickTime = System::currentTimeMillis();
			Pos *spawnPos = level->getSharedSpawnPos();

			int twoRPlusOne = r*2 + 1;
			int total = twoRPlusOne * twoRPlusOne;
			for (int x = -r; x <= r && running; x += 16)
			{
				for (int z = -r; z <= r && running; z += 16)
				{
					if( s_bServerHalted || !g_NetworkManager.IsInSession() )
					{
						delete spawnPos;
						m_postUpdateTerminate = true;
						postProcessTerminate(mcprogress);
						return false;
					}
					//					printf(">>>%d %d %d\n",i,x,z);
					//                    int64_t now = System::currentTimeMillis();
					//                    if (now < lastTime) lastTime = now;
					//                    if (now > lastTime + 1000)
					{
						int pos = (x + r) * twoRPlusOne + (z + 1);
						//                        setProgress(L"Preparing spawn area", (pos) * 100 / total);
						mcprogress->progressStagePercentage((pos+r) * 100 / total);
						//                        lastTime = now;
					}
					static int count = 0;
					PIXBeginNamedEvent(0,"Creating %d ", (count++)%8);
					level->cache->create((spawnPos->x + x) >> 4, (spawnPos->z + z) >> 4, true);	// 4J - added parameter to disable postprocessing here
					PIXEndNamedEvent();
					//                    while (level->updateLights() && running)
					//                        ;
					if( System::currentTimeMillis() - lastStorageTickTime > 50 )
					{
						CompressedTileStorage::tick();
						SparseLightStorage::tick();
						SparseDataStorage::tick();
						lastStorageTickTime = System::currentTimeMillis();
					}
				}
			}

			// 4J - removed this as now doing the recheckGaps call when each chunk is post-processed, so can happen on things outside of the spawn area too
#if 0
			// 4J - added this code to propagate lighting properly in the spawn area before we go sharing it with the local client or across the network
			for (int x = -r; x <= r && running; x += 16)
			{
				for (int z = -r; z <= r && running; z += 16)
				{
					PIXBeginNamedEvent(0,"Lighting gaps for %d %d",x,z);
					level->getChunkAt(spawnPos->x + x, spawnPos->z + z)->recheckGaps(true);
					PIXEndNamedEvent();
				}
			}
#endif

			delete spawnPos;
#endif
		}
	}
	//	printf("Main thread complete at %dms\n",System::currentTimeMillis() - startTime);

	// Wait for post processing, then lighting threads, to end (post-processing may make more lighting changes)
	m_postUpdateTerminate = true;

	postProcessTerminate(mcprogress);


	// stronghold position?
	if(levels[0]->dimension->id==0)
	{

		app.DebugPrintf("===================================\n");

		if(!levels[0]->getLevelData()->getHasStronghold())
		{
			int x,z;
			if(app.GetTerrainFeaturePosition(eTerrainFeature_Stronghold,&x,&z))
			{
				levels[0]->getLevelData()->setXStronghold(x);
				levels[0]->getLevelData()->setZStronghold(z);
				levels[0]->getLevelData()->setHasStronghold();

				app.DebugPrintf("=== FOUND stronghold in terrain features list\n");

			}
			else
			{
				// can't find the stronghold position in the terrain feature list. Do we have to run a post-process?
				app.DebugPrintf("=== Can't find stronghold in terrain features list\n");
			}
		}
		else
		{
			app.DebugPrintf("=== Leveldata has stronghold position\n");
		}
		app.DebugPrintf("===================================\n");
	}

	//	printf("Post processing complete at %dms\n",System::currentTimeMillis() - startTime);

	//	printf("Lighting complete at %dms\n",System::currentTimeMillis() - startTime);

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	if( levels[1]->isNew )
	{
		levels[1]->save(true, mcprogress);
	}

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	if( levels[2]->isNew )
	{
		levels[2]->save(true, mcprogress);
	}

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	// 4J - added - immediately save newly created level, like single player game
	// 4J Stu - We also want to immediately save the tutorial
	if ( levels[0]->isNew )
		saveGameRules();

	if( levels[0]->isNew )
	{
		levels[0]->save(true, mcprogress);
	}

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	if( levels[0]->isNew || levels[1]->isNew || levels[2]->isNew )
	{
#ifndef _WINDOWS64
		// On Windows64 we skip the automatic initial save so that choosing
		// "Exit without saving" on a new world does not leave an orphaned save folder.
		levels[0]->saveToDisc(mcprogress, false);
#endif
	}

	if( s_bServerHalted || !g_NetworkManager.IsInSession() ) return false;

	/*
	* int r = 24; for (int x = -r; x <= r; x++) {
	* setProgress("Preparing spawn area", (x + r) * 100 / (r + r + 1)); for (int z
	* = -r; z <= r; z++) { if (!running) return; level.cache.create((level.xSpawn
	* >> 4) + x, (level.zSpawn >> 4) + z); while (running && level.updateLights())
	* ; } }
	*/
	endProgress();

	return true;
}

#ifdef _LARGE_WORLDS
void MinecraftServer::overwriteBordersForNewWorldSize(ServerLevel* level)
{
	// recreate the chunks round the border (2 chunks or 32 blocks deep), deleting any player data from them
	app.DebugPrintf("Expanding level size\n");
	int oldSize = level->getLevelData()->getXZSizeOld();
	// top
	int minVal = -oldSize/2;
	int maxVal = (oldSize/2)-1;
	for(int xVal = minVal; xVal <= maxVal; xVal++)
	{
		int zVal = minVal;
		level->cache->overwriteLevelChunkFromSource(xVal, zVal);
		level->cache->overwriteLevelChunkFromSource(xVal, zVal+1);
	}
	// bottom
	for(int xVal = minVal; xVal <= maxVal; xVal++)
	{
		int zVal = maxVal;
		level->cache->overwriteLevelChunkFromSource(xVal, zVal);
		level->cache->overwriteLevelChunkFromSource(xVal, zVal-1);
	}
	// left
	for(int zVal = minVal; zVal <= maxVal; zVal++)
	{
		int xVal = minVal;
		level->cache->overwriteLevelChunkFromSource(xVal, zVal);
		level->cache->overwriteLevelChunkFromSource(xVal+1, zVal);
	}
	// right
	for(int zVal = minVal; zVal <= maxVal; zVal++)
	{
		int xVal = maxVal;
		level->cache->overwriteLevelChunkFromSource(xVal, zVal);
		level->cache->overwriteLevelChunkFromSource(xVal-1, zVal);
	}
}

void MinecraftServer::overwriteHellBordersForNewWorldSize(ServerLevel* level, int oldHellSize)
{
	// recreate the chunks round the border (1 chunk or 16 blocks deep), deleting any player data from them
	app.DebugPrintf("Expanding level size\n");
	// top
	int minVal = -oldHellSize/2;
	int maxVal = (oldHellSize/2)-1;
	for(int xVal = minVal; xVal <= maxVal; xVal++)
	{
		int zVal = minVal;
		level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal, maxVal);
	}
	// bottom
	for(int xVal = minVal; xVal <= maxVal; xVal++)
	{
		int zVal = maxVal;
		level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal, maxVal);
	}
	// left
	for(int zVal = minVal; zVal <= maxVal; zVal++)
	{
		int xVal = minVal;
		level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal, maxVal);
	}
	// right
	for(int zVal = minVal; zVal <= maxVal; zVal++)
	{
		int xVal = maxVal;
		level->cache->overwriteHellLevelChunkFromSource(xVal, zVal, minVal, maxVal);
	}
}

#endif

void MinecraftServer::setProgress(const wstring& status, int progress)
{
	progressStatus = status;
	this->progress = progress;
	//    logger.info(status + ": " + progress + "%");
}

void MinecraftServer::endProgress()
{
	progressStatus = L"";
	this->progress = 0;
}

void MinecraftServer::saveAllChunks()
{
	//    logger.info("Saving chunks");
	for (unsigned int i = 0; i < levels.length; i++)
	{
		// 4J Stu - Due to the way save mounting is handled on XboxOne, we can actually save after the player has signed out.
#ifndef _XBOX_ONE
		if( m_bPrimaryPlayerSignedOut ) break;
#endif
		// 4J Stu - Save the levels in reverse order so we don't overwrite the level.dat
		// with the data from the nethers leveldata.
		// Fix for #7418 - Functional: Gameplay: Saving after sleeping in a bed will place player at nighttime when restarting.
		ServerLevel *level = levels[levels.length - 1 - i];
		if( level )	// 4J - added check as level can be nullptr if we end up in stopServer really early on due to network failure
		{
#ifdef MINECRAFT_SERVER_BUILD
			level->save(true, nullptr);
#else
			level->save(true, Minecraft::GetInstance()->progressRenderer);
#endif

			// Only close the level storage when we have saved the last level, otherwise we need to recreate the region files
			// when saving the next levels
			if( i == (levels.length - 1))
			{
				level->closeLevelStorage();
			}
		}
	}
}

// 4J-JEV: Added
void MinecraftServer::saveGameRules()
{
#ifndef _CONTENT_PACKAGE
	if(app.DebugSettingsOn() && app.GetGameSettingsDebugMask(ProfileManager.GetPrimaryPad())&(1L<<eDebugSetting_DistributableSave))
	{
		// Do nothing
	}
	else
#endif
	{
		byteArray ba;
		ba.data = nullptr;
		app.m_gameRules.saveGameRules( &ba.data, &ba.length );

		if (ba.data != nullptr)
		{
			ConsoleSaveFile *csf = getLevel(0)->getLevelStorage()->getSaveFile();
			FileEntry *fe = csf->createFile(ConsoleSavePath(GAME_RULE_SAVENAME));
			csf->setFilePointer(fe, 0, nullptr, FILE_BEGIN);
			DWORD length;
			csf->writeFile(fe, ba.data, ba.length, &length );

			delete [] ba.data;

			csf->closeHandle(fe);
		}
	}
}

void MinecraftServer::Suspend()
{
	PIXBeginNamedEvent(0,"Suspending server");
	m_suspending = true;
	// Get the frequency of the timer
	LARGE_INTEGER qwTicksPerSec, qwTime, qwNewTime, qwDeltaTime;
	float fElapsedTime = 0.0f;
	QueryPerformanceFrequency( &qwTicksPerSec );
	float fSecsPerTick = 1.0f / static_cast<float>(qwTicksPerSec.QuadPart);
	// Save the start time
	QueryPerformanceCounter( &qwTime );
	if(m_bLoaded && ProfileManager.IsFullVersion() && (!StorageManager.GetSaveDisabled()))
	{
		if (players != nullptr)
		{
			players->saveAll(nullptr);
		}
		for (unsigned int j = 0; j < levels.length; j++)
		{
			if( s_bServerHalted ) break;
			// 4J Stu - Save the levels in reverse order so we don't overwrite the level.dat
			// with the data from the nethers leveldata.
			// Fix for #7418 - Functional: Gameplay: Saving after sleeping in a bed will place player at nighttime when restarting.
			ServerLevel *level = levels[levels.length - 1 - j];
			level->Suspend();
		}
		if( !s_bServerHalted )
		{
			saveGameRules();
			levels[0]->saveToDisc(nullptr, true);
		}
	}
	QueryPerformanceCounter( &qwNewTime );

	qwDeltaTime.QuadPart = qwNewTime.QuadPart - qwTime.QuadPart;
	fElapsedTime = fSecsPerTick * static_cast<FLOAT>(qwDeltaTime.QuadPart);

	// 4J-JEV: Flush stats and call PlayerSessionExit.
	for (int iPad = 0; iPad < XUSER_MAX_COUNT; iPad++)
	{
		if (ProfileManager.IsSignedIn(iPad))
		{
			TelemetryManager->RecordPlayerSessionExit(iPad, DisconnectPacket::eDisconnect_Quitting);
		}
	}

	m_suspending = false;
	app.DebugPrintf("Suspend server: Elapsed time %f\n", fElapsedTime);
	PIXEndNamedEvent();
}

bool MinecraftServer::IsSuspending()
{
	return m_suspending;
}

void MinecraftServer::stopServer(bool didInit)
{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	ServerRuntime::Access::Shutdown();
#endif

	// 4J-PB - need to halt the rendering of the data, since we're about to remove it
#ifdef __PS3__
	if( ShutdownManager::ShouldRun(ShutdownManager::eServerThread )	)		// This thread will take itself out if we are shutting down
#endif
	{
		Minecraft::GetInstance()->gameRenderer->DisableUpdateThread();
	}

	app.DebugPrintf("Stopping server\n");
	//    logger.info("Stopping server");
	// 4J-PB - If the primary player has signed out, then don't attempt to save anything

	connection->stop();

	// also need to check for a profile switch here - primary player signs out, and another player signs in before dismissing the dash
#ifdef _DURANGO
	// On Durango check if the primary user is signed in OR mid-sign-out
	if(ProfileManager.GetUser(0, true) != nullptr)
#else
	if((m_bPrimaryPlayerSignedOut==false) && ProfileManager.IsSignedIn(ProfileManager.GetPrimaryPad()))
#endif
	{
#if defined(_XBOX_ONE) || defined(__ORBIS__)
		// Always save on exit! Except if saves are disabled.
		if(!saveOnExitAnswered()) m_saveOnExit = true;
#endif

		// if trial version or saving is disabled, then don't save anything. Also don't save anything if we didn't actually get through the server initialisation.
		if(m_saveOnExit && ProfileManager.IsFullVersion() && (!StorageManager.GetSaveDisabled()) && didInit)
		{
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
			ConsoleLog(L"Kicking all players...");
			kickAllPlayers();

			// The main thread drains any pending background save (see
			// Windows64_Minecraft.cpp after StorageManager.Tick()); the wait
			// below (hasPendingBackgroundSave) blocks until it completes so the
			// compression thread won't access freed memory after teardown.
#endif
			if (!m_savePerformed)
			{
				ConsoleLog(L"Saving players...");
				if (players != nullptr)
				{
					players->saveAll(Minecraft::GetInstance()->progressRenderer, true);
				}
				ConsoleLog(L"Saving chunks...");
				saveAllChunks();

				ConsoleLog(L"Saving gamerules...");
				saveGameRules();
				app.m_gameRules.unloadCurrentGameRules();
				if( levels[0] != nullptr )
				{
					ConsoleLog(L"Saving level.dat...");
					levels[0]->saveToDisc(Minecraft::GetInstance()->progressRenderer, false);
				}
				ConsoleLog(L"World saved.");
			}
		}
		else
		{
			ConsoleLog(L"Discarding unsaved changes.");
		}
	}

	if (players != nullptr)
	{
		players->drainPendingDisconnects();
	}
	// reset the primary player signout flag
	m_bPrimaryPlayerSignedOut=false;
	s_bServerHalted = false;

	// On Durango/Orbis, we need to wait for all the asynchronous saving processes to complete before destroying the levels, as that will ultimately delete
	// the directory level storage & therefore the ConsoleSaveSplit instance, which needs to be around until all the sub files have completed saving.
#if defined(_DURANGO) || defined(__ORBIS__) || defined(__PSVITA__)
	while(StorageManager.GetSaveState() != C4JStorage::ESaveGame_Idle )
	{
		Sleep(10);
	}
#endif

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	// Wait for any background compression thread to finish before destroying levels.
	// Without this, delete levels[i] can destroy ConsoleSaveFileOriginal while its
	// background thread is still writing — use-after-free crash.
	{
		int waitCount = 0;
		while (ConsoleSaveFileOriginal::hasPendingBackgroundSave())
		{
			Sleep(10);
			if (++waitCount > 3000) { ConsoleLog(L"Warning: background save timed out"); break; }
		}
		// Background compaction completes on its own; no explicit flush needed.
	}
#endif

	FourKitBridge::Shutdown();

	// 4J-PB remove the server levels
	unsigned int iServerLevelC=levels.length;
	for (unsigned int i = 0; i < iServerLevelC; i++)
	{
		if(levels[i]!=nullptr)
		{
			delete levels[i];
			levels[i] = nullptr;
		}
	}

#if defined(__PS3__) || defined(__ORBIS__)
	// Clear the update flags as it's possible they could be out of sync, causing a crash when starting a new world after the first new level ticks
	// Fix for PS3 #1538 - [IN GAME] If the user 'Exit without saving' from inside the Nether or The End, the title can hang when loading back into the save.
#endif

	delete connection;
	connection = nullptr;
	delete players;
	players = nullptr;
	delete settings;
	settings = nullptr;

	g_NetworkManager.ServerStopped();
}

void MinecraftServer::halt()
{
	running = false;
}

void MinecraftServer::setMaxBuildHeight(int maxBuildHeight)
{
	this->maxBuildHeight = maxBuildHeight;
}

int MinecraftServer::getMaxBuildHeight()
{
	return maxBuildHeight;
}

PlayerList *MinecraftServer::getPlayers()
{
	return players;
}

void MinecraftServer::setPlayers(PlayerList *players)
{
	this->players = players;
}

ServerConnection *MinecraftServer::getConnection()
{
	return connection;
}

bool MinecraftServer::isAnimals()
{
	return animals;
}

void MinecraftServer::setAnimals(bool animals)
{
	this->animals = animals;
}

bool MinecraftServer::isNpcsEnabled()
{
	return npcs;
}

void MinecraftServer::setNpcsEnabled(bool npcs)
{
	this->npcs = npcs;
}

bool MinecraftServer::isPvpAllowed()
{
	return pvp;
}

void MinecraftServer::setPvpAllowed(bool pvp)
{
	this->pvp = pvp;
}

bool MinecraftServer::isFlightAllowed()
{
	return allowFlight;
}

void MinecraftServer::setFlightAllowed(bool allowFlight)
{
	this->allowFlight = allowFlight;
}

bool MinecraftServer::isCommandBlockEnabled()
{
	return false; //settings.getBoolean("enable-command-block", false);
}

bool MinecraftServer::isNetherEnabled()
{
	return true; //settings.getBoolean("allow-nether", true);
}

bool MinecraftServer::isHardcore()
{
	return app.GetGameHostOption(eGameHostOption_Hardcore) > 0;
}

int MinecraftServer::getOperatorUserPermissionLevel()
{
	return Command::LEVEL_OWNERS; //settings.getInt("op-permission-level", Command.LEVEL_OWNERS);
}

CommandDispatcher *MinecraftServer::getCommandDispatcher()
{
	return commandDispatcher;
}

Pos *MinecraftServer::getCommandSenderWorldPosition()
{
	return new Pos(0, 0, 0);
}

Level *MinecraftServer::getCommandSenderWorld()
{
	return levels[0];
}

int MinecraftServer::getSpawnProtectionRadius()
{
	// Client-host mode must never apply dedicated-server spawn protection settings.
	if (!ShouldUseDedicatedServerProperties()) return 0;
	return m_spawnProtectionRadius;
}

bool MinecraftServer::isUnderSpawnProtection(Level *level, int x, int y, int z, shared_ptr<Player> player)
{
	if (level->dimension->id != 0) return false;
	//if (getPlayers()->getOps()->empty()) return false;
	if (getPlayers()->isOp(player->getName())) return false;
	if (getSpawnProtectionRadius() <= 0) return false;

	Pos *spawnPos = level->getSharedSpawnPos();
	int xd = Mth::abs(x - spawnPos->x);
	int zd = Mth::abs(z - spawnPos->z);
	int dist = max(xd, zd);

	return dist <= getSpawnProtectionRadius();
}

void MinecraftServer::setForceGameType(bool forceGameType)
{
	this->forceGameType = forceGameType;
}

bool MinecraftServer::getForceGameType()
{
	return forceGameType;
}

int64_t MinecraftServer::getCurrentTimeMillis()
{
	return System::currentTimeMillis();
}

int MinecraftServer::getPlayerIdleTimeout()
{
	return playerIdleTimeout;
}

void MinecraftServer::setPlayerIdleTimeout(int playerIdleTimeout)
{
	this->playerIdleTimeout = playerIdleTimeout;
}

extern int c0a, c0b, c1a, c1b, c1c, c2a, c2b;
void MinecraftServer::run(int64_t seed, void *lpParameter)
{
    NetworkGameInitData *initData = nullptr;
    DWORD initSettings = 0;
    bool findSeed = false;
	if(lpParameter != nullptr)
    {
        initData = static_cast<NetworkGameInitData *>(lpParameter);
        initSettings = app.GetGameHostOption(eGameHostOption_All);
        findSeed = initData->findSeed;
        m_texturePackId = initData->texturePackId;
    }
    //    try {		// 4J - removed try/catch/finally
    bool didInit = false;
	if (initServer(seed, initData, initSettings,findSeed))
    {
        didInit = true;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
        ConsoleLog(L"Server tick loop starting...");
#endif
        ServerLevel *levelNormalDimension = levels[0];
        // 4J-PB - Set the Stronghold position in the leveldata if there isn't one in there
        Minecraft *pMinecraft = Minecraft::GetInstance();
		LevelData *pLevelData=levelNormalDimension->getLevelData();

		if(pLevelData && pLevelData->getHasStronghold()==false)
        {
			int x,z;
			if(app.GetTerrainFeaturePosition(eTerrainFeature_Stronghold,&x,&z))
            {
                pLevelData->setXStronghold(x);
                pLevelData->setZStronghold(z);
                pLevelData->setHasStronghold();
            }
        }

        int64_t lastTime = getCurrentTimeMillis();
        int64_t unprocessedTime = 0;
        while (running && !s_bServerHalted)
        {
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
            // Full wall-clock cost of one run loop iteration (catch-up ticks
            // + setTime handlers + XUI delayed actions + Sleep).
            int64_t outerIterStart = getCurrentTimeMillis();
            int64_t outerIterTickWork = 0;
#endif
            int64_t now = getCurrentTimeMillis();

            // 4J Stu - When we pause the server, we don't want to count that as time passed
            // 4J Stu - TU-1 hotifx - Remove this line. We want to make sure that we tick connections at the proper rate when paused
			//Fix for #13191 - The host of a game can get a message informing them that the connection to the server has been lost
			//if(m_isServerPaused) lastTime = now;

            int64_t passedTime = now - lastTime;
            if (passedTime > MS_PER_TICK * 40)
            {
                //                logger.warning("Can't keep up! Did the system time change, or is the server overloaded?");
                passedTime = MS_PER_TICK * 40;
            }
            if (passedTime < 0)
            {
                //                logger.warning("Time ran backwards! Did the system time change?");
                passedTime = 0;
            }
            unprocessedTime += passedTime;
            lastTime = now;

            // 4J Added ability to pause the server
			if( !m_isServerPaused )
            {
                bool didTick = false;
                if (levels[0]->allPlayersAreSleeping())
                {
                    tick();
                    unprocessedTime = 0;
                }
                else
                {
                    //					int tickcount = 0;
                    //					int64_t beforeall = System::currentTimeMillis();
                    while (unprocessedTime > MS_PER_TICK)
                    {
                        unprocessedTime -= MS_PER_TICK;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                        // Per-iteration pre/tick/post timing.
                        int64_t iter_t0 = System::currentTimeMillis();
#endif
                        chunkPacketManagement_PreTick();
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                        int64_t iter_t1 = System::currentTimeMillis();
#endif
                        tick();
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                        int64_t iter_t2 = System::currentTimeMillis();
#endif
                        chunkPacketManagement_PostTick();
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                        int64_t iter_t3 = System::currentTimeMillis();
                        int64_t iter_total = iter_t3 - iter_t0;
                        outerIterTickWork += iter_total;
                        if (iter_total > 200)
                        {
                            ServerRuntime::LogInfof("perf",
                                "iter total=%lldms pre=%lld tick=%lld post=%lld",
                                (long long)iter_total,
                                (long long)(iter_t1 - iter_t0),
                                (long long)(iter_t2 - iter_t1),
                                (long long)(iter_t3 - iter_t2));
                        }
#endif
                    }
                    // Do NOT reset lastTime here. Resetting discards the wall
                    // time spent in the catch-up so passedTime restarts from
                    // post-tick, capping effective TPS at 1000 / (MS_PER_TICK
                    // + avgTickBody). Runaway after a real freeze is bounded
                    // by the passedTime > MS_PER_TICK * 40 cap above.
                    //					int64_t afterall = System::currentTimeMillis();
                    //					PIXReportCounter(L"Server time all",(float)(afterall-beforeall));
                    //					PIXReportCounter(L"Server ticks",(float)tickcount);
                }
            }
            else
            {
                // 4J Stu - TU1-hotfix
				//Fix for #13191 - The host of a game can get a message informing them that the connection to the server has been lost
                // The connections should tick at the same frequency even when paused
                while (unprocessedTime > MS_PER_TICK)
                {
                    unprocessedTime -= MS_PER_TICK;
                    // Keep ticking the connections to stop them timing out
                    connection->tick();
                }
            }
			if(MinecraftServer::setTimeAtEndOfTick)
            {
                MinecraftServer::setTimeAtEndOfTick = false;
                for (unsigned int i = 0; i < levels.length; i++)
                {
                    //					if (i == 0 || settings->getBoolean(L"allow-nether", true))		// 4J removed - we always have nether
                    {
                        ServerLevel *level = levels[i];
						level->setGameTime( MinecraftServer::setTime );
                    }
                }
            }
			if(MinecraftServer::setTimeOfDayAtEndOfTick)
            {
                MinecraftServer::setTimeOfDayAtEndOfTick = false;
                for (unsigned int i = 0; i < levels.length; i++)
                {
                    if (i == 0 || GetDedicatedServerBool(settings, L"allow-nether", true))
                    {
                        ServerLevel *level = levels[i];
						level->setDayTime( MinecraftServer::setTimeOfDay );
                    }
                }
            }

            // Flush any pending background save (StorageManager I/O) before
            // The background save is drained on the main thread (see
            // Windows64_Minecraft.cpp, after StorageManager.Tick()), matching
            // LCE-Revelations' TickCoreSystems. The server thread never does
            // the blocking StorageManager I/O.

            // Process delayed actions
            eXuiServerAction eAction;
            LPVOID param;
			for(int i=0;i<XUSER_MAX_COUNT;i++)
            {
                eAction = app.GetXuiServerAction(i);
                param = app.GetXuiServerActionParam(i);

				switch(eAction)
                {
                case eXuiServerAction_AutoSaveGame:
                {
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                    // Never stack a new save on top of an in-flight one: a
                    // second Flush() would snapshot pvSaveMem while the
                    // background compression thread is still reading it.
                    if (ConsoleSaveFileOriginal::hasPendingBackgroundSave())
                    {
                        ConsoleLog(L"Save already in progress; skipping.");
                        break;
                    }

                    ConsoleSaveProgressLog(L"Saving players...", 10);
                    if (players != nullptr)
                        players->saveAll(nullptr);

                    ConsoleSaveProgressLog(L"Saving chunks...", 30);
                    for (unsigned int j = 0; j < levels.length; j++)
                    {
                        if( s_bServerHalted ) break;
                        ServerLevel *level = levels[levels.length - 1 - j];
                        level->save(false, nullptr, true);
                    }

                    if( !s_bServerHalted )
                    {
                        ConsoleSaveProgressLog(L"Saving gamerules...", 70);
                        saveGameRules();
                    }

                    ConsoleSaveProgressLog(L"Saving level.dat...", 85);
                    if( !s_bServerHalted && levels[0] != nullptr )
                        levels[0]->saveToDisc(Minecraft::GetInstance()->progressRenderer, true);

                    // "Save: World Saved." is printed by tick() once the
                    // background compression + disc write actually complete.
#endif
                    break;
                }
                case eXuiServerAction_SaveGame:
                {
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
                    bool isExitSave = m_exitAfterSave;

                    // Don't stack a save on an in-flight one; the shutdown
                    // drain / fallback save in stopServer handles completion.
                    if (ConsoleSaveFileOriginal::hasPendingBackgroundSave())
                    {
                        ConsoleLog(L"Save already in progress; skipping.");
                        m_exitAfterSave = false;
                        break;
                    }

                    if (!isExitSave)
                        broadcastStartSavingPacket();

                    ConsoleSaveProgressLog(L"Saving players...", 10);
                    if (players != nullptr)
                        players->saveAll(nullptr);

                    ConsoleSaveProgressLog(L"Saving chunks...", 30);
                    for (unsigned int j = 0; j < levels.length; j++)
                    {
                        if( s_bServerHalted ) break;
                        ServerLevel *level = levels[levels.length - 1 - j];
                        level->save(true, nullptr, false);
                    }

                    if( !s_bServerHalted )
                    {
                        ConsoleSaveProgressLog(L"Saving gamerules...", 70);
                        saveGameRules();
                    }

                    ConsoleSaveProgressLog(L"Saving level.dat...", 85);
                    if( !s_bServerHalted && levels[0] != nullptr )
                        levels[0]->saveToDisc(Minecraft::GetInstance()->progressRenderer, false);

                    // "Save: World Saved." is printed by tick() once the
                    // background compression + disc write actually complete.
                    if (!isExitSave)
                        broadcastStopSavingPacket();
                    if (isExitSave)
                    {
                        m_savePerformed = true;
                        app.SetAction(ProfileManager.GetPrimaryPad(), eAppAction_ExitWorld);
                    }
#else
                    bool isExitSave = m_exitAfterSave;
                    if (!isExitSave)
                        app.EnterSaveNotificationSection();

                    ConsoleProgressBar(0);
                    ConsoleLog(L"Saving players...");
                    if (players != nullptr)
                        players->saveAll(Minecraft::GetInstance()->progressRenderer);

                    if (!isExitSave)
                        players->broadcastAll(std::make_shared<UpdateProgressPacket>(20));
                    ConsoleProgressBar(20);

                    ConsoleLog(L"Saving chunks...");
                    for (unsigned int j = 0; j < levels.length; j++)
                    {
						if( s_bServerHalted ) break;
                        ServerLevel *level = levels[levels.length - 1 - j];
						level->save(true, Minecraft::GetInstance()->progressRenderer, false);

                        if (!isExitSave)
                            players->broadcastAll(std::make_shared<UpdateProgressPacket>(33 + (j * 33)));
                    }
                    ConsoleProgressBar(60);
					if( !s_bServerHalted )
                    {
                        ConsoleLog(L"Saving gamerules...");
                        saveGameRules();
                        ConsoleProgressBar(80);

                        ConsoleLog(L"Saving level.dat...");
						levels[0]->saveToDisc(Minecraft::GetInstance()->progressRenderer, false);
                        ConsoleProgressBar(90);
                    }
                    if (!isExitSave)
                        app.LeaveSaveNotificationSection();
                    ConsoleSaveFileOriginal::flushPendingBackgroundSave();
                    ConsoleProgressBar(100);
                    ConsoleLog(L"World saved.");
                    if (isExitSave)
                    {
                        m_exitAfterSave = false;
                        m_savePerformed = true;
                        app.SetAction(ProfileManager.GetPrimaryPad(), eAppAction_ExitWorld);
                    }
#endif
                    break;
                }
                case eXuiServerAction_DropItem:
                    // Find the player, and drop the id at their feet
                    {
                        shared_ptr<ServerPlayer> player = players->players.at(0);
						size_t id = (size_t) param;
                        player->drop(std::make_shared<ItemInstance>(id, 1, 0));
                    }
                    break;
                case eXuiServerAction_SpawnMob:
                    {
                        shared_ptr<ServerPlayer> player = players->players.at(0);
                        eINSTANCEOF factory = static_cast<eINSTANCEOF>((size_t)param);
						shared_ptr<Mob> mob = dynamic_pointer_cast<Mob>(EntityIO::newByEnumType(factory,player->level ));
						mob->moveTo(player->x+1, player->y, player->z+1, player->level->random->nextFloat() * 360, 0);
						mob->setDespawnProtected();		// 4J added, default to being protected against despawning (has to be done after initial position is set)
                        player->level->addEntity(mob);
                    }
                    break;
                case eXuiServerAction_PauseServer:
					m_isServerPaused = ( (size_t) param == TRUE );
					if( m_isServerPaused )
                    {
                        m_serverPausedEvent->Set();
                    }
                    break;
                case eXuiServerAction_ToggleRain:
                    {
                        bool isRaining = levels[0]->getLevelData()->isRaining();
                        levels[0]->getLevelData()->setRaining(!isRaining);
                        levels[0]->getLevelData()->setRainTime(levels[0]->random->nextInt(Level::TICKS_PER_DAY * 7) + Level::TICKS_PER_DAY / 2);
                    }
                    break;
                case eXuiServerAction_ToggleThunder:
                    {
                        bool isThundering = levels[0]->getLevelData()->isThundering();
                        levels[0]->getLevelData()->setThundering(!isThundering);
                        levels[0]->getLevelData()->setThunderTime(levels[0]->random->nextInt(Level::TICKS_PER_DAY * 7) + Level::TICKS_PER_DAY / 2);
                    }
                    break;
                case eXuiServerAction_ServerSettingChanged_Gamertags:
                    players->broadcastAll(std::make_shared<ServerSettingsChangedPacket>(ServerSettingsChangedPacket::HOST_OPTIONS, app.GetGameHostOption(eGameHostOption_Gamertags)));
                    break;
                case eXuiServerAction_ServerSettingChanged_BedrockFog:
                    players->broadcastAll(std::make_shared<ServerSettingsChangedPacket>(ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS, app.GetGameHostOption(eGameHostOption_All)));
                    break;

                case eXuiServerAction_ServerSettingChanged_Difficulty:
                    players->broadcastAll(std::make_shared<ServerSettingsChangedPacket>(ServerSettingsChangedPacket::HOST_DIFFICULTY, Minecraft::GetInstance()->options->difficulty));
                    break;
                case eXuiServerAction_ExportSchematic:
#ifndef _CONTENT_PACKAGE
                    app.EnterSaveNotificationSection();

					//players->broadcastAll( shared_ptr<UpdateProgressPacket>( new UpdateProgressPacket(20) ) );

					if( !s_bServerHalted )
                    {
                        ConsoleSchematicFile::XboxSchematicInitParam *initData = static_cast<ConsoleSchematicFile::XboxSchematicInitParam *>(param);
#ifdef _XBOX
                        File targetFileDir(File::pathRoot + File::pathSeparator + L"Schematics");
#else
                        File targetFileDir(L"Schematics");
#endif
						if(!targetFileDir.exists())	targetFileDir.mkdir();

                        wchar_t filename[128];
						swprintf(filename,128,L"%ls%dx%dx%d.sch",initData->name,(initData->endX - initData->startX + 1), (initData->endY - initData->startY + 1), (initData->endZ - initData->startZ + 1));

						File dataFile = File( targetFileDir, wstring(filename) );
						if(dataFile.exists()) dataFile._delete();
                        FileOutputStream fos = FileOutputStream(dataFile);
                        DataOutputStream dos = DataOutputStream(&fos);
                        ConsoleSchematicFile::generateSchematicFile(&dos, levels[0], initData->startX, initData->startY, initData->startZ, initData->endX, initData->endY, initData->endZ, initData->bSaveMobs, initData->compressionType);
                        dos.close();

                        delete initData;
                    }
                    app.LeaveSaveNotificationSection();
#endif
                    break;
                case eXuiServerAction_SetCameraLocation:
#ifndef _CONTENT_PACKAGE
                    {
                        DebugSetCameraPosition *pos = static_cast<DebugSetCameraPosition *>(param);

						app.DebugPrintf(	"DEBUG: Player=%i\n", pos->player );
						app.DebugPrintf(	"DEBUG: Teleporting to pos=(%f.2, %f.2, %f.2), looking at=(%f.2,%f.2)\n",
                                        pos->m_camX, pos->m_camY, pos->m_camZ,
							pos->m_yRot, pos->m_elev
							);

                        shared_ptr<ServerPlayer> player = players->players.at(pos->player);
						player->debug_setPosition(	pos->m_camX, pos->m_camY, pos->m_camZ,
							pos->m_yRot, pos->m_elev	);

                        // Doesn't work
						//player->setYHeadRot(pos->m_yRot);
						//player->absMoveTo(pos->m_camX, pos->m_camY, pos->m_camZ, pos->m_yRot, pos->m_elev);
                    }
#endif
                    break;
                }

				app.SetXuiServerAction(i,eXuiServerAction_Idle);
            }

            Sleep(1);
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
            int64_t outerIterTotal = getCurrentTimeMillis() - outerIterStart;

            // Distribution histogram (gated). Buckets every outer iter, dumps
            // the bucket counts + a self-computed TPS every ~10 seconds.
            if (ServerRuntime::g_serverPerfTrace)
            {
                static const int kBucketCount = 14;
                static int64_t s_bucketEdges[kBucketCount] = {
                    2, 5, 10, 20, 30, 40, 50, 60, 80, 100, 200, 500, 1000, INT64_MAX
                };
                static unsigned int s_buckets[kBucketCount] = {0};
                static int64_t s_histWindowStartMs = 0;
                static int     s_histWindowStartTick = 0;
                static int64_t s_histTotalIterMs = 0;
                static unsigned int s_histTotalIters = 0;
                static unsigned int s_histTickIters = 0;
                int64_t nowMsForHist = getCurrentTimeMillis();
                if (s_histWindowStartMs == 0)
                {
                    s_histWindowStartMs = nowMsForHist;
                    s_histWindowStartTick = (int)tickCount;
                }
                for (int b = 0; b < kBucketCount; b++)
                {
                    if (outerIterTotal <= s_bucketEdges[b])
                    {
                        s_buckets[b]++;
                        break;
                    }
                }
                s_histTotalIterMs += outerIterTotal;
                s_histTotalIters++;
                if (outerIterTickWork > 0) s_histTickIters++;
                int ticksThisWindow = (int)tickCount - s_histWindowStartTick;
                if (ticksThisWindow >= 200)
                {
                    int64_t windowMs = nowMsForHist - s_histWindowStartMs;
                    double calcTps = windowMs > 0 ? (ticksThisWindow * 1000.0) / windowMs : 0.0;
                    double avgIterMs = s_histTotalIters > 0 ? (double)s_histTotalIterMs / s_histTotalIters : 0.0;
                    ServerRuntime::LogInfof("perf",
                        "histogram window: %d ticks in %lldms calcTps=%.2f iters=%u tickIters=%u avgIter=%.2fms | "
                        "<=2:%u <=5:%u <=10:%u <=20:%u <=30:%u <=40:%u <=50:%u <=60:%u <=80:%u <=100:%u <=200:%u <=500:%u <=1000:%u >1000:%u",
                        ticksThisWindow, (long long)windowMs, calcTps,
                        s_histTotalIters, s_histTickIters, avgIterMs,
                        s_buckets[0], s_buckets[1], s_buckets[2], s_buckets[3],
                        s_buckets[4], s_buckets[5], s_buckets[6], s_buckets[7],
                        s_buckets[8], s_buckets[9], s_buckets[10], s_buckets[11],
                        s_buckets[12], s_buckets[13]);
                    for (int b = 0; b < kBucketCount; b++) s_buckets[b] = 0;
                    s_histWindowStartMs = nowMsForHist;
                    s_histWindowStartTick = (int)tickCount;
                    s_histTotalIterMs = 0;
                    s_histTotalIters = 0;
                    s_histTickIters = 0;
                }
            }

            if (outerIterTotal > 200)
            {
                ServerRuntime::LogInfof("perf",
                    "outerIter total=%lldms tickWork=%lld postTickOverhead=%lld",
                    (long long)outerIterTotal,
                    (long long)outerIterTickWork,
                    (long long)(outerIterTotal - outerIterTickWork));
            }
#endif
        }
    }
	//else
    //{
	//     while (running)
    //	{
	//        handleConsoleInputs();
    //		Sleep(10);
	//    }
	//}
#if 0
} catch (Throwable t) {
	t.printStackTrace();
	logger.log(Level.SEVERE, "Unexpected exception", t);
	while (running) {
		handleConsoleInputs();
		try {
			Thread.sleep(10);
		} catch (InterruptedException e1) {
			e1.printStackTrace();
		}
	}
} finally {
	try {
		stopServer();
		stopped = true;
	} catch (Throwable t) {
		t.printStackTrace();
	} finally {
		System::exit(0);
	}
}
#endif

    // 4J Stu - Stop the server when the loops complete, as the finally would do
    stopServer(didInit);
    stopped = true;
}

void MinecraftServer::broadcastStartSavingPacket()
{
	players->broadcastAll(std::make_shared<GameEventPacket>(GameEventPacket::START_SAVING, 0));;
}

void MinecraftServer::broadcastStopSavingPacket()
{
	players->broadcastAll(std::make_shared<GameEventPacket>(GameEventPacket::STOP_SAVING, 0));;
}

void MinecraftServer::tick()
{
	// Per-substep wall-clock timing. Logs one summary line when total tick
	// exceeds TICK_SLOW_THRESHOLD_MS.
	const int64_t TICK_SLOW_THRESHOLD_MS = 200;
	const int kMaxLevelsRecorded = 8;
	int64_t tickStart = System::currentTimeMillis();
	int64_t lvlTickMs[kMaxLevelsRecorded] = {0};
	int64_t lvlEntMs[kMaxLevelsRecorded]  = {0};
	int64_t lvlTrkMs[kMaxLevelsRecorded]  = {0};
	int     lvlDimId[kMaxLevelsRecorded]  = {0};
	unsigned int recordedLevels = 0;

	vector<wstring> toRemove;
    for ( auto& it : ironTimers )
    {
		int t = it.second;
		if (t > 0)
		{
			ironTimers[it.first] = t - 1;
		}
		else
		{
			toRemove.push_back(it.first);
		}
	}
	for (const auto& i : toRemove)
	{
		ironTimers.erase(i);
	}

	AABB::resetPool();
	Vec3::resetPool();

	tickCount++;

	// Drive the FourKit scheduler tick loop so managed tasks fire on time.
	FourKitBridge::ServerTickCallback(tickCount);

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	if (m_autosaveEnabled)
	{
		DWORD now = GetTickCount();
		if ((LONG)(now - m_autosaveNextTickMs) >= 0)
		{
			if (app.GetXuiServerAction(ProfileManager.GetPrimaryPad()) == eXuiServerAction_Idle
				&& !ConsoleSaveFileOriginal::hasPendingBackgroundSave())
			{
				app.SetXuiServerAction(ProfileManager.GetPrimaryPad(), eXuiServerAction_AutoSaveGame);
				FourKitBridge::FireWorldSave();
			}
			m_autosaveNextTickMs = now + m_autosaveIntervalMs;
		}
	}

	{
		// The heavy part of a save runs on a background thread (compress) then
		// the main thread (disc write); hasPendingBackgroundSave() stays true
		// the whole time. Each stage prints its own progress line; tick()
		// reports "Still saving..." while the write is in flight and only
		// reports completion once the write callback has actually fired.
		if (s_saveInProgress)
		{
			if (!ConsoleSaveFileOriginal::hasPendingBackgroundSave())
			{
				DWORD elapsed = GetTickCount() - s_saveStartTick;
				wchar_t line[128];
				swprintf_s(line, L"Save: World Saved. (%u.%u s)", elapsed / 1000, (elapsed / 100) % 10);
				ConsoleLog(line);
				s_saveInProgress = false;
				s_saveStartTick = 0;
				s_saveLastProgressTick = 0;
			}
			else
			{
				DWORD now = GetTickCount();
				if ((DWORD)(now - s_saveLastProgressTick) >= 10000)
				{
					s_saveLastProgressTick = now;
					DWORD elapsed = now - s_saveStartTick;
					wchar_t line[128];
					swprintf_s(line, L"Save: Still saving... (%u s)", elapsed / 1000);
					ConsoleLog(line);
				}
			}
		}
	}
#endif

	// 4J We need to update client difficulty levels based on the servers
	Minecraft *pMinecraft = Minecraft::GetInstance();
	// 4J-PB - sending this on the host changing the difficulty in the menus
	/*	if(m_lastSentDifficulty != pMinecraft->options->difficulty)
	{
	m_lastSentDifficulty = pMinecraft->options->difficulty;
	players->broadcastAll( shared_ptr<ServerSettingsChangedPacket>( new ServerSettingsChangedPacket( ServerSettingsChangedPacket::HOST_DIFFICULTY, pMinecraft->options->difficulty) ) );
	}*/

	for (unsigned int i = 0; i < levels.length; i++)
	{
		//        if (i == 0 || settings->getBoolean(L"allow-nether", true))		// 4J removed - we always have nether
		{
			ServerLevel *level = levels[i];

			// 4J Stu - We set the levels difficulty based on the minecraft options
			level->difficulty = app.GetGameHostOption(eGameHostOption_Difficulty); //pMinecraft->options->difficulty;

#if DEBUG_SERVER_DONT_SPAWN_MOBS
			level->setSpawnSettings(false, false);
#else
			level->setSpawnSettings(level->difficulty > 0 && !Minecraft::GetInstance()->isTutorial(), animals);
#endif

			if (tickCount % 20 == 0)
			{
				players->broadcastAll(std::make_shared<SetTimePacket>(level->getGameTime(), level->getDayTime(), level->getGameRules()->getBoolean(GameRules::RULE_DAYLIGHT)), level->dimension->id);
			}
			// #ifndef __PS3__
			static int64_t stc = 0;
			int64_t st0 = System::currentTimeMillis();
			PIXBeginNamedEvent(0,"Level tick %d",i);
			static_cast<Level *>(level)->tick();
			int64_t st1 = System::currentTimeMillis();
			PIXEndNamedEvent();
			PIXBeginNamedEvent(0,"Update lights %d",i);

			int64_t st2 = System::currentTimeMillis();
			PIXEndNamedEvent();
			PIXBeginNamedEvent(0,"Entity tick %d",i);
#ifdef __PSVITA__
			if ((players->getPlayerCount(level) > 0) || level->hasEntitiesToRemove())
			{
				// AP - the PlayerList->viewDistance initially starts out at 3 to make starting a level speedy
				// the problem with this is that spawned monsters are always generated on the edge of the known map
				// which means they wont process (unless they are surrounded by 2 visible chunks). This means
				// they wont checkDespawn so they are NEVER removed which results in monsters not spawning.
				// This bit of hack will modify the view distance once the level is up and running.
				int newViewDistance = 5;
				level->getServer()->getPlayers()->setViewDistance(newViewDistance);
				level->getTracker()->updateMaxRange();
				level->getChunkMap()->setRadius(level->getServer()->getPlayers()->getViewDistance());
			}
#endif
			level->tickEntities();
			PIXEndNamedEvent();

			int64_t stEntDone = System::currentTimeMillis();

			PIXBeginNamedEvent(0,"Entity tracker tick");
			level->getTracker()->tick();
			PIXEndNamedEvent();

			int64_t st3 = System::currentTimeMillis();
			//			printf(">>>>>>>>>>>>>>>>>>>>>> Tick %d %d %d : %d\n", st1 - st0, st2 - st1, st3 - st2, st0 - stc );
			stc = st0;
			// #endif// __PS3__

			// Record per-level breakdown for the slow-tick summary.
			if (i < kMaxLevelsRecorded)
			{
				lvlTickMs[i] = st1 - st0;          // Level::tick (mob spawner, chunk source, tile ticks, etc.)
				lvlEntMs[i]  = stEntDone - st2;    // tickEntities (per-entity AI/physics)
				lvlTrkMs[i]  = st3 - stEntDone;    // EntityTracker::tick (visibility & broadcasts)
				lvlDimId[i]  = level->dimension->id;
				recordedLevels = i + 1;
			}
		}
	}
	int64_t afterLevels = System::currentTimeMillis();
	Entity::tickExtraWandering();	// 4J added
	int64_t afterExtraW = System::currentTimeMillis();

	// Process player disconnect/kick queue BEFORE ticking connections.
	// PendingConnection::handleLogin rejects duplicate XUIDs, so the old
	// player must be removed from PlayerList before a reconnecting client's
	// LoginPacket is processed.
	PIXBeginNamedEvent(0,"Players tick");
	players->tick();
	PIXEndNamedEvent();
	int64_t afterPlayers = System::currentTimeMillis();
	PIXBeginNamedEvent(0,"Connection tick");
	connection->tick();
	PIXEndNamedEvent();
	int64_t afterConn = System::currentTimeMillis();

	// 4J - removed
#if 0
	for (size_t i = 0; i < tickables.size(); i++) {
		tickables.get(i)-tick();
	}
#endif

	//    try {		// 4J - removed try/catch
	handleConsoleInputs();
	//    } catch (Exception e) {
	//        logger.log(Level.WARNING, "Unexpected exception while parsing console command", e);
	//    }

	int64_t totalMs = System::currentTimeMillis() - tickStart;
#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
	if (totalMs > TICK_SLOW_THRESHOLD_MS)
	{
		// Build a single one-line breakdown so it greps cleanly. Per-level:
		// Level::tick / tickEntities / tracker tick. Then global subsystems.
		char buf[512];
		int n = 0;
		for (unsigned int i = 0; i < recordedLevels && n >= 0 && n < (int)sizeof(buf); i++)
		{
			n += snprintf(buf + n, sizeof(buf) - n,
				" L%d:tick=%lld ent=%lld trk=%lld",
				lvlDimId[i],
				(long long)lvlTickMs[i],
				(long long)lvlEntMs[i],
				(long long)lvlTrkMs[i]);
		}
		ServerRuntime::LogInfof("perf",
			"slow tick total=%lldms%s | extraW=%lld players=%lld conn=%lld",
			(long long)totalMs,
			buf,
			(long long)(afterExtraW  - afterLevels),
			(long long)(afterPlayers - afterExtraW),
			(long long)(afterConn    - afterPlayers));
	}
#else
	(void)totalMs;
#endif

	// TPS tracking
	m_tpsWindowTicks++;
	m_tpsWindowTicksMs += totalMs;
	int64_t now = System::currentTimeMillis();

	int64_t elapsed5s = now - m_tps5sStartMs;
	if (elapsed5s >= 5000)
	{
		m_tps5s = (m_tps5sTicks > 0) ? ((double)m_tps5sTicks * 1000.0 / (double)elapsed5s) : 20.0;
		m_tps5sTicks = 0;
		m_tps5sStartMs = now;
	}

	int64_t elapsed1m = now - m_tps1mStartMs;
	if (elapsed1m >= 60000)
	{
		m_tps1m = (m_tps1mTicks > 0) ? ((double)m_tps1mTicks * 1000.0 / (double)elapsed1m) : 20.0;
		m_tps1mTicks = 0;
		m_tps1mStartMs = now;
	}

	int64_t elapsed5m = now - m_tps5mStartMs;
	if (elapsed5m >= 300000)
	{
		m_tps5m = (m_tps5mTicks > 0) ? ((double)m_tps5mTicks * 1000.0 / (double)elapsed5m) : 20.0;
		m_tps5mTicks = 0;
		m_tps5mStartMs = now;
	}

	int64_t elapsed10m = now - m_tps10mStartMs;
	if (elapsed10m >= 600000)
	{
		m_tps10m = (m_tps10mTicks > 0) ? ((double)m_tps10mTicks * 1000.0 / (double)elapsed10m) : 20.0;
		m_tps10mTicks = 0;
		m_tps10mStartMs = now;
	}

	m_tps5sTicks++;
	m_tps1mTicks++;
	m_tps5mTicks++;
	m_tps10mTicks++;
}

void MinecraftServer::handleConsoleInput(const wstring& msg, ConsoleInputSource *source)
{
	EnterCriticalSection(&m_consoleInputCS);
	consoleInput.push_back(new ConsoleInput(msg, source));
	LeaveCriticalSection(&m_consoleInputCS);
}

bool MinecraftServer::executeConsoleCommandNow(const wstring& msg)
{
	return ExecuteConsoleCommand(this, msg);
}

void MinecraftServer::handleConsoleInputs()
{
	vector<ConsoleInput *> pendingInputs;
	EnterCriticalSection(&m_consoleInputCS);
	pendingInputs.swap(consoleInput);
	LeaveCriticalSection(&m_consoleInputCS);

	for (size_t i = 0; i < pendingInputs.size(); ++i)
	{
		ConsoleInput *input = pendingInputs[i];
		ExecuteConsoleCommand(this, input->msg);
		delete input;
	}
}

void MinecraftServer::main(int64_t seed, void *lpParameter)
{
#if __PS3__
	ShutdownManager::HasStarted(ShutdownManager::eServerThread );
#endif
	server = new MinecraftServer();
	server->run(seed, lpParameter);
	delete server;
	server = nullptr;
	ShutdownManager::HasFinished(ShutdownManager::eServerThread );
}

void MinecraftServer::HaltServer(bool bPrimaryPlayerSignedOut)
{
	s_bServerHalted = true;
	if( server != nullptr )
	{
		m_bPrimaryPlayerSignedOut=bPrimaryPlayerSignedOut;
		server->halt();
	}
}

#if defined(_WINDOWS64) && defined(MINECRAFT_SERVER_BUILD)
void MinecraftServer::kickAllPlayers()
{
	if (players == nullptr) return;
	auto snapshot = players->getPlayersSnapshot();
	for (auto &player : snapshot)
	{
		players->queueDisconnect(player, 0, L"Server shutting down", true, false);
	}
	players->drainPendingDisconnects();
}
#endif

File *MinecraftServer::getFile(const wstring& name)
{
	return new File(name);
}

void MinecraftServer::info(const wstring& string)
{
	PrintConsoleLine(L"[INFO]", string);
}

void MinecraftServer::warn(const wstring& string)
{
	PrintConsoleLine(L"[WARN]", string);
}

wstring MinecraftServer::getConsoleName()
{
	return L"CONSOLE";
}

ServerLevel *MinecraftServer::getLevel(int dimension)
{
	if (dimension == -1) return levels[1];
	else if (dimension == 1) return levels[2];
	else return levels[0];
}

// 4J added
void MinecraftServer::setLevel(int dimension, ServerLevel *level)
{
	if (dimension == -1) levels[1] = level;
	else if (dimension == 1) levels[2] = level;
	else levels[0] = level;
}

#if defined _ACK_CHUNK_SEND_THROTTLING
bool MinecraftServer::chunkPacketManagement_CanSendTo(INetworkPlayer *player)
{
	if( s_hasSentEnoughPackets ) return false;
	if( player == nullptr ) return false;

	for( size_t i = 0; i < s_sentTo.size(); i++ )
	{
		if( s_sentTo[i]->IsSameSystem(player) )
		{
			return false;
		}
	}

#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)
	return ( player->GetOutstandingAckCount() < 3 );
#else
	return ( player->GetOutstandingAckCount() < 2 );
#endif
}

void MinecraftServer::chunkPacketManagement_DidSendTo(INetworkPlayer *player)
{
	int64_t currentTime = System::currentTimeMillis();

	if( ( currentTime - s_tickStartTime ) >= MAX_TICK_TIME_FOR_PACKET_SENDS )
	{
		s_hasSentEnoughPackets = true;
//		app.DebugPrintf("Sending, setting enough packet flag: %dms\n",currentTime - s_tickStartTime);
	}
	else
	{
//		app.DebugPrintf("Sending, more time: %dms\n",currentTime - s_tickStartTime);
	}

	player->SentChunkPacket();

	s_sentTo.push_back(player);
}

void MinecraftServer::chunkPacketManagement_PreTick()
{
//	app.DebugPrintf("*************************************************************************************************************************************************************************\n");
	s_hasSentEnoughPackets = false;
	s_tickStartTime = System::currentTimeMillis();
	s_sentTo.clear();

	connection->sortPlayersByChunkPriority();
}

void MinecraftServer::chunkPacketManagement_PostTick()
{
}

#else
// 4J Added - round-robin chunk sends by player index. Compare vs the player at the current queue index,
// not GetSessionIndex() (smallId), so reused smallIds after many connect/disconnects still get chunk sends.
bool MinecraftServer::chunkPacketManagement_CanSendTo(INetworkPlayer *player)
{
	if( player == nullptr ) return false;

#ifdef MINECRAFT_SERVER_BUILD
	// Cap chunk-data sends per tick. Other players are served on later ticks
	// via the per-tick rotation in ServerConnection::tick.
	return s_dedicatedChunkSendsThisTick < DEDICATED_MAX_CHUNK_SENDS_PER_TICK;
#else
	int time = GetTickCount();
	DWORD currentPlayerCount = g_NetworkManager.GetPlayerCount();
	if( currentPlayerCount == 0 ) return false;
	int index = s_slowQueuePlayerIndex % (int)currentPlayerCount;
	INetworkPlayer *queuePlayer = g_NetworkManager.GetPlayerByIndex( index );
	if( queuePlayer != NULL && (player == queuePlayer || player->IsSameSystem(queuePlayer)) && (time - s_slowQueueLastTime) > MINECRAFT_SERVER_SLOW_QUEUE_DELAY )
	{
		return true;
	}

	return false;
#endif
}

void MinecraftServer::chunkPacketManagement_DidSendTo(INetworkPlayer *player)
{
	s_slowQueuePacketSent = true;
#ifdef MINECRAFT_SERVER_BUILD
	s_dedicatedChunkSendsThisTick++;
#endif
}

void MinecraftServer::chunkPacketManagement_PreTick()
{
#ifdef MINECRAFT_SERVER_BUILD
	s_dedicatedChunkSendsThisTick = 0;
#endif
}

void MinecraftServer::chunkPacketManagement_PostTick()
{
	// 4J Ensure that the slow queue owner keeps cycling if it's not been used in a while
	int time = GetTickCount();
	if( ( s_slowQueuePacketSent ) || (  (time - s_slowQueueLastTime) > ( 2 * MINECRAFT_SERVER_SLOW_QUEUE_DELAY ) ) )
	{
//		app.DebugPrintf("Considering cycling: (%d) %d - %d -> %d > %d\n",s_slowQueuePacketSent, time, s_slowQueueLastTime, (time - s_slowQueueLastTime), (2*MINECRAFT_SERVER_SLOW_QUEUE_DELAY));
		MinecraftServer::cycleSlowQueueIndex();
		s_slowQueuePacketSent = false;
		s_slowQueueLastTime = time;
	}
//	else
//	{
//		app.DebugPrintf("Not considering cycling: %d - %d -> %d > %d\n",time, s_slowQueueLastTime, (time - s_slowQueueLastTime), (2*MINECRAFT_SERVER_SLOW_QUEUE_DELAY));
//	}
}

void MinecraftServer::cycleSlowQueueIndex()
{
	if( !g_NetworkManager.IsInSession() ) return;

	int startingIndex = s_slowQueuePlayerIndex;
	INetworkPlayer *currentPlayer = nullptr;
	DWORD currentPlayerCount = 0;
	do
	{
		currentPlayerCount = g_NetworkManager.GetPlayerCount();
		if( startingIndex >= currentPlayerCount ) startingIndex = 0;
		++s_slowQueuePlayerIndex;

		if( currentPlayerCount > 0 )
		{
			s_slowQueuePlayerIndex %= currentPlayerCount;
			// Fix for #9530 - NETWORKING: Attempting to fill a multiplayer game beyond capacity results in a softlock for the last players to join.
			// The QNet session might be ending while we do this, so do a few more checks that the player is real
			currentPlayer = g_NetworkManager.GetPlayerByIndex( s_slowQueuePlayerIndex );
		}
		else
		{
			s_slowQueuePlayerIndex = 0;
		}
	} while ( g_NetworkManager.IsInSession() &&
		currentPlayerCount > 0 &&
		s_slowQueuePlayerIndex != startingIndex &&
		currentPlayer != nullptr &&
		currentPlayer->IsLocal()
		);
//	app.DebugPrintf("Cycled slow queue index to %d\n", s_slowQueuePlayerIndex);
}
#endif

// 4J added - sets up a vector of flags to indicate which entities (with small Ids) have been removed from the level, but are still haven't constructed a network packet
// to tell a remote client about it. These small Ids shouldn't be re-used. Most of the time this method shouldn't actually do anything, in which case it will return false
// and nothing is set up.
bool MinecraftServer::flagEntitiesToBeRemoved(unsigned int *flags)
{
	bool removedFound = false;
	for( unsigned int i = 0; i < levels.length; i++ )
	{
		ServerLevel *level = levels[i];
		if( level )
		{
			level->flagEntitiesToBeRemoved( flags, &removedFound );
		}
	}
	return removedFound;
}
