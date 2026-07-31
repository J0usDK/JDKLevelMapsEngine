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
		explicit CBaseSpatialMap(SMapHeader&& header, const string& mapFilePath);
		virtual ~CBaseSpatialMap() = default;

		virtual size_t GetTileByteSize() const = 0;

		EMapType GetType() const override;
		const char* GetFilePath() const override;
		bool IsValid() const override;
		size_t GetMemoryUsage() const override;
		const SMapHeader& GetHeader() const override;
		uint32 GetMaxCapacity() const override;

		CBaseSpatialMap* AsSpatialMap() override;
		const CBaseSpatialMap* AsSpatialMap() const override;

		void SetMaximalCapacity(uint32 capacity, size_t tileByteSize);
		bool IsTileLoaded(uint32 tileIndex) const;
		uint16 ReserveTileSlot(uint32 tileIndex, uint8** ppOutBuffer);
		void CommitTile(uint32 tileIndex, uint16 slot);
		void ReleaseTile(uint32 tileIndex);
		void ReleaseTileSlotWithoutCommit(uint16 slot);

		void SetTileDirectory(std::vector<STileEntry>&& directory);
		const STileEntry& GetTileEntry(uint32 tileIndex) const;

		void FlushPendingMaintenance();

	protected:
		const uint8* GetTileData(uint32 tileIndex) const;

	protected:
		SMapHeader m_header;
		bool m_isValid;

	private:
		Core::Containers::COpenAddressTable m_lookupTable;
		std::vector<uint8> m_tilePool;
		std::vector<uint32> m_freeSlots;

		std::vector<STileEntry> m_tileDirectory;

		size_t m_tileByteSize = 0;
		string m_mapFilePath;
	};
}