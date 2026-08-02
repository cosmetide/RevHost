#include "stdafx.h"
#include "RateLimiter.h"

namespace ServerRuntime
{
	namespace Security
	{
		RateLimiter::RateLimiter()
		{
			InitializeCriticalSection(&m_lock);
		}

		RateLimiter::~RateLimiter()
		{
			DeleteCriticalSection(&m_lock);
		}

		bool RateLimiter::AllowConnection(const std::string &ip, int maxPerWindow, int windowMs)
		{
			if (maxPerWindow <= 0 || windowMs <= 0)
			{
				return true;
			}

			ULONGLONG now = GetTickCount64();
			ULONGLONG windowDuration = static_cast<ULONGLONG>(windowMs);

			EnterCriticalSection(&m_lock);

			auto &timestamps = m_connectionTimes[ip];

			// Remove timestamps outside the sliding window
			while (!timestamps.empty() && (now - timestamps.front()) > windowDuration)
			{
				timestamps.pop_front();
			}

			bool allowed = timestamps.size() < static_cast<size_t>(maxPerWindow);
			if (allowed)
			{
				timestamps.push_back(now);
			}

			LeaveCriticalSection(&m_lock);
			return allowed;
		}

		void RateLimiter::EvictStale(int evictionAgeMs)
		{
			ULONGLONG now = GetTickCount64();
			ULONGLONG evictionAge = static_cast<ULONGLONG>(evictionAgeMs);

			EnterCriticalSection(&m_lock);

			auto it = m_connectionTimes.begin();
			while (it != m_connectionTimes.end())
			{
				if (it->second.empty() ||
					(now - it->second.back()) > evictionAge)
				{
					it = m_connectionTimes.erase(it);
				}
				else
				{
					++it;
				}
			}

			LeaveCriticalSection(&m_lock);
		}

		RateLimiter &GetGlobalRateLimiter()
		{
			static RateLimiter s_instance;
			return s_instance;
		}
	}
}
