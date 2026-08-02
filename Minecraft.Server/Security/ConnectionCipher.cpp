#include "stdafx.h"
#include "ConnectionCipher.h"

#include <cstring>

namespace ServerRuntime
{
	namespace Security
	{
		ConnectionCipherRegistry::ConnectionCipherRegistry()
		{
			InitializeCriticalSection(&m_lock);
			memset(m_pending, 0, sizeof(m_pending));
			memset(m_pendingKeys, 0, sizeof(m_pendingKeys));
		}

		ConnectionCipherRegistry::~ConnectionCipherRegistry()
		{
			SecureZeroMemory(m_pendingKeys, sizeof(m_pendingKeys));
			DeleteCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::PrepareKey(unsigned char smallId, uint8_t outKey[StreamCipher::KEY_SIZE])
		{
			uint8_t key[StreamCipher::KEY_SIZE];
			if (!StreamCipher::GenerateKey(key))
			{
				return false;
			}

			EnterCriticalSection(&m_lock);
			memcpy(m_pendingKeys[smallId], key, StreamCipher::KEY_SIZE);
			m_pending[smallId] = true;
			LeaveCriticalSection(&m_lock);

			memcpy(outKey, key, StreamCipher::KEY_SIZE);
			SecureZeroMemory(key, sizeof(key));
			return true;
		}

		bool ConnectionCipherRegistry::CommitCipher(unsigned char smallId)
		{
			EnterCriticalSection(&m_lock);
			if (!m_pending[smallId])
			{
				LeaveCriticalSection(&m_lock);
				return false;
			}

			m_ciphers[smallId].Initialize(m_pendingKeys[smallId]);
			SecureZeroMemory(m_pendingKeys[smallId], StreamCipher::KEY_SIZE);
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
			return true;
		}

		void ConnectionCipherRegistry::CancelPending(unsigned char smallId)
		{
			EnterCriticalSection(&m_lock);
			SecureZeroMemory(m_pendingKeys[smallId], StreamCipher::KEY_SIZE);
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::HasPendingKey(unsigned char smallId) const
		{
			EnterCriticalSection(&m_lock);
			bool pending = m_pending[smallId];
			LeaveCriticalSection(&m_lock);
			return pending;
		}

		void ConnectionCipherRegistry::DeactivateCipher(unsigned char smallId)
		{
			EnterCriticalSection(&m_lock);
			m_ciphers[smallId].Reset();
			SecureZeroMemory(m_pendingKeys[smallId], StreamCipher::KEY_SIZE);
			m_pending[smallId] = false;
			LeaveCriticalSection(&m_lock);
		}

		bool ConnectionCipherRegistry::TryEncryptOutgoing(unsigned char smallId, uint8_t *data, int length)
		{
			EnterCriticalSection(&m_lock);
			bool active = m_ciphers[smallId].IsActive();
			if (active)
			{
				m_ciphers[smallId].Encrypt(data, length);
			}
			LeaveCriticalSection(&m_lock);
			return active;
		}

		bool ConnectionCipherRegistry::IsCipherActive(unsigned char smallId) const
		{
			EnterCriticalSection(&m_lock);
			bool active = m_ciphers[smallId].IsActive();
			LeaveCriticalSection(&m_lock);
			return active;
		}

		void ConnectionCipherRegistry::DecryptIncoming(unsigned char smallId, uint8_t *data, int length)
		{
			EnterCriticalSection(&m_lock);
			m_ciphers[smallId].Decrypt(data, length);
			LeaveCriticalSection(&m_lock);
		}

		ConnectionCipherRegistry &GetCipherRegistry()
		{
			static ConnectionCipherRegistry s_instance;
			return s_instance;
		}
	}
}
