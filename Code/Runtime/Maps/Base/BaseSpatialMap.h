#pragma once
#include "Shared/ILevelMap.h"
#include "Shared/MapHeader.h"
#include "Core/Containers/OpenAddressTable.h"

namespace JDKLevelMaps::Maps
{
	constexpr uint16 kInvalidSlot = 0xFFFF;

	class CBaseSpatialMap : public virtual ILevelMap
	{
	public:
		explicit CBaseSpatialMap(const SMapHeader& header);
		virtual ~CBaseSpatialMap() = default;

		EMapType GetType() const override;
		bool IsValid() const override;
		size_t GetMemoryUsage() const override;
		const SMapHeader& GetHeader() const;

		void SetMaximalCapacity(uint32 capacity, size_t tileByteSize);
		bool IsTileLoaded(uint32 tileIndex) const;
		uint16 ReserveTileSlot(uint32 tileIndex, uint8** ppOutBuffer);
		void CommitTile(uint32 tileIndex, uint16 slot);
		void ReleaseTile(uint32 tileIndex);

		void FlushPendingMaintenance();

	protected:
		const uint8* GetTileData(size_t tileIndex) const;

	protected:
		SMapHeader m_header;
		bool m_isValid;

	protected:
		Core::Containers::COpenAddressTable m_lookupTable;
		std::vector<uint8> m_tilePool;
		std::vector<uint32> m_freeSlots;

		size_t m_tileByteSize = 0;
	};
}