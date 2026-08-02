#include "StdAfx.h"
#include "BaseSpatialMap.h"

#include <limits>

namespace JDKLevelMaps::Maps
{
	CBaseSpatialMap::CBaseSpatialMap(SMapHeader&& header, const string& mapFilePath) : m_header(std::move(header)), m_mapFilePath(mapFilePath)
	{
		const uint64 totalTiles = static_cast<uint64>(m_header.tileCountX) * m_header.tileCountY;

		m_isValid = m_header.gridWidth > 0 && m_header.gridHeight > 0
			&& m_header.cellSize > 0.0f
			&& m_header.tileSize > 0
			&& m_header.tileCountX > 0
			&& m_header.tileCountY > 0
			&& totalTiles <= std::numeric_limits<uint32>::max();

		const float tileWorldSize = m_header.cellSize * static_cast<float>(m_header.tileSize);
		m_tileWorldSizeInv = tileWorldSize > 0.0001f ? 1.0f / tileWorldSize : 1.0f;
	}

	void CBaseSpatialMap::SetMaximalCapacity(uint32 capacity, size_t tileByteSize)
	{
		m_tileByteSize = tileByteSize;

		m_lookupTable.Reset();
		m_lookupTable.Initialize(capacity);

		m_tilePool.clear();
		m_tilePool.resize(static_cast<size_t>(capacity) * tileByteSize, 0);

		m_freeSlots.clear();
		m_freeSlots.reserve(capacity);
		for (uint32 i = capacity; i > 0; --i)
			m_freeSlots.push_back(i - 1);
	}

	bool CBaseSpatialMap::IsTileLoaded(uint32 tileIndex) const
	{
		return m_lookupTable.Find(tileIndex) != Core::Containers::kInvalidValue;
	}

	uint16 CBaseSpatialMap::ReserveTileSlot(uint32 tileIndex, uint8** ppOutBuffer)
	{
		CRY_ASSERT_MESSAGE(!IsTileLoaded(tileIndex), "[JDKLevelMaps] ReserveTileSlot called for an already loaded tile");

		if (m_freeSlots.empty())
		{
			*ppOutBuffer = nullptr;
			return kInvalidSlot;
		}

		const uint32 slot = m_freeSlots.back();
		m_freeSlots.pop_back();

		*ppOutBuffer = &m_tilePool[static_cast<size_t>(slot) * m_tileByteSize];
		return static_cast<uint16>(slot);
	}

	void CBaseSpatialMap::CommitTile(uint32 tileIndex, uint16 slot)
	{
		CRY_ASSERT_MESSAGE(!IsTileLoaded(tileIndex), "[JDKLevelMaps] CommitTile called for tileIndex already committed - slot leak");
		m_lookupTable.Insert(tileIndex, slot);
	}

	void CBaseSpatialMap::ReleaseTile(uint32 tileIndex)
	{
		const uint32 slot = m_lookupTable.Find(tileIndex);
		if (slot == Core::Containers::kInvalidValue)
			return;

		m_lookupTable.Remove(tileIndex);
		m_freeSlots.push_back(slot);
	}

	void CBaseSpatialMap::ReleaseTileSlotWithoutCommit(uint16 slot)
	{
		m_freeSlots.push_back(slot);
	}

	void CBaseSpatialMap::SetTileDirectory(std::vector<STileEntry>&& directory)
	{
		m_tileDirectory = std::move(directory);
	}

	const STileEntry& CBaseSpatialMap::GetTileEntry(uint32 tileIndex) const
	{
		CRY_ASSERT_MESSAGE(tileIndex < m_tileDirectory.size(), "[JDKLevelMaps] tileIndex out of range");
		return m_tileDirectory[tileIndex];
	}

	const uint8* CBaseSpatialMap::GetTileData(uint32 tileIndex) const
	{
		const uint32 slot = m_lookupTable.Find(tileIndex);
		if (slot == Core::Containers::kInvalidValue)
			return nullptr;
		return &m_tilePool[static_cast<size_t>(slot) * m_tileByteSize];
	}

	EMapType CBaseSpatialMap::GetType() const { return m_header.mapType; }
	const char* CBaseSpatialMap::GetFilePath() const { return m_mapFilePath.c_str(); }
	bool CBaseSpatialMap::IsValid() const { return m_isValid; }
	size_t CBaseSpatialMap::GetMemoryUsage() const { return sizeof(SMapHeader) + m_tilePool.capacity() + m_lookupTable.GetMemoryUsage(); }
	const SMapHeader& CBaseSpatialMap::GetHeader() const { return m_header; }
	uint32 CBaseSpatialMap::GetMaxCapacity() const { return static_cast<uint32>(m_freeSlots.capacity()); }
	float CBaseSpatialMap::GetTileWorldSizeInv() const { return m_tileWorldSizeInv; }

	CBaseSpatialMap* CBaseSpatialMap::AsSpatialMap() { return this; }
	const CBaseSpatialMap* CBaseSpatialMap::AsSpatialMap() const { return this; }
}