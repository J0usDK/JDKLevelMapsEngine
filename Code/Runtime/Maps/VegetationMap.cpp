#include "StdAfx.h"
#include "VegetationMap.h"

#include "Bootstrap/Reader/MapReader.h"

JDKLevelMaps::Maps::CVegetationMap::CVegetationMap(SLoadedMap&& loadedMap) :
	m_header(loadedMap.header),
	m_packedData(std::move(loadedMap.packedData)),
	m_packedDataSize(loadedMap.packedDataSize),
	m_tileOffsets(std::move(loadedMap.tileOffsets))
{
	const size_t expectedSize = static_cast<size_t>(m_header.tileCountX * m_header.tileCountY);

	m_isValid = m_header.mapType == EMapType::VegetationDensity
		&& m_header.gridWidth > 0 && m_header.gridHeight > 0
		&& m_header.cellSize > 0.0f
		&& m_header.tileSize > 0
		&& m_tileOffsets.size() == expectedSize;
}

uint8 JDKLevelMaps::Maps::CVegetationMap::GetDensity(MapLayers::EVegetationLayers layer, float worldX, float worldY) const
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

	const uint32 offset = m_tileOffsets[tileIndex];
	if (offset == kEmptyTileOffset)
		return 0;

	const uint32 localX = gridX % m_header.tileSize;
	const uint32 localY = gridY % m_header.tileSize;
	const uint32 localIndex = ((localY * m_header.tileSize) + localX) * MapLayers::kVegetationChannelCount + channel;

	return m_packedData[offset + localIndex];
}

JDKLevelMaps::EMapType JDKLevelMaps::Maps::CVegetationMap::GetType() const { return m_header.mapType; }
size_t JDKLevelMaps::Maps::CVegetationMap::GetMemoryUsage() const { return sizeof(SMapHeader) + (m_tileOffsets.capacity() * sizeof(uint32)) + m_packedDataSize; }
bool JDKLevelMaps::Maps::CVegetationMap::IsValid() const { return m_isValid; }