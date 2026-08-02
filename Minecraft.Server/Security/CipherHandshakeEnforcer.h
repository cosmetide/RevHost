#pragma once

#ifdef _WINDOWS64
#include <Windows.h>
#endif

#include <vector>

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * Tracks pending cipher handshakes and kicks clients that don't complete
		 * within the grace period.
		 *
		 * When require-secure-client is enabled, old/unpatched clients that ignore
		 * MC|CKey are disconnected before they receive any PlayerInfoPacket data
		 * containing other players' XUIDs.
		 *
		 * Called from the main tick thread only (PlayerList::tick).
		 */
		class CipherHandshakeEnforcer
		{
		public:
			// 5 seconds at 20 TPS. The security gate buffers all game data until
			// cipher completes, so no data leaks regardless of grace period length.
			// 5 seconds accommodates high-latency connections.
			static const int kGraceTicks = 100;

			CipherHandshakeEnforcer();
			~CipherHandshakeEnforcer();

			CipherHandshakeEnforcer(const CipherHandshakeEnforcer &) = delete;
			CipherHandshakeEnforcer &operator=(const CipherHandshakeEnforcer &) = delete;

			/**
			 * Register that MC|CKey was sent to this smallId at the given tick.
			 */
			void OnCipherKeySent(unsigned char smallId, unsigned int currentTick);

			/**
			 * Check for timed-out handshakes. Returns smallIds that exceeded the
			 * grace period without the cipher becoming active. Also returns
			 * smallIds that just completed (cipher became active) in outCompleted.
			 */
			void CheckTimeouts(unsigned int currentTick,
				std::vector<unsigned char> &outExpired,
				std::vector<unsigned char> &outCompleted);

			/**
			 * Clean up tracking for a disconnected connection.
			 */
			void OnDisconnected(unsigned char smallId);

		private:
			static const int MAX_CONNECTIONS = 256;
			unsigned int m_sentTick[MAX_CONNECTIONS]; // 0 = not tracked
		bool m_tracked[MAX_CONNECTIONS];
		};

		CipherHandshakeEnforcer &GetHandshakeEnforcer();
	}
}
