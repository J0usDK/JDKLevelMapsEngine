#include "StdAfx.h"
#include "OpenAddressTable.h"

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
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		return ++v;
	}

	void COpenAddressTable::Initialize(uint32 maxElements)
	{
		if (maxElements == 0)
			return;

		uint64 targetCapacity = (static_cast<uint64>(maxElements) * 10 + 6) / 7;
		uint32 actualCapacity = NextPowerOfTwo(static_cast<uint32>(targetCapacity));

		m_table.assign(actualCapacity, SHashEntry{});
		m_mask = actualCapacity - 1;

		m_activeCount = 0;
		m_tombstoneCount = 0;
		m_capacityLimit = actualCapacity / 4;

		m_bIsDirty = false;
		m_bIsRebuilding = false;
	}

	uint32 COpenAddressTable::Find(uint32 key) const
	{
		CRY_ASSERT_MESSAGE(!m_bIsRebuilding, "[JDKLevelMaps] Rebuild is active. Safe access guaranteed only in Update phase");

		if (m_table.empty())
			return kInvalidValue;

#if !defined(_RELEASE)
		uint32 dbgProbes = 0;
#endif

		uint32 idx = Hash(key) & m_mask;
		for (;;)
		{
#if !defined (_RELEASE)
			CRY_ASSERT_MESSAGE(++dbgProbes <= m_table.size(), "[JDKLevelMaps] Infinite loop in Find(). Table is 100% full without empty slots.");
#endif

			const SHashEntry& e = m_table[idx];
			if (e.key == key)
				return e.value;
			if (e.key == kInvalidKey)
				return kInvalidValue;
			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Insert(uint32 key, uint32 value)
	{
		CRY_ASSERT_MESSAGE(!m_bIsRebuilding, "[JDKLevelMaps] Rebuild is active. Safe access guaranteed only in Update phase");

		if (m_table.empty()) return;

		uint32 idx = Hash(key) & m_mask;
		uint32 tombstoneIdx = kInvalidKey;

#if !defined(_RELEASE)
		uint32 dbgProbes = 0;
#endif

		for (;;)
		{
#if !defined (_RELEASE)
			CRY_ASSERT_MESSAGE(++dbgProbes <= m_table.size(), "[JDKLevelMaps] Infinite loop in Insert(). Table is 100% full without empty slots.");
#endif

			SHashEntry& entry = m_table[idx];

			if (entry.key == key)
			{
				entry.value = value;
				return;
			}
			if (entry.key == kTombstoneKey)
			{
				if (tombstoneIdx == kInvalidKey)
					tombstoneIdx = idx;
			}
			else if (entry.key == kInvalidKey)
			{
				if (tombstoneIdx == kInvalidKey)
				{
					m_table[idx].key = key;
					m_table[idx].value = value;
				}
				else
				{
					m_table[tombstoneIdx].key = key;
					m_table[tombstoneIdx].value = value;
					m_tombstoneCount--;
				}

				++m_activeCount;

				if (m_tombstoneCount > m_capacityLimit || m_tombstoneCount > m_activeCount / 2)
					m_bIsDirty = true;
				return;
			}
			
			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Remove(uint32 key)
	{
		CRY_ASSERT_MESSAGE(!m_bIsRebuilding, "[JDKLevelMaps] Rebuild is active. Safe access guaranteed only in Update phase");

		if (m_table.empty()) return;

#if !defined(_RELEASE)
		uint32 dbgProbes = 0;
#endif

		uint32 idx = Hash(key) & m_mask;
		for (;;)
		{
#if !defined (_RELEASE)
			CRY_ASSERT_MESSAGE(++dbgProbes <= m_table.size(), "[JDKLevelMaps] Infinite loop in Remove(). Table is 100% full without empty slots.");
#endif

			SHashEntry& entry = m_table[idx];
			if (entry.key == key)
			{
				entry.key = kTombstoneKey;
				entry.value = kInvalidValue;

				m_activeCount--;
				m_tombstoneCount++;

				if (m_tombstoneCount > m_capacityLimit || m_tombstoneCount > m_activeCount / 2)
					m_bIsDirty = true;
				return;
			}
			if (entry.key == kInvalidKey)
				return;
			
			idx = (idx + 1) & m_mask;
		}
	}

	void COpenAddressTable::Clear()
	{
		CRY_ASSERT_MESSAGE(!m_bIsRebuilding, "[JDKLevelMaps] Rebuild is active. Safe access guaranteed only in Update phase");

		std::fill(m_table.begin(), m_table.end(), SHashEntry{});

		m_activeCount = 0;
		m_tombstoneCount = 0;

		m_bIsDirty = false;
		m_bIsRebuilding = false;
	}

	void COpenAddressTable::Reset()
	{
		CRY_ASSERT_MESSAGE(!m_bIsRebuilding, "[JDKLevelMaps] Rebuild is active. Safe access guaranteed only in Update phase");

		m_table.clear();
		m_mask = 0;

		m_activeCount = 0;
		m_tombstoneCount = 0;
		m_capacityLimit = 0;

		m_bIsDirty = false;
		m_bIsRebuilding = false;
	}

	void COpenAddressTable::FlushRebuild()
	{
		if (!m_bIsDirty)
			return;

		Rebuild();
	}

	void COpenAddressTable::Rebuild()
	{
		m_bIsRebuilding = true;

		std::vector<SHashEntry> newTable(m_table.size(), SHashEntry{});
		for (const auto& e : m_table)
		{
			if (e.key != kInvalidKey && e.key != kTombstoneKey)
			{
#if !defined(_RELEASE)
				uint32 dbgProbes = 0;
#endif

				uint32 idx = Hash(e.key) & m_mask;
				for (;;)
				{
#if !defined (_RELEASE)
					CRY_ASSERT_MESSAGE(++dbgProbes <= m_table.size(), "[JDKLevelMaps] Infinite loop in Rebuild(). Table is 100% full without empty slots.");
#endif

					if (newTable[idx].key == kInvalidKey)
					{
						newTable[idx] = e;
						break;
					}
					idx = (idx + 1) & m_mask;
				}
			}
		}

		m_table = std::move(newTable);
		m_tombstoneCount = 0;

		m_bIsDirty = false;
		m_bIsRebuilding = false;
	}

	size_t COpenAddressTable::GetMemoryUsage() const { return m_table.capacity() * sizeof(SHashEntry); }
}