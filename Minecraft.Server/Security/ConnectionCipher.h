#pragma once

#include "StreamCipher.h"

#ifdef _WINDOWS64
#include <Windows.h>
#endif

namespace ServerRuntime
{
	namespace Security
	{
		/**
		 * Per-connection cipher registry for the dedicated server.
		 *
		 * Handshake protocol (4-message, via CustomPayloadPacket):
		 * 1. Server calls PrepareKey(smallId) -> sends MC|CKey with key to client
		 * 2. Client stores key, sends MC|CAck, activates send cipher
		 * 3. Server recv thread detects MC|CAck -> calls SendCOnAndCommit which
		 *    atomically sends MC|COn plaintext then calls CommitCipher(smallId)
		 * 4. Client recv thread detects MC|COn -> activates recv cipher
		 *
		 * Backwards compatible: old clients ignore MC|CKey, server never gets ack,
		 * cipher stays inactive. Old servers never send MC|CKey, client stays plaintext.
		 */
		class ConnectionCipherRegistry
		{
		public:
			ConnectionCipherRegistry();
			~ConnectionCipherRegistry();

			ConnectionCipherRegistry(const ConnectionCipherRegistry &) = delete;
			ConnectionCipherRegistry &operator=(const ConnectionCipherRegistry &) = delete;
			ConnectionCipherRegistry(ConnectionCipherRegistry &&) = delete;
			ConnectionCipherRegistry &operator=(ConnectionCipherRegistry &&) = delete;

			/**
			 * Generate a random key and store it in pending state for the given smallId.
			 * Does NOT activate the cipher. Call CommitCipher() after the client acks.
			 * Returns the generated key in outKey.
			 */
			bool PrepareKey(unsigned char smallId, uint8_t outKey[StreamCipher::KEY_SIZE]);

			/**
			 * Activate a previously prepared cipher. Called from the recv thread
			 * when the client's MC|CAck is detected by raw byte matching.
			 * Returns false if no key was pending for this smallId.
			 */
			bool CommitCipher(unsigned char smallId);

			/**
			 * Cancel a pending key (e.g., client disconnected before ack).
			 */
			void CancelPending(unsigned char smallId);

			/**
			 * Check if a key is pending for the given smallId (no side effects).
			 */
			bool HasPendingKey(unsigned char smallId) const;

			/**
			 * Deactivate the cipher and cancel any pending key for a disconnected connection.
			 */
			void DeactivateCipher(unsigned char smallId);

			/**
			 * Atomically check if cipher is active and encrypt outgoing data.
			 * Returns true if data was encrypted, false if cipher is inactive (data untouched).
			 */
			bool TryEncryptOutgoing(unsigned char smallId, uint8_t *data, int length);

			/**
			 * Check if the cipher is active (handshake completed) for a given smallId.
			 * Thread-safe, read-only query.
			 */
			bool IsCipherActive(unsigned char smallId) const;

			/**
			 * Decrypt incoming data from a specific connection.
			 * No-op if the cipher is not active for this connection.
			 */
			void DecryptIncoming(unsigned char smallId, uint8_t *data, int length);

		private:
			static const int MAX_CONNECTIONS = 256;
			StreamCipher m_ciphers[MAX_CONNECTIONS];
			bool m_pending[MAX_CONNECTIONS];
			uint8_t m_pendingKeys[MAX_CONNECTIONS][StreamCipher::KEY_SIZE];
			mutable CRITICAL_SECTION m_lock;
		};

		/**
		 * Global cipher registry singleton.
		 */
		ConnectionCipherRegistry &GetCipherRegistry();
	}
}
