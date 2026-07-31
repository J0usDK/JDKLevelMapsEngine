#include "StdAfx.h"
#include "VegetationFullMap.h"

#include "Runtime/Maps/Base/LoadedMap.h"
#include "Bootstrap/Reader/MapReader.h"


namespace JDKLevelMaps::Maps
{
	CVegetationFullMap::CVegetationFullMap(SLoadedMap&& loadedMap) : CBaseFullMap(std::move(loadedMap))
	{
		if (m_header.mapType != EMapType::VegetationDensity)
			m_isValid = false;
	}

	uint8 CVegetationFullMap::GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const
	{
		if (!m_isValid)
			return 0;

		const int32 channel = MapLayers::ToChannelIndex(layer);
		if (channel < 0)
			return 0;

		const int32 gridX = static_cast<int32>((worldX - m_header.originX) / m_header.cellSize);
		const int32 gridY = static_cast<int32>((worldY - m_header.originY) / m_header.cellSize);

		if (gridX < 0 || gridX >= m_header.gridWidth || gridY < 0 || gridY >= m_header.gridHeight)
			return 0;

		const uint32 tileX = gridX / m_header.tileSize;
		const uint32 tileY = gridY / m_header.tileSize;
		const uint32 tileIndex = tileY * m_header.tileCountX + tileX;

		const uint8* pTileData = GetTileData(tileIndex);
		if (!pTileData)
			return 0;

		const uint32 localX = gridX % m_header.tileSize;
		const uint32 localY = gridY % m_header.tileSize;
		const uint32 localIndex = ((localY * m_header.tileSize) + localX) * MapLayers::kVegetationChannelCount + channel;

		return pTileData[localIndex];
	}

	const IVegetationMap* CVegetationFullMap::AsVegetationMap() const { return this; }
}