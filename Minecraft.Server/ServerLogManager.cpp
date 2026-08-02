#include "stdafx.h"

#include "ServerLogManager.h"

#include "Common/StringUtils.h"
#include "ServerLogger.h"

#include <array>
#include <mutex>
#include <unordered_map>

extern bool g_Win64DedicatedServer;

namespace ServerRuntime
{
	namespace ServerLogManager
	{
		namespace
		{
			/**
			 * **!! This information is managed solely for logging purposes, but it is questionable from a liability perspective, so it will eventually need to be separated !!**
			 * 
			 * Tracks the remote IP and accepted player name associated with one `smallId`
			 * 1つのsmallIdに紐づく接続IPとプレイヤー名を保持する
			 */
			struct ConnectionLogEntry
			{
				std::string remoteIp;
				std::string playerName;
				PlayerUID offlineXuid = INVALID_XUID;
				PlayerUID onlineXuid = INVALID_XUID;
				bool isGuest = false;
				bool cipherActive = false;
				bool tokenIssued = false;
				bool tokenVerified = false;
			};

			/**
			 * Owns the shared connection cache used by hook points running on different threads
			 * 複数スレッドのhookから共有される接続キャッシュを保持する
			 */
			struct ServerLogState
			{
				std::mutex stateLock;
				std::array<ConnectionLogEntry, 256> entries;

				// Name->XUIDs cache from recent login attempts (case-insensitive name key)
				// Multiple XUIDs per name for ambiguity detection
				std::unordered_map<std::string, std::vector<PlayerUID>> nameToXuidCache;
			};

			ServerLogState g_serverLogState;

			static bool IsDedicatedServerLoggingEnabled()
			{
				return g_Win64DedicatedServer;
			}

			static void ResetConnectionLogEntry(ConnectionLogEntry *entry)
			{
				if (entry == NULL)
				{
					return;
				}

				entry->remoteIp.clear();
				entry->playerName.clear();
				entry->offlineXuid = INVALID_XUID;
				entry->onlineXuid = INVALID_XUID;
				entry->isGuest = false;
				entry->cipherActive = false;
				entry->tokenIssued = false;
				entry->tokenVerified = false;
			}

			static std::string NormalizeRemoteIp(const char *ip)
			{
				if (ip == NULL || ip[0] == 0)
				{
					return std::string("unknown");
				}

				return std::string(ip);
			}

			static std::string NormalizePlayerName(const std::wstring &playerName)
			{
				std::string playerNameUtf8 = StringUtils::WideToUtf8(playerName);
				if (playerNameUtf8.empty())
				{
					playerNameUtf8 = "<unknown>";
				}

				return playerNameUtf8;
			}

            // Default to the main app channel when the caller does not provide a source tag.
            static const char *NormalizeClientLogSource(const char *source)
            {
                if (source == NULL || source[0] == 0)
                {
                    return "app";
                }

                return source;
            }

            static void EmitClientDebugLogLine(const char *source, const std::string &line)
            {
                if (line.empty())
                {
                    return;
                }

                LogDebugf("client", "[%s] %s", NormalizeClientLogSource(source), line.c_str());
            }

            // Split one debug payload into individual lines so each line becomes a prompt-safe server log entry.
            static void ForwardClientDebugMessage(const char *source, const char *message)
            {
                if (message == NULL || message[0] == 0)
                {
                    return;
                }

                const char *cursor = message;
                while (*cursor != 0)
                {
                    const char *lineStart = cursor;
                    while (*cursor != 0 && *cursor != '\r' && *cursor != '\n')
                    {
                        ++cursor;
                    }

                    // Split multi-line client debug output into prompt-safe server log entries.
                    if (cursor > lineStart)
                    {
                        EmitClientDebugLogLine(source, std::string(lineStart, (size_t)(cursor - lineStart)));
                    }

                    while (*cursor == '\r' || *cursor == '\n')
                    {
                        ++cursor;
                    }
                }
            }

            // Share the same formatting path for app, user, and legacy debug-spew forwards.
            static void ForwardFormattedClientDebugLogV(const char *source, const char *format, va_list args)
            {
                if (!IsDedicatedServerLoggingEnabled() || format == NULL || format[0] == 0)
                {
                    return;
                }

                char messageBuffer[2048] = {};
                vsnprintf_s(messageBuffer, sizeof(messageBuffer), _TRUNCATE, format, args);
                ForwardClientDebugMessage(source, messageBuffer);
            }

            static const char *TcpRejectReasonToString(ETcpRejectReason reason)
			{
				switch (reason)
				{
				case eTcpRejectReason_BannedIp: return "banned-ip";
				case eTcpRejectReason_GameNotReady: return "game-not-ready";
				case eTcpRejectReason_ServerFull: return "server-full";
				case eTcpRejectReason_RateLimited: return "rate-limited";
				case eTcpRejectReason_TooManyPending: return "too-many-pending";
				case eTcpRejectReason_InvalidProxyHeader: return "invalid-proxy-header";
				default: return "unknown";
				}
			}

			static const char *LoginRejectReasonToString(ELoginRejectReason reason)
			{
				switch (reason)
				{
				case eLoginRejectReason_BannedXuid: return "banned-xuid";
				case eLoginRejectReason_NotWhitelisted: return "not-whitelisted";
				case eLoginRejectReason_DuplicateXuid: return "duplicate-xuid";
				case eLoginRejectReason_DuplicateName: return "duplicate-name";
				default: return "unknown";
				}
			}

			static const char *DisconnectReasonToString(DisconnectPacket::eDisconnectReason reason)
			{
				switch (reason)
				{
				case DisconnectPacket::eDisconnect_None: return "none";
				case DisconnectPacket::eDisconnect_Quitting: return "quitting";
				case DisconnectPacket::eDisconnect_Closed: return "closed";
				case DisconnectPacket::eDisconnect_LoginTooLong: return "login-too-long";
				case DisconnectPacket::eDisconnect_IllegalStance: return "illegal-stance";
				case DisconnectPacket::eDisconnect_IllegalPosition: return "illegal-position";
				case DisconnectPacket::eDisconnect_MovedTooQuickly: return "moved-too-quickly";
				case DisconnectPacket::eDisconnect_NoFlying: return "no-flying";
				case DisconnectPacket::eDisconnect_Kicked: return "kicked";
				case DisconnectPacket::eDisconnect_TimeOut: return "timeout";
				case DisconnectPacket::eDisconnect_Overflow: return "overflow";
				case DisconnectPacket::eDisconnect_EndOfStream: return "end-of-stream";
				case DisconnectPacket::eDisconnect_ServerFull: return "server-full";
				case DisconnectPacket::eDisconnect_OutdatedServer: return "outdated-server";
				case DisconnectPacket::eDisconnect_OutdatedClient: return "outdated-client";
				case DisconnectPacket::eDisconnect_UnexpectedPacket: return "unexpected-packet";
				case DisconnectPacket::eDisconnect_ConnectionCreationFailed: return "connection-creation-failed";
				case DisconnectPacket::eDisconnect_NoMultiplayerPrivilegesHost: return "no-multiplayer-privileges-host";
				case DisconnectPacket::eDisconnect_NoMultiplayerPrivilegesJoin: return "no-multiplayer-privileges-join";
				case DisconnectPacket::eDisconnect_NoUGC_AllLocal: return "no-ugc-all-local";
				case DisconnectPacket::eDisconnect_NoUGC_Single_Local: return "no-ugc-single-local";
				case DisconnectPacket::eDisconnect_ContentRestricted_AllLocal: return "content-restricted-all-local";
				case DisconnectPacket::eDisconnect_ContentRestricted_Single_Local: return "content-restricted-single-local";
				case DisconnectPacket::eDisconnect_NoUGC_Remote: return "no-ugc-remote";
				case DisconnectPacket::eDisconnect_NoFriendsInGame: return "no-friends-in-game";
				case DisconnectPacket::eDisconnect_Banned: return "banned";
				case DisconnectPacket::eDisconnect_NotFriendsWithHost: return "not-friends-with-host";
				case DisconnectPacket::eDisconnect_NATMismatch: return "nat-mismatch";
				default: return "unknown";
				}
			}
		}

        // Only forward client-side debug output while the process is running as the dedicated server.
        bool ShouldForwardClientDebugLogs()
        {
            return IsDedicatedServerLoggingEnabled();
        }

        void ForwardClientAppDebugLogV(const char *format, va_list args)
        {
            ForwardFormattedClientDebugLogV("app", format, args);
        }

        void ForwardClientUserDebugLogV(int user, const char *format, va_list args)
        {
            char source[32] = {};
            _snprintf_s(source, sizeof(source), _TRUNCATE, "app:user=%d", user);
            ForwardFormattedClientDebugLogV(source, format, args);
        }

        void ForwardClientDebugSpewLogV(const char *format, va_list args)
        {
            ForwardFormattedClientDebugLogV("debug-spew", format, args);
        }

        // Clear every cached connection slot during startup so stale metadata never leaks into future logs.
        void Initialize()
		{
			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			for (size_t index = 0; index < g_serverLogState.entries.size(); ++index)
			{
				ResetConnectionLogEntry(&g_serverLogState.entries[index]);
			}
		}

			// Reuse Initialize as the shutdown cleanup path because both operations wipe the cache.
		void Shutdown()
		{
			Initialize();
		}

		// Log the raw socket arrival before a smallId is assigned so early rejects still have an IP in the logs.
		void OnIncomingTcpConnection(const char *ip)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			const std::string remoteIp = NormalizeRemoteIp(ip);
			LogInfof("network", "incoming tcp connection from %s", remoteIp.c_str());
		}

		// TCP rejects happen before connection state is cached, so log directly from the supplied remote IP.
		void OnRejectedTcpConnection(const char *ip, ETcpRejectReason reason)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			const std::string remoteIp = NormalizeRemoteIp(ip);
			LogWarnf("network", "rejected tcp connection from %s: reason=%s", remoteIp.c_str(), TcpRejectReasonToString(reason));
		}

			// Cache the accepted remote IP immediately so later login and disconnect logs can reuse it.
		void OnAcceptedTcpConnection(unsigned char smallId, const char *ip)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			const std::string remoteIp = NormalizeRemoteIp(ip);
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
				ResetConnectionLogEntry(&entry);
				entry.remoteIp = remoteIp;
			}

			LogInfof("network", "accepted tcp connection from %s as smallId=%u", remoteIp.c_str(), (unsigned)smallId);
		}

		static std::string FormatXuidForLog(PlayerUID xuid)
		{
			if (xuid == INVALID_XUID) return "none";
			char buf[32] = {};
			sprintf_s(buf, sizeof(buf), "0x%016llx", (unsigned long long)xuid);
			return buf;
		}

		// Once login succeeds, bind the resolved player name and identity onto the cached transport entry.
		void OnAcceptedPlayerLogin(unsigned char smallId, const std::wstring &playerName,
			PlayerUID offlineXuid, PlayerUID onlineXuid, bool isGuest)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			const std::string playerNameUtf8 = NormalizePlayerName(playerName);
			std::string remoteIp("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
				entry.playerName = playerNameUtf8;
				entry.offlineXuid = offlineXuid;
				entry.onlineXuid = onlineXuid;
				entry.isGuest = isGuest;
				if (!entry.remoteIp.empty())
				{
					remoteIp = entry.remoteIp;
				}
			}

			std::string xuidStr = FormatXuidForLog(offlineXuid);
			std::string logMsg = "accepted player login: name=\"" + playerNameUtf8 +
				"\" ip=" + remoteIp +
				" xuid=" + xuidStr;
			if (onlineXuid != INVALID_XUID && onlineXuid != offlineXuid)
			{
				logMsg += " online-xuid=" + FormatXuidForLog(onlineXuid);
			}
			if (isGuest)
			{
				logMsg += " guest=yes";
			}
			logMsg += " smallId=" + std::to_string((unsigned)smallId);
			LogInfof("network", "%s", logMsg.c_str());
		}

		// Read the cached IP for the rejection log, then clear the slot because the player never fully joined.
		void OnRejectedPlayerLogin(unsigned char smallId, const std::wstring &playerName, ELoginRejectReason reason)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			const std::string playerNameUtf8 = NormalizePlayerName(playerName);
			std::string remoteIp("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty())
				{
					remoteIp = entry.remoteIp;
				}
				ResetConnectionLogEntry(&entry);
			}

			LogWarnf("network", "rejected login from %s: name=\"%s\" reason=%s", remoteIp.c_str(), playerNameUtf8.c_str(), LoginRejectReasonToString(reason));
		}

		// Disconnect logging is the final consumer of cached metadata, so it also clears the slot afterward.
		void OnPlayerDisconnected(
			unsigned char smallId,
			const std::wstring &playerName,
			DisconnectPacket::eDisconnectReason reason,
			bool initiatedByServer)
		{
			if (!IsDedicatedServerLoggingEnabled())
			{
				return;
			}

			std::string playerNameUtf8 = NormalizePlayerName(playerName);
			std::string remoteIp("unknown");
			{
				// Copy state under lock and emit the log after unlocking so CLI output never blocks connection bookkeeping.
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty())
				{
					remoteIp = entry.remoteIp;
				}
				if (playerNameUtf8 == "<unknown>" && !entry.playerName.empty())
				{
					playerNameUtf8 = entry.playerName;
				}
				ResetConnectionLogEntry(&entry);
			}

			LogInfof(
				"network",
				"%s: name=\"%s\" ip=%s smallId=%u reason=%s",
				initiatedByServer ? "disconnecting player" : "player disconnected",
				playerNameUtf8.c_str(),
				remoteIp.c_str(),
				(unsigned)smallId,
				DisconnectReasonToString(reason));
		}

		/** 
		 * For logging purposes, the responsibility is technically misplaced, but the IP is cached in `LogManager`.
		 * Those cached values are then used to retrieve the player's IP.
		 * 
		 * Eventually, this should be implemented in a separate class or on the `Minecraft.Client` side instead.
		 */
		bool TryGetConnectionRemoteIp(unsigned char smallId, std::string *outIp)
		{
			if (!IsDedicatedServerLoggingEnabled() || outIp == NULL)
			{
				return false;
			}

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			const ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
			if (entry.remoteIp.empty() || entry.remoteIp == "unknown")
			{
				return false;
			}

			*outIp = entry.remoteIp;
			return true;
		}

		// Provide explicit cache cleanup for paths that terminate without going through disconnect logging.
		void ClearConnection(unsigned char smallId)
		{
			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			ResetConnectionLogEntry(&g_serverLogState.entries[smallId]);
		}

		// ---- Security milestone tracking ----

		static void TryEmitPlayerSecuredSummary(unsigned char smallId, const ConnectionLogEntry &entry)
		{
			// Only emit when cipher is confirmed active (the primary security gate)
			if (!entry.cipherActive) return;
			// If tokens are required, wait until token status is resolved
			if (!entry.tokenIssued && !entry.tokenVerified) return;

			const char *tokenStatus = entry.tokenVerified ? "verified" : (entry.tokenIssued ? "issued" : "n/a");
			std::string xuidStr = FormatXuidForLog(entry.offlineXuid);
			std::string logMsg = "player secured: name=\"" + entry.playerName +
				"\" xuid=" + xuidStr +
				" ip=" + (entry.remoteIp.empty() ? "unknown" : entry.remoteIp) +
				" cipher=active token=" + tokenStatus;
			if (entry.isGuest)
			{
				logMsg += " guest=yes";
			}
			LogInfof("security", "%s", logMsg.c_str());
		}

		void OnCipherHandshakeCompleted(unsigned char smallId)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
			entry.cipherActive = true;

			// If tokens are not required, emit summary now
			// (check if player name is cached -- it should be by this point)
			if (!entry.playerName.empty())
			{
				// Defer: token status may still arrive. Summary emits from token methods
				// or if tokens are disabled, we need to check config.
				// For simplicity: always defer to token methods. If tokens are disabled,
				// the caller in PlayerList.cpp will call a direct emit.
			}
		}

		void OnCipherCompletedNoTokenRequired(unsigned char smallId)
		{
			// Called when cipher completes and require-challenge-token is false
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
			entry.cipherActive = true;

			if (!entry.playerName.empty())
			{
				std::string xuidStr = FormatXuidForLog(entry.offlineXuid);
				LogInfof("security", "player secured: name=\"%s\" xuid=%s ip=%s cipher=active token=n/a%s",
					entry.playerName.c_str(), xuidStr.c_str(),
					entry.remoteIp.empty() ? "unknown" : entry.remoteIp.c_str(),
					entry.isGuest ? " guest=yes" : "");
			}
		}

		void OnIdentityTokenIssued(unsigned char smallId)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
			entry.tokenIssued = true;
			TryEmitPlayerSecuredSummary(smallId, entry);
		}

		void OnIdentityTokenVerified(unsigned char smallId)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			ConnectionLogEntry &entry = g_serverLogState.entries[smallId];
			entry.tokenVerified = true;
			TryEmitPlayerSecuredSummary(smallId, entry);
		}

		void OnIdentityTokenMismatch(unsigned char smallId, const std::wstring &playerName)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::string name = NormalizePlayerName(playerName);
			std::string ip("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				const auto &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty()) ip = entry.remoteIp;
			}
			LogWarnf("security", "identity token mismatch for player \"%s\" (ip=%s) - use: revoketoken %s",
				name.c_str(), ip.c_str(), name.c_str());
		}

		void OnIdentityTokenTimeout(unsigned char smallId, const std::wstring &playerName)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::string name = NormalizePlayerName(playerName);
			std::string ip("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				const auto &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty()) ip = entry.remoteIp;
			}
			LogWarnf("security", "kicked player \"%s\" (ip=%s) - identity token response timed out",
				name.c_str(), ip.c_str());
		}

		void OnUnsecuredClientKicked(unsigned char smallId)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::string ip("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				const auto &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty()) ip = entry.remoteIp;
			}
			LogWarnf("security", "kicked unsecured client (smallId=%u, ip=%s) - cipher handshake timed out",
				(unsigned)smallId, ip.c_str());
		}

		void OnXuidSpoofDetected(unsigned char smallId, const std::wstring &claimedName,
			const char *newIp, const char *existingIp)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::string name = NormalizePlayerName(claimedName);
			LogWarnf("security", "XUID spoof suspected for \"%s\" - new IP %s conflicts with existing IP %s",
				name.c_str(),
				(newIp != nullptr) ? newIp : "unknown",
				(existingIp != nullptr) ? existingIp : "unknown");
		}

		void OnUnauthorizedCommand(unsigned char smallId, const std::wstring &playerName, const char *action)
		{
			if (!IsDedicatedServerLoggingEnabled()) return;

			std::string name = NormalizePlayerName(playerName);
			std::string ip("unknown");
			{
				std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
				const auto &entry = g_serverLogState.entries[smallId];
				if (!entry.remoteIp.empty()) ip = entry.remoteIp;
			}
			LogWarnf("security", "non-OP player \"%s\" attempted %s (ip=%s)",
				name.c_str(), (action != nullptr) ? action : "unknown-action", ip.c_str());
		}

		// ---- Name-to-XUID cache ----

		// Normalize a player name for cache key consistency (lowercase + trim)
		static std::string NormalizeNameKey(const std::string &name)
		{
			return StringUtils::ToLowerAscii(StringUtils::TrimAscii(name));
		}

		// Maximum entries in the name->XUID cache to prevent unbounded growth
		static const size_t kMaxCacheEntries = 256;
		// Maximum XUIDs tracked per name
		static const size_t kMaxXuidsPerName = 8;

		void CachePlayerXuid(const std::wstring &playerName, PlayerUID xuid)
		{
			if (playerName.empty() || xuid == INVALID_XUID)
			{
				return;
			}

			// Note: playerName is from the LoginPacket and is attacker-controlled.
			// This cache is an operator convenience tool for `whitelist add <name>`,
			// not a security mechanism. The operator sees the resolved XUID and can
			// verify it before whitelisting. Ambiguous names are blocked.
			std::string key = NormalizeNameKey(StringUtils::WideToUtf8(playerName));

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);

			// Evict oldest cache entry if at capacity
			if (g_serverLogState.nameToXuidCache.size() >= kMaxCacheEntries &&
				g_serverLogState.nameToXuidCache.find(key) == g_serverLogState.nameToXuidCache.end())
			{
				g_serverLogState.nameToXuidCache.erase(g_serverLogState.nameToXuidCache.begin());
			}

			auto &entries = g_serverLogState.nameToXuidCache[key];
			// Move matching XUID to the back (most recent) or append if new
			for (auto it = entries.begin(); it != entries.end(); ++it)
			{
				if (*it == xuid)
				{
					entries.erase(it);
					break;
				}
			}
			entries.push_back(xuid);
			// Cap per-name vector
			while (entries.size() > kMaxXuidsPerName)
			{
				entries.erase(entries.begin());
			}
		}

		int GetCachedXuids(const std::string &playerName, std::vector<PlayerUID> *outXuids)
		{
			if (playerName.empty())
			{
				if (outXuids != nullptr) outXuids->clear();
				return 0;
			}

			std::string key = NormalizeNameKey(playerName);

			std::lock_guard<std::mutex> stateLock(g_serverLogState.stateLock);
			auto it = g_serverLogState.nameToXuidCache.find(key);
			if (it == g_serverLogState.nameToXuidCache.end() || it->second.empty())
			{
				if (outXuids != nullptr) outXuids->clear();
				return 0;
			}

			if (outXuids != nullptr)
			{
				*outXuids = it->second;
			}
			return static_cast<int>(it->second.size());
		}
	}
}
