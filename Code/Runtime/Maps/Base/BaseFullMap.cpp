#include "StdAfx.h"
#include "BaseFullMap.h"

#include "Runtime/Maps/Base/LoadedMap.h"

namespace JDKLevelMaps::Maps
{
	CBaseFullMap::CBaseFullMap(SLoadedMap&& loadedMap) :
		m_header(loadedMap.header),
		m_packedData(std::move(loadedMap.packedData)),
		m_packedDataSize(loadedMap.packedDataSize),
		m_tileOffsets(std::move(loadedMap.tileOffsets))
	{
		const size_t expectedSize = static_cast<size_t>(m_header.tileCountX * m_header.tileCountY);

		m_isValid = m_header.gridWidth > 0 && m_header.gridHeight > 0
			&& m_header.cellSize > 0.0f
			&& m_header.tileSize > 0
			&& m_tileOffsets.size() == expectedSize;
	}

	EMapType CBaseFullMap::GetType() const { return m_header.mapType; }
	size_t CBaseFullMap::GetMemoryUsage() const { return sizeof(SMapHeader) + (m_tileOffsets.capacity() * sizeof(uint32)) + m_packedDataSize; }
	bool CBaseFullMap::IsValid() const { return m_isValid; }
}