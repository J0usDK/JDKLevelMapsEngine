#include "StdAfx.h"
#include "BaseFullMap.h"

#include "Runtime/Maps/Base/LoadedMap.h"

namespace JDKLevelMaps::Maps
{
	CBaseFullMap::CBaseFullMap(SLoadedMap&& loadedMap) :
		m_header(loadedMap.header),
		m_packedData(std::move(loadedMap.packedData)),
		m_packedDataSize(loadedMap.packedDataSize),
		m_tileOffsets(std::move(loadedMap.tileOffsets)),
		m_mapFilePath(loadedMap.mapFilePath)
	{
		const size_t expectedSize = static_cast<size_t>(m_header.tileCountX * m_header.tileCountY);

		m_isValid = m_header.gridWidth > 0 && m_header.gridHeight > 0
			&& m_header.cellSize > 0.0f
			&& m_header.tileSize > 0
			&& m_tileOffsets.size() == expectedSize;
	}

	const uint8* CBaseFullMap::GetTileData(uint32 tileIndex) const
	{
		if (tileIndex >= m_tileOffsets.size())
			return nullptr;

		const uint32 offset = m_tileOffsets[tileIndex];
		if (offset == kEmptyTileOffset)
			return nullptr;
		return m_packedData.get() + offset;
	}

	EMapType CBaseFullMap::GetType() const { return m_header.mapType; }
	const char* CBaseFullMap::GetFilePath() const { return m_mapFilePath.c_str(); }
	size_t CBaseFullMap::GetMemoryUsage() const { return sizeof(SMapHeader) + (m_tileOffsets.capacity() * sizeof(uint32)) + m_packedDataSize; }
	bool CBaseFullMap::IsValid() const { return m_isValid; }
	const SMapHeader& CBaseFullMap::GetHeader() const { return m_header; }
	uint32 CBaseFullMap::GetMaxCapacity() const { return static_cast<uint32>(m_tileOffsets.capacity()); }

	CBaseFullMap* CBaseFullMap::AsFullMap() { return this; }
	const CBaseFullMap* CBaseFullMap::AsFullMap() const { return this; }
}