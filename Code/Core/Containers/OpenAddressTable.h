#pragma once
#include <vector>

#include <CryCore/BaseTypes.h>

namespace JDKLevelMaps::Core::Containers
{
	constexpr uint32 kInvalidKey = 0xFFFFFFFF;
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

		uint32 Find(uint32 key) const;
		uint32* GetValuePtr(uint32 key);

		void Insert(uint32 key, uint32 value);
		void Remove(uint32 key);

		void Clear();
		void Reset();
		void Resize(uint32 newCapacity);

		inline uint32 GetSize() const { return m_activeCount; }
		inline uint32 GetCapacity() const { return static_cast<uint32>(m_table.size()); }
		inline bool IsEmpty() const { return m_activeCount == 0; }
		size_t GetMemoryUsage() const;

	private:
		static uint32 Hash(uint32 x);
		static uint32 NextPowerOfTwo(uint32 v);

		uint32 FindIndex(uint32 key) const;

	private:
		std::vector<SHashEntry> m_table;
		uint32 m_mask = 0;

		uint32 m_activeCount = 0;
		uint32 m_resizeThreshold = 0;
	};
}