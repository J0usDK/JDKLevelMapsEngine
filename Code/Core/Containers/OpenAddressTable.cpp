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
		v--;
		COMPUTE_NEXT_POT_RETURN(v);
	}

	void COpenAddressTable::Initialize(uint32 maxElements)
	{
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
		if (m_table.empty())
			return kInvalidValue;
		
		DBG_PROBE_INIT();

		uint32 idx = Hash(key) & m_mask;
		for (;;)
		{
			DBG_PROBE_CHECK(m_table.size());

			const SHashEntry& e = m_table[idx];
			if (e.key == key)
				return e.value;
			if (e.key == kInvalidKey)
				return kInvalidValue;
			idx = (idx + 1) & m_mask;
		}
	}

	uint32* COpenAddressTable::GetValuePtr(uint32 key)
	{
		if (m_table.empty())
			return nullptr;

		DBG_PROBE_INIT();

		uint32 idx = Hash(key) & m_mask;
		for (;;)
		{
			DBG_PROBE_CHECK(m_table.size());

			SHashEntry& e = m_table[idx];
			if (e.key == key)
				return &e.value;
			if (e.key == kInvalidKey)
				return nullptr;
			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Insert(uint32 key, uint32 value)
	{
		if (m_table.empty()) return;

		if (m_activeCount >= m_resizeThreshold)
			Resize(static_cast<uint32>(m_table.size() * 2));

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
		if (m_table.empty()) return;

		DBG_PROBE_INIT();

		uint32 i = Hash(key) & m_mask;
		for (;;)
		{
			DBG_PROBE_CHECK(m_table.size());

			if (m_table[i].key == kInvalidKey) return;
			if (m_table[i].key == key) break;

			i = (i + 1) & m_mask;
		}
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
	}

	void COpenAddressTable::Resize(uint32 newCapacity)
	{
		uint32 actualCapacity = NextPowerOfTwo(newCapacity);
		if (actualCapacity <= m_table.size())
			return;

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