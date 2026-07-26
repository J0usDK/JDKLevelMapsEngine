#include "StdAfx.h"
#include "BaseSpatialMap.h"

namespace JDKLevelMaps::Maps
{
	CBaseSpatialMap::CBaseSpatialMap(const SMapHeader& header) : m_header(header)
	{
		m_isValid = m_header.gridWidth > 0 && m_header.gridHeight > 0
			&& m_header.cellSize > 0.0f
			&& m_header.tileSize > 0;
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

	void CBaseSpatialMap::FlushPendingMaintenance()
	{
		m_lookupTable.FlushRebuild();
	}

	const uint8* CBaseSpatialMap::GetTileData(size_t tileIndex) const
	{
		const uint32 slot = m_lookupTable.Find(tileIndex);
		if (slot == Core::Containers::kInvalidValue)
			return nullptr;
		return &m_tilePool[static_cast<size_t>(slot) * m_tileByteSize];
	}

	EMapType CBaseSpatialMap::GetType() const { return m_header.mapType; }
	bool CBaseSpatialMap::IsValid() const { return m_isValid; }
	size_t CBaseSpatialMap::GetMemoryUsage() const { return sizeof(SMapHeader) + m_tilePool.capacity() + m_lookupTable.GetMemoryUsage(); }
}