#pragma once
#include <vector>

#include <CryCore/BaseTypes.h>

namespace JDKLevelMaps::Core::Containers
{
	constexpr uint32 kInvalidKey = 0xFFFFFFFF;
	constexpr uint32 kTombstoneKey = 0xFFFFFFFE;
	constexpr uint32 kInvalidValue = 0xFFFFFFFF;

	struct SHashEntry
	{
		uint32 key = kInvalidKey;
		uint32 value = kInvalidValue;
	};

	class COpenAddressTable final
	{
	public:
		void Initialize(uint32 maxElements);

		// WARNING: Do not call while the table is rebuilding. Safe to call only during the Update phase.
		uint32 Find(uint32 key) const;

		// WARNING: Do not call while the table is rebuilding. Safe to call only during the Update phase.
		void Insert(uint32 key, uint32 value);

		// WARNING: Do not call while the table is rebuilding. Safe to call only during the Update phase.
		void Remove(uint32 key);
		
		// WARNING: Do not call while the table is rebuilding. Safe to call only during the Update phase.
		void Clear();

		// WARNING: Do not call while the table is rebuilding. Safe to call only during the Update phase.
		void Reset();

		void FlushRebuild();

	private:
		static uint32 Hash(uint32 x);
		static uint32 NextPowerOfTwo(uint32 v);

		void Rebuild();

	private:
		std::vector<SHashEntry> m_table;
		uint32 m_mask = 0;

		uint32 m_activeCount = 0;
		uint32 m_tombstoneCount = 0;
		uint32 m_capacityLimit = 0;

		bool m_bIsDirty = false;
		bool m_bIsRebuilding = false;
	};
}