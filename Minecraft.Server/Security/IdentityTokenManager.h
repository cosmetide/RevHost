#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#ifdef _WINDOWS64
#include <Windows.h>
#endif

typedef unsigned __int64 PlayerUID;

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * Persistent XUID-to-token binding for identity verification.
		 *
		 * On first login, the server issues a random 32-byte token to the client
		 * over the encrypted cipher channel. The client stores it locally.
		 * On subsequent logins, the server challenges the client to present
		 * its stored token. Mismatch = kicked.
		 *
		 * This prevents XUID replay attacks: an attacker who steals a XUID
		 * still needs the token, which was only sent over the encrypted channel.
		 *
		 * Tokens are stored in `identity-tokens.json` and persist across restarts.
		 */
		class IdentityTokenManager
		{
		public:
			static const int TOKEN_SIZE = 32;

			IdentityTokenManager();
			~IdentityTokenManager();

			IdentityTokenManager(const IdentityTokenManager &) = delete;
			IdentityTokenManager &operator=(const IdentityTokenManager &) = delete;

			bool Initialize(const std::string &filePath);
			void Shutdown();

			bool HasToken(PlayerUID xuid) const;
			bool GetToken(PlayerUID xuid, uint8_t outToken[TOKEN_SIZE]) const;
			bool IssueToken(PlayerUID xuid, uint8_t outToken[TOKEN_SIZE]);
			bool VerifyToken(PlayerUID xuid, const uint8_t token[TOKEN_SIZE]) const;
			bool RevokeToken(PlayerUID xuid);

		private:
			bool Load();
			bool Save() const;

			std::string m_filePath;
			std::unordered_map<PlayerUID, std::vector<uint8_t>> m_tokens;
			mutable CRITICAL_SECTION m_lock;
			bool m_initialized;
		};

		IdentityTokenManager &GetIdentityTokenManager();
	}
}
