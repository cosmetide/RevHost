#include "stdafx.h"
#include "CipherHandshakeEnforcer.h"
#include "ConnectionCipher.h"

namespace ServerRuntime
{
	namespace Security
	{
		CipherHandshakeEnforcer::CipherHandshakeEnforcer()
		{
			memset(m_sentTick, 0, sizeof(m_sentTick));
			memset(m_tracked, 0, sizeof(m_tracked));
		}

		CipherHandshakeEnforcer::~CipherHandshakeEnforcer()
		{
		}

		void CipherHandshakeEnforcer::OnCipherKeySent(unsigned char smallId, unsigned int currentTick)
		{
			m_sentTick[smallId] = currentTick;
			m_tracked[smallId] = true;
		}

		void CipherHandshakeEnforcer::CheckTimeouts(unsigned int currentTick,
			std::vector<unsigned char> &outExpired,
			std::vector<unsigned char> &outCompleted)
		{
			auto &registry = GetCipherRegistry();

			for (int i = 0; i < MAX_CONNECTIONS; ++i)
			{
				if (!m_tracked[i])
					continue;

				if (registry.IsCipherActive(static_cast<unsigned char>(i)))
				{
					outCompleted.push_back(static_cast<unsigned char>(i));
					m_tracked[i] = false;
				}
				else if ((currentTick - m_sentTick[i]) > static_cast<unsigned int>(kGraceTicks))
				{
					outExpired.push_back(static_cast<unsigned char>(i));
					m_tracked[i] = false;
				}
			}
		}

		void CipherHandshakeEnforcer::OnDisconnected(unsigned char smallId)
		{
			m_tracked[smallId] = false;
		}

		CipherHandshakeEnforcer &GetHandshakeEnforcer()
		{
			static CipherHandshakeEnforcer s_instance;
			return s_instance;
		}
	}
}
