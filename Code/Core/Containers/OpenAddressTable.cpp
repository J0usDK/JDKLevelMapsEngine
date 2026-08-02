#include "StdAfx.h"
#include "OpenAddressTable.h"

#include "Core/Platforms/ContainersPlatform.h"

namespace JDKLevelMaps::Core::Containers
{
	uint32 COpenAddressTable::Hash(uint32 x)
	{
		return static_cast<uint32>(
			(static_cast<uint64>(x) * 11400714819323198485ull) >> 32
		);
	}

	uint32 COpenAddressTable::NextPowerOfTwo(uint32 v)
	{
		if (v <= 1) return 1;

		JDK_SOFT_ASSERT(v < 0x80000000, "OpenAddressTable capacity overflow. Requested capacity is too large. Table will fail silently.");

		v--;
		COMPUTE_NEXT_POT_RETURN(v);
	}

	uint32 COpenAddressTable::FindIndex(uint32 key) const
	{
		if (m_table.empty()) return kInvalidKey;

		DBG_PROBE_INIT();
		uint32 idx = Hash(key) & m_mask;

		for (;;)
		{
			DBG_PROBE_CHECK(m_table.size());

			const SHashEntry& e = m_table[idx];
			if (e.key == key) return idx;
			if (e.key == kInvalidKey) return kInvalidKey;

			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Initialize(uint32 maxElements)
	{
		JDK_ASSERT_ERROR(m_table.empty(), "COpenAddressTable::Initialize() called on already initialized table. Use Reset() to clear the table first.");

		if (maxElements == 0) return;

		uint64 targetCapacity = (static_cast<uint64>(maxElements) * 10 + 6) / 7;
		uint32 actualCapacity = NextPowerOfTwo(static_cast<uint32>(targetCapacity));

		m_table.assign(actualCapacity, SHashEntry{});
		m_mask = actualCapacity - 1;
		m_activeCount = 0;
		m_resizeThreshold = (actualCapacity * 3) / 4;
	}

	uint32 COpenAddressTable::Find(uint32 key) const
	{
		if (key == kInvalidKey) return kInvalidValue;
		uint32 idx = FindIndex(key);
		return (idx != kInvalidKey) ? m_table[idx].value : kInvalidValue;
	}

	uint32* COpenAddressTable::GetValuePtr(uint32 key)
	{
		if (key == kInvalidKey) return nullptr;
		uint32 idx = FindIndex(key);
		return (idx != kInvalidKey) ? &m_table[idx].value : nullptr;
	}

	void COpenAddressTable::Insert(uint32 key, uint32 value)
	{
		JDK_ASSERT_ERROR(key != kInvalidKey, "Attempting to insert reserved kInvalidKey (0xFFFFFFFF). This will break the hash table.");
		if (key == kInvalidKey || m_table.empty()) return;

		if (m_activeCount >= m_resizeThreshold)
		{
			JDK_ASSERT_FATAL(m_table.size() < 0x7FFFFFFF, "OpenAddressTable capacity overflow during auto-resize. Table will degrade into an infinite loop.");
			Resize(static_cast<uint32>(m_table.size() * 2));
		}

		DBG_PROBE_INIT();

		uint32 idx = Hash(key) & m_mask;
		for (;;)
		{
			DBG_PROBE_CHECK(m_table.size());
			SHashEntry& entry = m_table[idx];

			if (entry.key == key)
			{
				entry.value = value;
				return;
			}
			if (entry.key == kInvalidKey)
			{
				entry.key = key;
				entry.value = value;
				m_activeCount++;
				return;
			}
			
			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Remove(uint32 key)
	{
		JDK_SOFT_ASSERT(key != kInvalidKey, "Attempting to remove reserved kInvalidKey.");
		if (key == kInvalidKey || m_table.empty()) return;

		uint32 i = FindIndex(key);
		if (i == kInvalidKey) return;

		m_activeCount--;

		uint32 j = i;
		uint32 k = (j + 1) & m_mask;

		for (;;)
		{
			if (m_table[k].key == kInvalidKey)
			{
				m_table[j].key = kInvalidKey;
				m_table[j].value = kInvalidValue;
				return;
			}

			uint32 idealSlot = Hash(m_table[k].key) & m_mask;
			if (((j - idealSlot) & m_mask) < ((k - idealSlot) & m_mask))
			{
				m_table[j] = m_table[k];
				j = k;
			}
			k = (k + 1) & m_mask;
		}
	}

	void COpenAddressTable::Clear()
	{
		std::fill(m_table.begin(), m_table.end(), SHashEntry{});
		m_activeCount = 0;
	}

	void COpenAddressTable::Reset()
	{
		m_table.clear();
		m_mask = 0;
		m_resizeThreshold = 0;
		m_activeCount = 0;
	}

	void COpenAddressTable::Resize(uint32 newCapacity)
	{
		uint32 actualCapacity = NextPowerOfTwo(newCapacity);

		JDK_ASSERT_ERROR(actualCapacity > m_table.size(), "Resize capacity is smaller or equal to current. Check for uint32 overflow or invalid manual Resize call.");

		if (actualCapacity <= m_table.size())
			return;

		JDK_PERF_WARN("OpenAddressTable::Resize() triggered mid-frame. This causes reallocation and may stutter the game.");

		std::vector<SHashEntry> newTable(actualCapacity, SHashEntry{});
		uint32 newMask = actualCapacity - 1;

		for (const auto& e : m_table)
		{
			if (e.key == kInvalidKey) continue;

			DBG_PROBE_INIT();

			uint32 idx = Hash(e.key) & newMask;
			for (;;)
			{
				DBG_PROBE_CHECK(newTable.size());

				if (newTable[idx].key == kInvalidKey)
				{
					newTable[idx].key = e.key;
					newTable[idx].value = e.value;
					break;
				}
				idx = (idx + 1) & newMask;
			}
		}

		std::swap(m_table, newTable);

		m_mask = newMask;
		m_resizeThreshold = (actualCapacity * 3) / 4;
	}

	size_t COpenAddressTable::GetMemoryUsage() const { return m_table.capacity() * sizeof(SHashEntry); }
}