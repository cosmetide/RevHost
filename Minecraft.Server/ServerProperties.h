#pragma once

#include <string>
#include "ServerLogger.h"

namespace ServerRuntime
{
	/**
	 * `server.properties`
	 */
	struct ServerPropertiesConfig
	{
		/** `server-port` */
		int serverPort;
		/** `server-ip` */
		std::string serverIp;
		/** `lan-advertise` */
		bool lanAdvertise;
		/** `white-list` */
		bool whiteListEnabled;
		/** `lan-server-name` (max 16 chars at runtime) */
		std::string lanServerName;
		/** `max-players` */
		int maxPlayers;

	/** `autosave-interval` — suffixed value like \"5m\", \"300s\" or \"1h\" (1s–24h, default 5m) */
	int autosaveIntervalSeconds;
	/** `autosave` — enable periodic autosave */
	bool autosave;

		/** host options / game settings */
		int difficulty;
		/** `spawn-protection` radius in blocks (0 disables protection) */
		int spawnProtectionRadius;
		bool generateStructures;
		bool pvp;
		bool trustPlayers;
		bool fireSpreads;
		bool tnt;
		bool spawnAnimals;
		bool spawnNpcs;
		bool spawnMonsters;
		bool allowFlight;
		bool allowNether;
		bool hostCanFly;
		bool hostCanChangeHunger;
		bool hostCanBeInvisible;
		bool disableSaving;
		bool mobGriefing;
		bool keepInventory;
		bool doMobSpawning;
		bool doMobLoot;
		bool doTileDrops;
		bool naturalRegeneration;
		bool doDaylightCycle;
		bool hardcore;
		/** `hardcore-ban-ip` — whether hardcore death bans include IP bans */
		bool hardcoreBanIp;

		/** `max-monsters` natural spawn cap for monsters (zombies, skeletons, creepers, etc.) */
		int maxMonsters;
		/** `max-animals` natural spawn cap for animals (cows, sheep, pigs) */
		int maxAnimals;
		/** `max-ambient` natural spawn cap for ambient mobs (bats) */
		int maxAmbient;
		/** `max-water-animals` natural spawn cap for water mobs (squid) */
		int maxWaterAnimals;
		/** `max-wolves` natural spawn cap for wolves */
		int maxWolves;
		/** `max-chickens` natural spawn cap for chickens */
		int maxChickens;
		/** `max-mushroom-cows` natural spawn cap for mooshrooms */
		int maxMushroomCows;

		/** security settings */
		/** `hide-player-list-prelogin` — strip XUIDs from PreLoginPacket response */
		bool hidePlayerListPreLogin;
		/** `rate-limit-connections-per-window` — max TCP connections per IP within the rate limit window */
		int rateLimitConnectionsPerWindow;
		/** `rate-limit-window-seconds` — sliding window duration for connection rate limiting */
		int rateLimitWindowSeconds;
		/** `max-pending-connections` — max simultaneous pending (pre-login) connections */
		int maxPendingConnections;
		/** `require-challenge-token` — reserved for future protocol extension (not yet enforced) */
		bool requireChallengeToken;
		/** `enable-stream-cipher` — enable XOR stream cipher for traffic obfuscation */
		bool enableStreamCipher;
		/** `require-secure-client` — kick clients that do not complete the cipher handshake */
		bool requireSecureClient;
		/** `proxy-protocol` — parse PROXY protocol v1 headers from TCP tunnel (e.g. playit.gg) */
		bool proxyProtocol;

		/** other MinecraftServer runtime settings */
		int maxBuildHeight;
	};

	/**
	 * server.properties loader
	 *
	 * - ファイル欠損時はデフォルト値で新規作成
	 * - 必須キー不足時は補完して再保存
	 *
	 * @return `WorldManager` が利用するワールド設定
	 */
	ServerPropertiesConfig LoadServerPropertiesConfig();

	/**
	 * server.properties saver
	 *
	 * - `level-name` と `white-list` を更新
	 * - それ以外の既存キーは極力保持
	 *
	 * @param config 保存するワールド識別情報と永続化対象設定
	 * @return 書き込み成功時 `true`
	 */
	bool SaveServerPropertiesConfig(const ServerPropertiesConfig &config);
}
